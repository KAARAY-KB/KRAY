// ============================================================================
// usb_controller.cpp - USB 控制器统一接口（实现文件）
//
// 功能说明：
//   实现 UsbController 类的所有方法。
//   UsbController 作为外观类，将底层模块的方法调用转发到对应的内部组件。
//   大部分方法都是简单的委托（delegation），不包含复杂业务逻辑。
//
// 方法分类：
//   1. 构造/析构：初始化 USB 上下文和设备管理器
//   2. 设备查询：委托给 UsbDeviceManager
//   3. HID 操作：委托给 HidDevice
//   4. 同步传输：委托给 SyncTransfer
//   5. 异步传输：委托给 AsyncTransfer
// ============================================================================

#include "usb_controller.hpp"

namespace usb_ctrl {

// ============================================================================
// UsbController 构造函数
//
// 初始化 USB 上下文（libusb_init）和设备管理器（自动枚举设备）。
// 这是整个 USB 控制库的入口点。
// ============================================================================
UsbController::UsbController() {
    _ctx = std::make_unique<UsbContext>();           // 初始化 libusb
    _manager = std::make_unique<UsbDeviceManager>(*_ctx); // 枚举设备
}

// ============================================================================
// UsbController 析构函数
//
// 按依赖顺序释放资源：先关闭 HID 设备，再停止异步引擎。
// unique_ptr 会自动释放剩余资源。
// ============================================================================
UsbController::~UsbController() {
    hotplug_stop();      // 停止热插拔监听
    close_hid_device(); // 关闭 HID 设备（释放接口、重新附加内核驱动）
    async_stop();       // 停止异步引擎（停止事件处理线程）
}

// ============================================================================
// 基础设置
// ============================================================================

// 设置 libusb 调试级别（委托给 UsbContext）
void UsbController::set_debug_level(int level) { _ctx->set_debug_level(level); }

// 刷新设备列表（委托给 UsbDeviceManager）
void UsbController::refresh_devices() { _manager->refresh(); }

// ============================================================================
// 设备查询（全部委托给 UsbDeviceManager）
// ============================================================================

std::string UsbController::list_devices() const { return _manager->list_summary(); }
std::string UsbController::list_devices_detail() const { return _manager->list_detail(); }
std::string UsbController::list_devices_tree() const { return _manager->list_tree(); }
std::string UsbController::list_hid_devices() const { return _manager->list_hid_summary(); }

size_t UsbController::device_count() const { return _manager->count(); }

// 获取指定索引设备的详细信息
std::string UsbController::device_detail(size_t index) const {
    auto& devs = _manager->devices(); // 获取设备列表引用
    if (index >= devs.size()) return "Invalid device index"; // 索引越界检查
    return devs[index].detail(); // 委托给 UsbDevice::detail()
}

std::vector<UsbDevice> UsbController::find_by_vid_pid(uint16_t vid, uint16_t pid) {
    return _manager->find_by_vid_pid(vid, pid); // 委托给设备管理器
}

std::vector<UsbDevice> UsbController::find_hid_devices() {
    return _manager->find_hid_devices(); // 委托给设备管理器
}

// ============================================================================
// HID 设备操作
// ============================================================================

// 按索引打开 HID 设备
bool UsbController::open_hid_device(size_t index) {
    close_hid_device(); // 先关闭之前打开的设备
    auto& devs = _manager->devices(); // 获取设备列表
    if (index >= devs.size()) return false; // 索引越界检查
    // 创建 HidDevice 对象
    auto hid = std::make_unique<HidDevice>(devs[index]);
    // 打开设备（检测 HID 接口、分离内核驱动、声明接口）
    if (!hid->open()) return false;
    _hid_device = std::move(hid); // 保存 HID 设备对象
    // 创建同步传输对象（复用 HID 设备的句柄）
    _sync_transfer = std::make_unique<SyncTransfer>(_hid_device->transfer().handle());
    return true;
}

// 按 VID/PID 打开 HID 设备
bool UsbController::open_hid_device_by_vid_pid(uint16_t vid, uint16_t pid) {
    // 查找匹配 VID/PID 的设备
    auto devs = _manager->find_by_vid_pid(vid, pid);
    // 遍历匹配的设备，尝试打开第一个有 HID 接口的设备
    for (auto& dev : devs) {
        if (dev.has_hid_interface()) { // 检查是否有 HID 接口
            auto hid = std::make_unique<HidDevice>(dev);
            if (hid->open()) { // 尝试打开
                close_hid_device(); // 关闭之前打开的设备
                _hid_device = std::move(hid);
                _sync_transfer = std::make_unique<SyncTransfer>(_hid_device->transfer().handle());
                return true;
            }
        }
    }
    return false;
}

// 关闭 HID 设备
void UsbController::close_hid_device() {
    _async_transfer.reset(); // 释放异步传输对象
    _event_thread.reset();   // 停止并释放事件线程
    _sync_transfer.reset(); // 释放同步传输对象
    _hid_device.reset();    // 释放 HID 设备对象（自动调用 close()）
}

// 检查 HID 设备是否已打开
bool UsbController::is_hid_open() const {
    return _hid_device && _hid_device->is_open(); // 对象存在且已打开
}

// 获取 HID 设备信息
std::string UsbController::hid_device_info() const {
    if (!_hid_device) return "No HID device opened"; // 未打开设备
    return _hid_device->device_summary(); // 委托给 HidDevice
}

// ============================================================================
// HID 报告读写（委托给 HidDevice）
// ============================================================================

TransferResult UsbController::hid_read(int length, unsigned int timeout_ms) {
    if (!_hid_device) return {false, 0, 0, "No HID device opened", {}}; // 设备未打开
    return _hid_device->read_report(length, timeout_ms); // 委托给 HidDevice
}

// 读取最新的 HID 输入报告（先排空缓冲区，再等待设备新数据）
TransferResult UsbController::hid_read_latest(int length, unsigned int drain_timeout_ms) {
    if (!_hid_device) return {false, 0, 0, "No HID device opened", {}}; // 设备未打开
    return _hid_device->read_latest(length, drain_timeout_ms); // 委托给 HidDevice::read_latest()
}

TransferResult UsbController::hid_write(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_hid_device) return {false, 0, 0, "No HID device opened", {}};
    return _hid_device->write_report(data, timeout_ms);
}

