// ============================================================================
// usb_hotplug.cpp - USB 热插拔异步监听模块（实现文件）
//
// 功能说明：
//   实现 UsbHotplug 类的所有方法，包括：
//   - 平台热插拔能力检测
//   - 回调注册（libusb_hotplug_register_callback）
//   - 回调注销（libusb_hotplug_deregister_callback）
//   - VID/PID/类代码过滤
//   - 热插拔事件分发
// ============================================================================

#include "usb_hotplug.hpp" // 自身头文件
#include "libusb.h"        // libusb 热插拔 API

namespace usb_ctrl {
namespace core {

// ============================================================================
// UsbHotplug 构造函数
//
// 保存 libusb 上下文指针，初始化过滤条件为匹配所有设备。
// ============================================================================
UsbHotplug::UsbHotplug(libusb_context* ctx) : _ctx(ctx) {}

// ============================================================================
// UsbHotplug 析构函数
//
// 自动注销热插拔回调，防止悬空回调。
// ============================================================================
UsbHotplug::~UsbHotplug() { stop(); }

// ============================================================================
// is_supported - 检查当前平台是否支持热插拔
//
// 调用 libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) 检测。
// Windows (winusb) 从 libusb 1.0.22 开始支持热插拔。
// Linux 原生支持（基于 udev）。
// macOS 原生支持。
//
// @return true 表示支持热插拔
// ============================================================================
bool UsbHotplug::is_supported() {
    // 查询 libusb 是否具有热插拔能力
    return libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) != 0;
}

// ============================================================================
// set_callback - 设置热插拔回调函数
//
// @param cb 回调函数，接收事件类型和设备对象
// ============================================================================
void UsbHotplug::set_callback(HotplugCallback cb) { _callback = std::move(cb); }

// ============================================================================
// set_vid_filter - 设置 VID 过滤条件
//
// 仅监听指定厂商 ID 的设备。
// 传入 LIBUSB_HOTPLUG_MATCH_ANY 表示匹配所有厂商。
//
// @param vid 厂商 ID
// ============================================================================
void UsbHotplug::set_vid_filter(uint16_t vid) { _vid = vid; }

// ============================================================================
// set_pid_filter - 设置 PID 过滤条件
//
// 仅监听指定产品 ID 的设备。
// 传入 LIBUSB_HOTPLUG_MATCH_ANY 表示匹配所有产品。
//
// @param pid 产品 ID
// ============================================================================
void UsbHotplug::set_pid_filter(uint16_t pid) { _pid = pid; }

// ============================================================================
// set_class_filter - 设置设备类过滤条件
//
// 仅监听指定设备类的设备（如 0x03=HID）。
// 传入 LIBUSB_HOTPLUG_MATCH_ANY 表示匹配所有类。
//
// @param cls 设备类代码
// ============================================================================
void UsbHotplug::set_class_filter(uint8_t cls) { _cls = cls; }

// ============================================================================
// start - 启动热插拔监听
//
// 调用 libusb_hotplug_register_callback 注册回调。
// 注册后，当事件线程（UsbEventThread）运行 libusb_handle_events_timeout 时，
// 设备插入/拔出事件会触发 _hotplug_cb 回调。
//
// 注册标志：
//   LIBUSB_HOTPLUG_ENUMERATE - 注册时枚举已存在的设备（触发 Arrived 回调）
//
// @return true 表示注册成功
// ============================================================================
bool UsbHotplug::start() {
    // 如果已在监听，直接返回成功
    if (_handle != 0) return true;

    // 检查平台是否支持热插拔
    if (!is_supported()) return false;

    // 注册热插拔回调，监听设备到达和离开事件
    int rc = libusb_hotplug_register_callback(
        _ctx,                                    // libusb 上下文
        static_cast<libusb_hotplug_event>(
            LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | // 设备插入事件
            LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),    // 设备拔出事件
        LIBUSB_HOTPLUG_ENUMERATE,                // 枚举已存在的设备
        _vid,                                    // 厂商 ID 过滤
        _pid,                                    // 产品 ID 过滤
        _cls,                                    // 设备类过滤
        _hotplug_cb,                             // 静态回调函数
        this,                                    // 传递 this 作为 user_data
        &_handle);                               // 输出回调句柄

    // 返回注册结果
    return rc == LIBUSB_SUCCESS;
}

// ============================================================================
// stop - 停止热插拔监听
//
// 调用 libusb_hotplug_deregister_callback 注销回调。
// 注销后不再接收热插拔事件。
// ============================================================================
void UsbHotplug::stop() {
    // 检查是否已注册
    if (_handle != 0) {
        // 注销热插拔回调
        libusb_hotplug_deregister_callback(_ctx, _handle);
        // 重置句柄为 0
        _handle = 0;
    }
}

// ============================================================================
// _hotplug_cb - libusb 热插拔回调静态入口
//
// 由 libusb 在事件循环中调用。
// 通过 user_data 获取 UsbHotplug 实例，转发给实例方法 _on_event。
//
// @param ctx       libusb 上下文（未使用）
// @param dev       发生事件的 libusb 设备
// @param event     libusb 热插拔事件类型
// @param user_data 注册时传入的 this 指针
// @return 0 表示成功（libusb 要求返回 0）
// ============================================================================
int LIBUSB_CALL UsbHotplug::_hotplug_cb(
    libusb_context* ctx,
    libusb_device* dev,
    libusb_hotplug_event event,
    void* user_data)
{
    // 获取 UsbHotplug 实例指针
    auto* self = static_cast<UsbHotplug*>(user_data);
    // 转发给实例方法处理
    self->_on_event(dev, event);
    // 返回 0 表示回调成功
    return 0;
}

// ============================================================================
// _on_event - 处理热插拔事件
//
// 将 libusb 设备指针包装为 UsbDevice 对象，
// 将 libusb 事件类型转换为 HotplugEvent 枚举，
// 然后调用用户回调函数。
//
// 注意：UsbDevice 构造时会调用 libusb_ref_device 增加引用计数，
// 回调结束后 UsbDevice 析构时会自动 libusb_unref_device 减少引用计数。
//
// @param dev   发生事件的 libusb 设备
// @param event libusb 热插拔事件类型
// ============================================================================
void UsbHotplug::_on_event(libusb_device* dev, libusb_hotplug_event event) {
    // 如果用户未设置回调，直接返回
    if (!_callback) return;

    // 将 libusb 事件类型转换为 HotplugEvent 枚举
    HotplugEvent ev = (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
                          ? HotplugEvent::Arrived  // 设备插入
                          : HotplugEvent::Left;    // 设备拔出

    // 将 libusb_device* 包装为 UsbDevice 对象（RAII 管理引用计数）
    UsbDevice device(dev);

    // 调用用户回调函数
    _callback(ev, device);
}

} // namespace core
} // namespace usb_ctrl
