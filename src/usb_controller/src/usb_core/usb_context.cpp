// ============================================================================
// usb_context.cpp - USB 上下文管理模块（实现文件）
//
// 功能说明：
//   实现 UsbContext 类的构造函数、析构函数和调试级别设置方法。
//   通过 libusb_init() / libusb_exit() 管理 libusb 库的生命周期。
// ============================================================================

#include "usb_context.hpp" // 自身头文件
#include "libusb.h"        // libusb 库 API 声明

namespace usb_ctrl {
namespace core {

// ============================================================================
// UsbContext 构造函数
//
// 调用 libusb_init() 初始化 libusb 库。
// 初始化失败时抛出 UsbException 异常，携带错误码和错误名称。
// ============================================================================
UsbContext::UsbContext() {
    // 调用 libusb_init 初始化 USB 库，获取上下文指针
    int rc = libusb_init(&_ctx);
    // 检查返回值：负数表示初始化失败
    if (rc < 0) {
        // 抛出异常，包含错误描述和错误码
        throw UsbException(
            std::string("libusb_init: ") + libusb_error_name(rc), rc);
    }
}

// ============================================================================
// UsbContext 析构函数
//
// 调用 libusb_exit() 清理 libusb 库资源。
// 仅在上下文指针非空时执行清理，防止重复释放。
// ============================================================================
UsbContext::~UsbContext() {
    // 检查上下文指针是否有效
    if (_ctx) {
        // 释放 libusb 库资源，关闭所有已打开的设备句柄
        libusb_exit(_ctx);
    }
}

// ============================================================================
// set_debug_level - 设置 libusb 调试输出级别
//
// @param level 调试级别整数值
//   0 = LIBUSB_LOG_LEVEL_NONE    : 无输出
//   1 = LIBUSB_LOG_LEVEL_ERROR   : 仅错误
//   2 = LIBUSB_LOG_LEVEL_WARNING : 错误+警告
//   3 = LIBUSB_LOG_LEVEL_INFO    : 错误+警告+信息
//   4 = LIBUSB_LOG_LEVEL_DEBUG   : 全部输出
// ============================================================================
void UsbContext::set_debug_level(int level) {
    // 调用 libusb_set_debug 设置日志输出级别
    libusb_set_debug(_ctx, level);
}

} // namespace core
} // namespace usb_ctrl
