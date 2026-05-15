// ============================================================================
// usb_hotplug_base.cpp - USB 热插拔抽象基类（实现文件）
//
// 功能说明：
//   实现 is_supported() 和 create_hotplug() 工厂函数。
//   平台选择逻辑集中在此文件，UsbController 无需条件编译。
// ============================================================================

#include "usb_hotplug_base.hpp"

#ifdef _WIN32
#include "usb_hotplug_win.hpp"
#else
#include "usb_hotplug_linux.hpp"
#endif

namespace usb_ctrl {
namespace core {

// 检查当前平台是否支持热插拔
bool UsbHotplugBase::is_supported() {
#ifdef _WIN32
    return UsbHotplugWin::is_supported();
#else
    return UsbHotplug::is_supported();
#endif
}

// 工厂函数：创建平台相关的热插拔实例
std::unique_ptr<UsbHotplugBase> create_hotplug(libusb_context* ctx) {
#ifdef _WIN32
    (void)ctx; // Windows 不需要 libusb 上下文
    return std::make_unique<UsbHotplugWin>();
#else
    return std::make_unique<UsbHotplug>(ctx);
#endif
}

} // namespace core
} // namespace usb_ctrl
