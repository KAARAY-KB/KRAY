#include "usb_transfer.hpp"
#include "usb_core/usb_context.hpp"
#include "libusb.h"

#include <sstream>
#include <iomanip>

namespace usb_transfer {

// SyncTransfer 构造函数：接收已打开的设备句柄
SyncTransfer::SyncTransfer(libusb_device_handle* handle)
    : _handle(handle) {
}

// Bulk 读操作
// endpoint: 端点地址 (应为 IN 端点)
// length: 期望读取的字节数
// timeout_ms: 超时时间 (毫秒)
// 注意：超时也可能返回部分数据，视为成功
TransferResult SyncTransfer::bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    TransferResult result;
    result.data.resize(length);
    int transferred = 0;
    int rc = libusb_bulk_transfer(_handle, endpoint, result.data.data(), length,
                                   &transferred, timeout_ms);
    if (rc == 0 || rc == LIBUSB_ERROR_TIMEOUT) {
        result.success = true;
        result.bytes_transferred = transferred;
        result.data.resize(transferred > 0 ? transferred : 0);
    } else {
        result.success = false;
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// Bulk 写操作
// endpoint: 端点地址 (应为 OUT 端点)
// data: 要发送的数据
// timeout_ms: 超时时间 (毫秒)
TransferResult SyncTransfer::bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data,
                                         unsigned int timeout_ms) {
    TransferResult result;
    int transferred = 0;
    int rc = libusb_bulk_transfer(_handle, endpoint,
                                   const_cast<unsigned char*>(data.data()),
                                   static_cast<int>(data.size()),
                                   &transferred, timeout_ms);
    if (rc == 0) {
        result.success = true;
        result.bytes_transferred = transferred;
    } else {
        result.success = false;
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// Interrupt 读操作
// endpoint: 端点地址 (应为 IN 端点)
// length: 期望读取的字节数
// timeout_ms: 超时时间 (毫秒)
// 注意：超时也可能返回部分数据，视为成功
TransferResult SyncTransfer::interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    TransferResult result;
    result.data.resize(length);
    int transferred = 0;
    int rc = libusb_interrupt_transfer(_handle, endpoint, result.data.data(), length,
                                        &transferred, timeout_ms);
    if (rc == 0 || rc == LIBUSB_ERROR_TIMEOUT) {
        result.success = true;
        result.bytes_transferred = transferred;
        result.data.resize(transferred > 0 ? transferred : 0);
    } else {
        result.success = false;
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// Interrupt 写操作
// endpoint: 端点地址 (应为 OUT 端点)
// data: 要发送的数据
// timeout_ms: 超时时间 (毫秒)
TransferResult SyncTransfer::interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data,
                                              unsigned int timeout_ms) {
    TransferResult result;
    int transferred = 0;
    int rc = libusb_interrupt_transfer(_handle, endpoint,
                                        const_cast<unsigned char*>(data.data()),
                                        static_cast<int>(data.size()),
                                        &transferred, timeout_ms);
    if (rc == 0) {
        result.success = true;
        result.bytes_transferred = transferred;
    } else {
        result.success = false;
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// Control 传输操作 (带数据的控制请求)
// bmRequestType: 请求类型 (方向、类型、接收者)
// bRequest: 请求代码
// wValue: 值字段
// wIndex: 索引字段
// data: 要发送的数据
// timeout_ms: 超时时间 (毫秒)
TransferResult SyncTransfer::control_transfer(uint8_t bmRequestType, uint8_t bRequest,
                                               uint16_t wValue, uint16_t wIndex,
                                               const std::vector<uint8_t>& data,
                                               unsigned int timeout_ms) {
    TransferResult result;
    int rc = libusb_control_transfer(_handle, bmRequestType, bRequest,
                                      wValue, wIndex,
                                      const_cast<unsigned char*>(data.data()),
                                      static_cast<uint16_t>(data.size()),
                                      timeout_ms);
    if (rc >= 0) {
        result.success = true;
        result.bytes_transferred = rc;
    } else {
        result.success = false;
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// Control 读操作 (从设备读取控制数据)
// wLength: 期望读取的字节数
TransferResult SyncTransfer::control_read(uint8_t bmRequestType, uint8_t bRequest,
                                           uint16_t wValue, uint16_t wIndex,
                                           uint16_t wLength, unsigned int timeout_ms) {
    TransferResult result;
    result.data.resize(wLength);
    int rc = libusb_control_transfer(_handle, bmRequestType, bRequest,
                                      wValue, wIndex,
                                      result.data.data(), wLength, timeout_ms);
    if (rc >= 0) {
        result.success = true;
        result.bytes_transferred = rc;
        result.data.resize(rc);
    } else {
        result.success = false;
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

// 将字节数组转换为十六进制字符串
// max_bytes: 最大转换的字节数 (防止输出过长)
std::string bytes_to_hex(const std::vector<uint8_t>& data, size_t max_bytes) {
    std::ostringstream oss;
    size_t len = std::min(data.size(), max_bytes);
    for (size_t i = 0; i < len; ++i) {
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]) << " ";
    }
    if (data.size() > max_bytes) {
        oss << "... (" << data.size() << " bytes total)";
    }
    return oss.str();
}

// 将字节数组转换为 ASCII 字符串
// 不可打印字符显示为 '.'
// max_bytes: 最大转换的字节数
std::string bytes_to_ascii(const std::vector<uint8_t>& data, size_t max_bytes) {
    std::ostringstream oss;
    size_t len = std::min(data.size(), max_bytes);
    for (size_t i = 0; i < len; ++i) {
        char c = static_cast<char>(data[i]);
        oss << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
    }
    return oss.str();
}

} // namespace usb_transfer
