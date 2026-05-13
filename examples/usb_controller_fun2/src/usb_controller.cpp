#include "usb_controller.hpp"

#include <sstream>
#include <iostream>

namespace usb_ctrl {

// UsbController 构造函数：初始化 USB 上下文和设备管理器
UsbController::UsbController() {
    _ctx = std::make_unique<UsbContext>();
    _manager = std::make_unique<UsbDeviceManager>(*_ctx);
}

// UsbController 析构函数：清理所有资源
UsbController::~UsbController() {
    close_hid_device();
    async_stop();
}

// 设置 libusb 调试日志级别
// level: 0=无日志, 1=错误, 2=警告, 3=信息, 4=调试
void UsbController::set_debug_level(int level) {
    _ctx->set_debug_level(level);
}

// 刷新设备列表
// 重新枚举系统中所有 USB 设备
void UsbController::refresh_devices() {
    _manager->refresh();
}

// 获取所有设备的摘要信息
std::string UsbController::list_devices() const {
    return _manager->list_all_summary();
}

// 获取所有设备的详细信息
std::string UsbController::list_devices_detail() const {
    return _manager->list_all_detail();
}

// 获取所有 HID 设备的摘要信息
std::string UsbController::list_hid_devices() const {
    return _manager->list_hid_summary();
}

// 获取设备总数
size_t UsbController::device_count() const {
    return _manager->count();
}

// 获取指定索引设备的详细信息
// index: 设备在列表中的索引
std::string UsbController::device_detail(size_t index) const {
    auto& devs = _manager->devices();
    if (index >= devs.size()) return "Invalid device index";
    return devs[index].detail();
}

// 根据 VID/PID 查找设备
// 返回所有匹配的设备列表
std::vector<UsbDevice> UsbController::find_by_vid_pid(uint16_t vid, uint16_t pid) {
    return _manager->find_by_vid_pid(vid, pid);
}

// 打开指定索引的 HID 设备
// index: 设备在列表中的索引
// 返回: 是否打开成功
bool UsbController::open_hid_device(size_t index) {
    close_hid_device();

    auto& devs = _manager->devices();
    if (index >= devs.size()) return false;

    auto hid = std::make_unique<HidDevice>(devs[index]);
    if (!hid->open()) return false;

    _hid_device = std::move(hid);
    _sync_transfer = std::make_unique<SyncTransfer>(_hid_device->transfer().handle());
    return true;
}

// 根据 VID/PID 打开 HID 设备
// 查找第一个匹配的 HID 设备并打开
// 返回: 是否打开成功
bool UsbController::open_hid_device_by_vid_pid(uint16_t vid, uint16_t pid) {
    auto devs = _manager->find_by_vid_pid(vid, pid);
    for (auto& dev : devs) {
        if (dev.has_hid_interface()) {
            auto hid = std::make_unique<HidDevice>(dev);
            if (hid->open()) {
                close_hid_device();
                _hid_device = std::move(hid);
                _sync_transfer = std::make_unique<SyncTransfer>(_hid_device->transfer().handle());
                return true;
            }
        }
    }
    return false;
}

// 关闭已打开的 HID 设备
// 清理异步传输对象、同步传输对象和 HID 设备对象
void UsbController::close_hid_device() {
    _async_hid.reset();
    _sync_transfer.reset();
    _hid_device.reset();
}

// 判断 HID 设备是否已打开
bool UsbController::is_hid_open() const {
    return _hid_device && _hid_device->is_open();
}

// 获取已打开 HID 设备的信息
std::string UsbController::hid_device_info() const {
    if (!_hid_device) return "No HID device opened";
    return _hid_device->device_summary();
}

// 同步读取 HID 报告
// length: 期望读取的字节数
// timeout_ms: 超时时间 (毫秒)
TransferResult UsbController::hid_read(int length, unsigned int timeout_ms) {
    if (!_hid_device) {
        TransferResult r;
        r.success = false;
        r.error_message = "No HID device opened";
        return r;
    }
    return _hid_device->read_report(length, timeout_ms);
}

// 同步写入 HID 报告
// data: 要发送的报告数据
// timeout_ms: 超时时间 (毫秒)
TransferResult UsbController::hid_write(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_hid_device) {
        TransferResult r;
        r.success = false;
        r.error_message = "No HID device opened";
        return r;
    }
    return _hid_device->write_report(data, timeout_ms);
}

