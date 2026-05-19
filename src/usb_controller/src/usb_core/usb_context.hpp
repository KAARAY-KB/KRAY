// ============================================================================
// usb_context.hpp - USB 上下文管理模块（头文件）
//
// 功能说明：
//   本文件定义了 USB 上下文（UsbContext）和 USB 异常（UsbException）类。
//   UsbContext 是对 libusb 库上下文的 RAII 封装，负责 libusb 的初始化与销毁。
//   UsbException 是自定义异常类，用于封装 libusb 操作中的错误码和错误信息。
//
// 依赖关系：
//   - libusb.h：底层 USB 操作库
//   - <memory>：智能指针支持
//   - <string>：字符串支持
//   - <stdexcept>：标准异常基类
// ============================================================================

#pragma once // 防止头文件重复包含

#include <memory>   // std::unique_ptr 等智能指针
#include <string>   // std::string 字符串类
#include <stdexcept> // std::runtime_error 标准运行时异常基类

// 前向声明 libusb 上下文结构体，避免在头文件中引入完整的 libusb.h
struct libusb_context;

// ============================================================================
// 命名空间：usb_ctrl::core
// 说明：USB 控制器核心模块，包含设备枚举、上下文管理等基础功能
// ============================================================================
namespace usb_ctrl {
namespace core {

// ============================================================================
// UsbException - USB 操作异常类
//
// 继承自 std::runtime_error，在标准异常基础上增加了 libusb 错误码。
// 用于在 USB 操作失败时抛出，携带可读的错误信息和数字错误码。
// ============================================================================
class UsbException : public std::runtime_error {
public:
    // 构造函数：接收错误消息字符串和可选的错误码
    // @param msg  错误描述信息
    // @param code libusb 返回的错误码，默认为 0
    UsbException(const std::string& msg, int code = 0)
        : std::runtime_error(msg) // 调用基类构造函数设置错误消息
        , _code(code) {}           // 保存错误码

    // 获取 libusb 错误码
    // @return 错误码整数值
    int code() const { return _code; }

private:
    int _code; // libusb 操作返回的错误码（负数表示失败）
};

// ============================================================================
// UsbContext - USB 上下文管理类（RAII 封装）
//
// 对 libusb_context 生命周期的 RAII（资源获取即初始化）封装。
// 构造时调用 libusb_init() 初始化 libusb 库，
// 析构时调用 libusb_exit() 清理资源。
// 禁止拷贝和移动，确保上下文唯一性。
// ============================================================================
class UsbContext {
public:
    // 构造函数：初始化 libusb 库，创建上下文
    // 如果初始化失败，抛出 UsbException 异常
    UsbContext();

    // 析构函数：清理 libusb 资源，释放上下文
    ~UsbContext();

    // 禁止拷贝构造和拷贝赋值（上下文不可共享）
    UsbContext(const UsbContext&) = delete;
    UsbContext& operator=(const UsbContext&) = delete;

    // 禁止移动构造和移动赋值（上下文不可转移）
    UsbContext(UsbContext&&) = delete;
    UsbContext& operator=(UsbContext&&) = delete;

    // 获取底层 libusb_context 原始指针
    // @return libusb_context* 原始指针，供 libusb API 调用使用
    libusb_context* handle() const { return _ctx; }

    // 设置 libusb 调试输出级别
    // @param level 调试级别：0=无输出, 1=错误, 2=警告, 3=信息, 4=调试
    void set_debug_level(int level);

private:
    libusb_context* _ctx = nullptr; // libusb 上下文原始指针，初始化为空
};

} // namespace core
} // namespace usb_ctrl
