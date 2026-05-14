// ============================================================================
// sync_transfer.cpp - USB 同步数据传输模块（实现文件）
//
// 功能说明：
//   实现 SyncTransfer 类的所有同步传输方法，以及数据传输相关的工具函数。
//   所有传输方法都是对 libusb 同步传输 API 的薄封装，将 C 风格接口
//   转换为类型安全的 C++ 接口，统一返回 TransferResult 结构体。
// ============================================================================

#include "sync_transfer.hpp"
#include "libusb.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace usb_ctrl {
namespace io {

// ============================================================================
// SyncTransfer 构造函数
//
// 保存设备句柄指针。注意：SyncTransfer 不拥有句柄的所有权，
// 调用者负责确保句柄在 SyncTransfer 使用期间保持有效。
// ============================================================================
SyncTransfer::SyncTransfer(libusb_device_handle* handle) : _handle(handle) {}

// ============================================================================
// bulk_read - 批量读取（Bulk IN 传输）
//
// 调用 libusb_bulk_transfer 从指定端点读取数据。
// 超时（LIBUSB_ERROR_TIMEOUT）被视为成功（返回 0 字节数据），
// 因为对于 HID 等设备，超时是正常的（设备没有数据要发送）。
//
// @param endpoint   端点地址（bit7=1 表示 IN 方向）
// @param length     要读取的最大字节数
// @param timeout_ms 超时时间（毫秒）
// @return TransferResult 包含读取结果
// ============================================================================
TransferResult SyncTransfer::bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    TransferResult result; // 创建结果结构体
    result.data.resize(length); // 预分配接收缓冲区
    int transferred = 0; // 实际传输字节数（输出参数）
    // 调用 libusb 批量传输 API
    int rc = libusb_bulk_transfer(_handle, endpoint, result.data.data(), length, &transferred, timeout_ms);
    // 成功或超时都视为成功（超时时 transferred 为 0）
    if (rc == 0 || rc == LIBUSB_ERROR_TIMEOUT) {
        result.success = true;
        result.bytes_transferred = transferred;
        result.data.resize(transferred > 0 ? transferred : 0); // 调整缓冲区大小为实际接收量
    } else {
        // 其他错误：记录错误码和错误信息
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// ============================================================================
// bulk_write - 批量写入（Bulk OUT 传输）
//
// 调用 libusb_bulk_transfer 向指定端点写入数据。
// 注意：需要使用 const_cast 去除 const 限定，因为 libusb API 不保证不修改数据。
//
// @param endpoint   端点地址（bit7=0 表示 OUT 方向）
// @param data       要写入的数据
// @param timeout_ms 超时时间（毫秒）
// @return TransferResult 包含写入结果
// ============================================================================
TransferResult SyncTransfer::bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    TransferResult result; // 创建结果结构体
    int transferred = 0; // 实际传输字节数（输出参数）
    // 调用 libusb 批量传输 API（const_cast 是因为 libusb C API 不保证 const 正确性）
    int rc = libusb_bulk_transfer(_handle, endpoint,
                                   const_cast<unsigned char*>(data.data()),
                                   static_cast<int>(data.size()), &transferred, timeout_ms);
    // 仅当返回 0 时表示成功
    if (rc == 0) {
        result.success = true;
        result.bytes_transferred = transferred;
    } else {
        // 写入失败：记录错误信息
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// ============================================================================
// interrupt_read - 中断读取（Interrupt IN 传输）
//
// 调用 libusb_interrupt_transfer 从 HID 中断端点读取数据。
// 这是 HID 设备最常用的数据传输方式（键盘按键、鼠标移动等）。
// 超时被视为成功（设备可能没有新数据）。
//
// @param endpoint   端点地址
// @param length     要读取的最大字节数
// @param timeout_ms 超时时间（毫秒）
// @return TransferResult 包含读取结果
// ============================================================================
TransferResult SyncTransfer::interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    TransferResult result; // 创建结果结构体
    result.data.resize(length); // 预分配接收缓冲区
    int transferred = 0; // 实际传输字节数（输出参数）
    // 调用 libusb 中断传输 API
    int rc = libusb_interrupt_transfer(_handle, endpoint, result.data.data(), length, &transferred, timeout_ms);
    // 成功或超时都视为成功
    if (rc == 0 || rc == LIBUSB_ERROR_TIMEOUT) {
        result.success = true;
        result.bytes_transferred = transferred;
        result.data.resize(transferred > 0 ? transferred : 0); // 调整缓冲区大小
    } else {
        // 其他错误：记录错误信息
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// ============================================================================
// interrupt_write - 中断写入（Interrupt OUT 传输）
//
// 调用 libusb_interrupt_transfer 向 HID 中断端点写入数据。
// 用于向 HID 设备发送输出报告（如设置键盘 LED）。
//
// @param endpoint   端点地址
// @param data       要写入的数据
// @param timeout_ms 超时时间（毫秒）
// @return TransferResult 包含写入结果
// ============================================================================
TransferResult SyncTransfer::interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    TransferResult result; // 创建结果结构体
    int transferred = 0; // 实际传输字节数（输出参数）
    // 调用 libusb 中断传输 API
    int rc = libusb_interrupt_transfer(_handle, endpoint,
                                        const_cast<unsigned char*>(data.data()),
                                        static_cast<int>(data.size()), &transferred, timeout_ms);
    // 仅当返回 0 时表示成功
    if (rc == 0) {
        result.success = true;
        result.bytes_transferred = transferred;
    } else {
        // 写入失败：记录错误信息
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// ============================================================================
// control_transfer - 控制传输（带数据输出）
//
// 调用 libusb_control_transfer 发送 USB 控制请求。
// 用于 HID Set_Report 等需要发送数据的控制请求。
// 返回值 >= 0 表示成功（返回传输的字节数）。
//
// @param bmRequestType 请求类型（方向+类型+接收者）
// @param bRequest      请求代码（如 0x09 = SET_REPORT）
// @param wValue        请求值
// @param wIndex        请求索引
// @param data          要发送的数据
// @param timeout_ms    超时时间
// @return TransferResult 包含传输结果
// ============================================================================
TransferResult SyncTransfer::control_transfer(uint8_t bmRequestType, uint8_t bRequest,
                                               uint16_t wValue, uint16_t wIndex,
                                               const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    TransferResult result; // 创建结果结构体
    // 调用 libusb 控制传输 API
    int rc = libusb_control_transfer(_handle, bmRequestType, bRequest,
                                      wValue, wIndex,
                                      const_cast<unsigned char*>(data.data()),
                                      static_cast<uint16_t>(data.size()), timeout_ms);
    // 返回值 >= 0 表示成功（返回实际传输字节数）
    if (rc >= 0) {
        result.success = true;
        result.bytes_transferred = rc;
    } else {
        // 传输失败：记录错误信息
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// ============================================================================
// control_read - 控制读取（带数据输入）
//
// 调用 libusb_control_transfer 发送 USB 控制请求并读取响应数据。
// 用于 HID Get_Report 等需要读取数据的控制请求。
//
// @param bmRequestType 请求类型（方向+类型+接收者）
// @param bRequest      请求代码（如 0x01 = GET_REPORT）
// @param wValue        请求值
// @param wIndex        请求索引
// @param wLength       期望读取的数据长度
// @param timeout_ms    超时时间
// @return TransferResult 包含读取结果和数据
// ============================================================================
TransferResult SyncTransfer::control_read(uint8_t bmRequestType, uint8_t bRequest,
                                           uint16_t wValue, uint16_t wIndex,
                                           uint16_t wLength, unsigned int timeout_ms) {
    TransferResult result; // 创建结果结构体
    result.data.resize(wLength); // 预分配接收缓冲区
    // 调用 libusb 控制传输 API（读取方向）
    int rc = libusb_control_transfer(_handle, bmRequestType, bRequest,
                                      wValue, wIndex, result.data.data(), wLength, timeout_ms);
    // 返回值 >= 0 表示成功
    if (rc >= 0) {
        result.success = true;
        result.bytes_transferred = rc;
        result.data.resize(rc); // 调整缓冲区大小为实际接收量
    } else {
        // 传输失败：记录错误信息
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// ============================================================================
// bytes_to_hex - 将字节数组转换为十六进制字符串
//
// 每 16 个字节换行，超过 max_bytes 的部分显示 "... (N bytes total)"。
//
// 输出格式示例：
//   48 65 6C 6C 6F 20 57 6F 72 6C 64 21 00 00 00 00
//   01 02 03 04
//
// @param data      字节数组
// @param max_bytes 最大显示字节数
// @return 十六进制格式的字符串
// ============================================================================
std::string bytes_to_hex(const std::vector<uint8_t>& data, size_t max_bytes) {
    std::ostringstream oss; // 字符串输出流
    size_t len = std::min(data.size(), max_bytes); // 实际显示的字节数
    // 逐个字节格式化为两位大写十六进制
    for (size_t i = 0; i < len; ++i) {
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]) << " "; // 两位十六进制 + 空格
        // 每 16 个字节换行（但不是最后一行）
        if ((i + 1) % 16 == 0 && i + 1 < len) oss << "\n";
    }
    // 如果数据超过最大显示字节数，显示省略提示
    if (data.size() > max_bytes) oss << "... (" << data.size() << " bytes total)";
    return oss.str();
}

// ============================================================================
// bytes_to_ascii - 将字节数组转换为 ASCII 字符串
//
// 可打印字符原样显示，不可打印字符显示为 '.'。
//
// @param data      字节数组
// @param max_bytes 最大显示字节数
// @return ASCII 格式的字符串
// ============================================================================
std::string bytes_to_ascii(const std::vector<uint8_t>& data, size_t max_bytes) {
    std::ostringstream oss; // 字符串输出流
    size_t len = std::min(data.size(), max_bytes); // 实际显示的字节数
    // 逐个字节转换为 ASCII 字符
    for (size_t i = 0; i < len; ++i) {
        char c = static_cast<char>(data[i]); // 转换为 char
        // 可打印字符原样显示，不可打印字符显示为 '.'
        oss << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
    }
    return oss.str();
}

// ============================================================================
// format_transfer_result - 格式化传输结果为可读字符串
//
// 成功时显示传输字节数和数据的十六进制/ASCII 对照。
// 失败时显示错误信息。
//
// 输出格式示例：
//   SUCCESS 8 bytes
//     Hex  : 48 65 6C 6C 6F 20 57 6F
//     ASCII: Hello Wo
//
// @param r       传输结果结构体
// @param max_hex 十六进制显示的最大字节数
// @return 格式化的结果字符串
// ============================================================================
std::string format_transfer_result(const TransferResult& r, size_t max_hex) {
    std::ostringstream oss; // 字符串输出流
    // 第一部分：成功/失败状态
    oss << (r.success ? "SUCCESS" : "FAILED");
    if (r.success) {
        // 成功：显示传输字节数
        oss << " " << r.bytes_transferred << " bytes";
        // 如果有数据，显示十六进制和 ASCII 对照
        if (!r.data.empty()) {
            oss << "\n  Hex  : " << bytes_to_hex(r.data, max_hex);
            oss << "\n  ASCII: " << bytes_to_ascii(r.data, max_hex);
        }
    } else {
        // 失败：显示错误信息
        oss << " [" << r.error_message << "]";
    }
    return oss.str();
}

} // namespace io
} // namespace usb_ctrl
