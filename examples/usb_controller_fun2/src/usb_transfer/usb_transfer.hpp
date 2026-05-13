#pragma once

#include <vector>
#include <cstdint>
#include <string>

// 前向声明 libusb 设备句柄结构体
struct libusb_device_handle;

namespace usb_transfer {

// 传输结果结构体
// 封装 libusb 传输操作的返回值和传输数据
struct TransferResult {
    bool success = false;                // 传输是否成功
    int bytes_transferred = 0;           // 实际传输的字节数
    int error_code = 0;                  // libusb 错误码 (成功时为 0)
    std::string error_message;           // 错误描述字符串
    std::vector<uint8_t> data;           // 接收到的数据 (读操作) 或发送的数据 (写操作)
};

// 同步传输类
// 封装 libusb 的同步传输 API (阻塞式)
// 支持 Bulk、Interrupt 和 Control 三种传输类型
class SyncTransfer {
public:
    explicit SyncTransfer(libusb_device_handle* handle);

    // 禁止拷贝
    SyncTransfer(const SyncTransfer&) = delete;
    SyncTransfer& operator=(const SyncTransfer&) = delete;

    // 获取底层 libusb 设备句柄
    libusb_device_handle* handle() const { return _handle; }

    // Bulk 读操作 (从设备接收数据)
    TransferResult bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);

    // Bulk 写操作 (向设备发送数据)
    TransferResult bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data,
                               unsigned int timeout_ms = 1000);

    // Interrupt 读操作 (从设备接收中断数据)
    TransferResult interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);

    // Interrupt 写操作 (向设备发送中断数据)
    TransferResult interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data,
                                    unsigned int timeout_ms = 1000);

    // Control 传输操作 (带数据的控制请求)
    TransferResult control_transfer(uint8_t bmRequestType, uint8_t bRequest,
                                     uint16_t wValue, uint16_t wIndex,
                                     const std::vector<uint8_t>& data,
                                     unsigned int timeout_ms = 1000);

    // Control 读操作 (从设备读取控制数据)
    TransferResult control_read(uint8_t bmRequestType, uint8_t bRequest,
                                 uint16_t wValue, uint16_t wIndex,
                                 uint16_t wLength, unsigned int timeout_ms = 1000);

private:
    libusb_device_handle* _handle;
};

// 将字节数组转换为十六进制字符串
std::string bytes_to_hex(const std::vector<uint8_t>& data, size_t max_bytes = 64);

// 将字节数组转换为 ASCII 字符串 (不可打印字符显示为 '.')
std::string bytes_to_ascii(const std::vector<uint8_t>& data, size_t max_bytes = 64);

} // namespace usb_transfer
