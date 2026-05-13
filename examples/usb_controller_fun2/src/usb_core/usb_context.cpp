#include "usb_context.hpp"
#include "libusb.h"

namespace usb_core {

// 构造函数：初始化 libusb 上下文
// 如果初始化失败，抛出 UsbException 异常
UsbContext::UsbContext() {
    int rc = libusb_init(&_ctx);
    if (rc < 0) {
        throw UsbException("libusb_init failed: " + std::string(libusb_error_name(rc)), rc);
    }
}

// 析构函数：释放 libusb 上下文资源
// 确保在对象销毁时正确清理 libusb 占用的系统资源
UsbContext::~UsbContext() {
    if (_ctx) {
        libusb_exit(_ctx);
        _ctx = nullptr;
    }
}

// 设置调试日志级别
// level: 0=无日志, 1=错误, 2=警告, 3=信息, 4=调试
void UsbContext::set_debug_level(int level) {
    libusb_set_debug(_ctx, level);
}

} // namespace usb_core
