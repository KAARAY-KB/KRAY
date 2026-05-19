// ============================================================================
// usb_hotplug_linux.cpp - USB 热插拔异步监听模块 - Linux 版本（实现文件）
//
// 功能说明：
//   实现 UsbHotplugLinux 类的所有方法，包括：
//   - 平台热插拔能力检测
//   - 回调注册（libusb_hotplug_register_callback）
//   - 回调注销（libusb_hotplug_deregister_callback）
//   - VID/PID/类代码过滤
//   - 热插拔事件分发
// ============================================================================

#include "usb_hotplug_linux.hpp"

namespace usb_ctrl {
namespace core {

// ============================================================================
// UsbHotplugLinux 构造函数
// ============================================================================
UsbHotplugLinux::UsbHotplugLinux(libusb_context* ctx) : _ctx(ctx) {}

// ============================================================================
// UsbHotplugLinux 析构函数：自动注销热插拔回调
// ============================================================================
UsbHotplugLinux::~UsbHotplugLinux() { stop(); }

// ============================================================================
// is_supported - 检查当前平台是否支持热插拔
// ============================================================================
bool UsbHotplugLinux::is_supported() {
    return libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) != 0;
}

// ============================================================================
// set_callback - 设置热插拔回调函数
// ============================================================================
void UsbHotplugLinux::set_callback(HotplugCallback cb) { _callback = std::move(cb); }

// ============================================================================
// set_vid_filter - 设置 VID 过滤条件
// ============================================================================
void UsbHotplugLinux::set_vid_filter(uint16_t vid) { _vid = vid; }

// ============================================================================
// set_pid_filter - 设置 PID 过滤条件
// ============================================================================
void UsbHotplugLinux::set_pid_filter(uint16_t pid) { _pid = pid; }

// ============================================================================
// set_class_filter - 设置设备类过滤条件
// ============================================================================
void UsbHotplugLinux::set_class_filter(uint8_t cls) { _cls = cls; }

// ============================================================================
// is_listening - 检查是否正在监听
// ============================================================================
bool UsbHotplugLinux::is_listening() const { return _handle != 0; }

// ============================================================================
// start - 启动热插拔监听
// ============================================================================
bool UsbHotplugLinux::start() {
    if (is_listening()) return true;
    if (!is_supported()) return false;

    int rc = libusb_hotplug_register_callback(
        _ctx,
        static_cast<libusb_hotplug_event>(
            LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
            LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
        LIBUSB_HOTPLUG_ENUMERATE,
        _vid,
        _pid,
        _cls,
        _hotplug_cb,
        this,
        &_handle);

    return rc == LIBUSB_SUCCESS;
}

// ============================================================================
// stop - 停止热插拔监听
// ============================================================================
void UsbHotplugLinux::stop() {
    if (_handle != 0) {
        libusb_hotplug_deregister_callback(_ctx, _handle);
        _handle = 0;
    }
}

// ============================================================================
// _hotplug_cb - libusb 热插拔回调静态入口
// ============================================================================
int LIBUSB_CALL UsbHotplugLinux::_hotplug_cb(
    libusb_context* ctx,
    libusb_device* dev,
    libusb_hotplug_event event,
    void* user_data)
{
    auto* self = static_cast<UsbHotplugLinux*>(user_data);
    self->_on_event(dev, event);
    return 0;
}

// ============================================================================
// _on_event - 处理热插拔事件
// ============================================================================
void UsbHotplugLinux::_on_event(libusb_device* dev, libusb_hotplug_event event) {
    if (!_callback) return;

    HotplugEvent ev = (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
                          ? HotplugEvent::Arrived
                          : HotplugEvent::Left;

    // 立即读取设备信息并缓存，避免回调中设备引用失效或打开失败
    UsbDevice device(dev);
    DeviceInfo saved_info = device.get_info();

    // 用缓存信息创建新 UsbDevice，确保回调拿到完整数据
    UsbDevice cached_dev(saved_info);
    _callback(ev, cached_dev);
}

} // namespace core
} // namespace usb_ctrl