TransferResult UsbController::hid_get_feature_report(uint8_t report_id, int length, unsigned int timeout_ms) {
    if (!_hid_device) return {false, 0, 0, "No HID device opened", {}};
    return _hid_device->get_feature_report(report_id, length, timeout_ms);
}

TransferResult UsbController::hid_send_feature_report(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_hid_device) return {false, 0, 0, "No HID device opened", {}};
    return _hid_device->send_feature_report(data, timeout_ms);
}

// ============================================================================
// 同步传输（委托给 SyncTransfer）
// ============================================================================

TransferResult UsbController::bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    if (!_sync_transfer) return {false, 0, 0, "No sync transfer available", {}}; // 传输对象未创建
    return _sync_transfer->bulk_read(endpoint, length, timeout_ms);
}

TransferResult UsbController::bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_sync_transfer) return {false, 0, 0, "No sync transfer available", {}};
    return _sync_transfer->bulk_write(endpoint, data, timeout_ms);
}

TransferResult UsbController::interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    if (!_sync_transfer) return {false, 0, 0, "No sync transfer available", {}};
    return _sync_transfer->interrupt_read(endpoint, length, timeout_ms);
}

TransferResult UsbController::interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_sync_transfer) return {false, 0, 0, "No sync transfer available", {}};
    return _sync_transfer->interrupt_write(endpoint, data, timeout_ms);
}

