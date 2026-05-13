#pragma once

#include <memory>
#include <string>
#include <stdexcept>

// 前向声明 libusb 上下文结构体
struct libusb_context;
struct libusb_device;
typedef struct libusb_device libusb_device;

namespace usb_core {

// USB 操作异常类，继承自 std::runtime_error
// 用于包装 libusb 的错误码并提供可读的错误信息
class UsbException : public std::runtime_error {
public:
    UsbException(const std::string& msg, int error_code = 0)
        : std::runtime_error(msg), _error_code(error_code) {}

    int error_code() const { return _error_code; }

private:
    int _error_code;
};

// USB 上下文管理类
// 使用 RAII 模式封装 libusb_context 的生命周期
// 构造时初始化 libusb，析构时自动释放资源
class UsbContext {
public:
    UsbContext();
    ~UsbContext();

    // 禁止拷贝和移动，保证单例语义
    UsbContext(const UsbContext&) = delete;
    UsbContext& operator=(const UsbContext&) = delete;
    UsbContext(UsbContext&&) = delete;
    UsbContext& operator=(UsbContext&&) = delete;

    // 获取底层 libusb_context 指针，供其他模块使用
    libusb_context* handle() const { return _ctx; }

    // 设置 libusb 调试日志级别 (0=无日志, 4=全部日志)
    void set_debug_level(int level);

private:
    libusb_context* _ctx = nullptr;
};

} // namespace usb_core