// 获取 HID 特性报告
// report_id: 报告 ID
// length: 期望读取的字节数
// timeout_ms: 超时时间 (毫秒)
TransferResult UsbController::hid_get_feature_report(uint8_t report_id, int length,
                                                      unsigned int timeout_ms) {
    if (!_hid_device) {
        TransferResult r;
        r.success = false;
        r.error_message = "No HID device opened";
        return r;
    }
    return _hid_device->get_feature_report(report_id, length, timeout_ms);
}

// 发送 HID 特性报告
// data: 特性报告数据
// timeout_ms: 超时时间 (毫秒)
TransferResult UsbController::hid_send_feature_report(const std::vector<uint8_t>& data,
                                                       unsigned int timeout_ms) {
    if (!_hid_device) {
        TransferResult r;
        r.success = false;
        r.error_message = "No HID device opened";
        return r;
    }
    return _hid_device->send_feature_report(data, timeout_ms);
}

// 启动异步传输引擎
// 如果引擎尚未创建，则创建并启动
void UsbController::async_start() {
    if (!_async_engine) {
        _async_engine = std::make_unique<AsyncTransferEngine>(_ctx->handle());
        _async_engine->start();
    }
}

// 停止异步传输引擎
// 停止引擎并清理所有异步传输对象
void UsbController::async_stop() {
    _async_hid.reset();
    if (_async_engine) {
        _async_engine->stop();
        _async_engine.reset();
    }
}

// 异步读取 HID 报告 (单次)
// 如果异步引擎尚未启动，则自动启动
// length: 期望读取的字节数
// callback: 读取完成后的回调函数
void UsbController::hid_read_async(int length, AsyncCallback callback, unsigned int timeout_ms) {
    if (!_hid_device || !_hid_device->is_open()) return;
    async_start();
    if (!_async_hid) {
        _async_hid = std::make_unique<AsyncHidTransfer>(
            *_async_engine, _hid_device->transfer().handle(),
            _hid_device->in_endpoint().address,
            _hid_device->out_endpoint().address);
    }
    _async_hid->read_async(length, std::move(callback), timeout_ms);
}

// 异步写入 HID 报告 (单次)
// 如果异步引擎尚未启动，则自动启动
// data: 要发送的报告数据
// callback: 发送完成后的回调函数
void UsbController::hid_write_async(const std::vector<uint8_t>& data, AsyncCallback callback,
                                     unsigned int timeout_ms) {
    if (!_hid_device || !_hid_device->is_open()) return;
    async_start();
    if (!_async_hid) {
        _async_hid = std::make_unique<AsyncHidTransfer>(
            *_async_engine, _hid_device->transfer().handle(),
            _hid_device->in_endpoint().address,
            _hid_device->out_endpoint().address);
    }
    _async_hid->write_async(data, std::move(callback), timeout_ms);
}

// 连续异步读取 HID 报告
// 自动循环提交读取请求，直到调用 hid_stop_continuous()
// length: 期望读取的字节数
// callback: 每次读取完成后的回调函数
void UsbController::hid_read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms) {
    if (!_hid_device || !_hid_device->is_open()) return;
    async_start();
    if (!_async_hid) {
        _async_hid = std::make_unique<AsyncHidTransfer>(
            *_async_engine, _hid_device->transfer().handle(),
            _hid_device->in_endpoint().address,
            _hid_device->out_endpoint().address);
    }
    _async_hid->read_continuous(length, std::move(callback), timeout_ms);
}

// 停止连续异步读取
void UsbController::hid_stop_continuous() {
    if (_async_hid) {
        _async_hid->stop_continuous();
    }
}

// 获取待处理的异步传输请求数量
size_t UsbController::async_pending_count() const {
    return _async_engine ? _async_engine->pending_count() : 0;
}

} // namespace usb_ctrl