// ============================================================================
// 异步传输
// ============================================================================

// 启动异步传输引擎
void UsbController::async_start() {
    if (!_event_thread) {
        // 创建异步传输对象（使用 libusb 上下文）
        _event_thread = std::make_unique<UsbEventThread>(_ctx->handle());
        _event_thread->start(); // 启动事件处理线程
    }
}

// 停止异步传输引擎
void UsbController::async_stop() {
    _async_transfer.reset(); // 释放异步传输对象
    if (_event_thread) {
        _event_thread->stop();
        _event_thread.reset(); // 释放事件线程
    }
}

// 确保异步传输对象已创建并绑定设备
void UsbController::_ensure_async_bound() {
    async_start(); // 确保引擎已启动
    if (_hid_device && _hid_device->is_open()) {
        if (!_async_transfer)
            _async_transfer = std::make_unique<AsyncTransfer>(*_event_thread);
    // 绑定设备（如果尚未绑定）
        if (!_async_transfer->is_bound())
            _async_transfer->bind_device(_hid_device->handle(),
                                         _hid_device->in_endpoint().address,
                                         _hid_device->out_endpoint().address);
    }
}

// 单次异步 HID 读取
void UsbController::hid_read_async(int length, AsyncCallback callback, unsigned int timeout_ms) {
    if (!_hid_device || !_hid_device->is_open()) return; // 设备未打开
    _ensure_async_bound(); // 确保引擎已启动且设备已绑定
    _async_transfer->read_async(length, std::move(callback), timeout_ms); // 提交异步读取
}

// 单次异步 HID 写入
void UsbController::hid_write_async(const std::vector<uint8_t>& data, AsyncCallback callback, unsigned int timeout_ms) {
    if (!_hid_device || !_hid_device->is_open()) return;
    _ensure_async_bound();
    _async_transfer->write_async(data, std::move(callback), timeout_ms); // 提交异步写入
}

// 连续异步 HID 读取
void UsbController::hid_read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms) {
    if (!_hid_device || !_hid_device->is_open()) return;
    _ensure_async_bound();
    _async_transfer->read_continuous(length, std::move(callback), timeout_ms); // 启动连续读取
}

// 停止连续异步读取
void UsbController::hid_stop_continuous() {
    if (_async_transfer) _async_transfer->stop_continuous(); // 委托给 AsyncTransfer
}

// 获取异步传输待处理数量
size_t UsbController::async_pending_count() const {
    return _async_transfer ? _async_transfer->pending_count() : 0; // 引擎未启动时返回 0
}

// ============================================================================
// 热插拔监听
// ============================================================================

// 检查当前平台是否支持热插拔
bool UsbController::is_hotplug_supported() {
    return UsbHotplugBase::is_supported();
}

// 启动热插拔监听（无过滤，监听所有设备）
bool UsbController::hotplug_start(HotplugCallback callback) {
    if (_hotplug) _hotplug->stop();
    _hotplug = core::create_hotplug(_ctx->handle());
    _hotplug->set_callback(std::move(callback));
    return _hotplug->start();
}

// 启动热插拔监听（带 VID/PID 过滤）
bool UsbController::hotplug_start(uint16_t vid, uint16_t pid, HotplugCallback callback) {
    if (_hotplug) _hotplug->stop();
    _hotplug = core::create_hotplug(_ctx->handle());
    _hotplug->set_vid_filter(vid);
    _hotplug->set_pid_filter(pid);
    _hotplug->set_callback(std::move(callback));
    return _hotplug->start();
}

// 停止热插拔监听
void UsbController::hotplug_stop() {
    if (_hotplug) {
        _hotplug->stop(); // 注销回调
        _hotplug.reset(); // 释放对象
    }
}

// 检查热插拔是否正在监听
bool UsbController::is_hotplug_listening() const {
    return _hotplug && _hotplug->is_listening();
}

} // namespace usb_ctrl
