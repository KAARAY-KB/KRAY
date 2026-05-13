// ============================================================================
// usb_transfer.hpp - USB 同步数据传输模块（头文件）
//
// 功能说明：
//   本文件定义了同步 USB 数据传输相关的数据结构和类：
//   - TransferResult：传输结果结构体，封装传输状态和数据
//   - SyncTransfer：同步传输类，封装 libusb 的同步批量/中断/控制传输
//   - 工具函数：bytes_to_hex、bytes_to_ascii、format_transfer_result
//
// 同步传输 vs 异步传输：
//   同步传输会阻塞调用线程直到传输完成或超时，适合简单场景。
//   异步传输通过回调函数通知结果，适合高性能/连续传输场景。
//
// 依赖关系：
//   - libusb.h：底层 USB 传输 API
//   - <vector>：数据缓冲区
//   - <cstdint>：定长整数类型
//   - <string>：字符串支持
// ============================================================================

#pragma once

#include <vector>
#include <cstdint>
#include <string>

struct libusb_device_handle;

namespace usb_ctrl {
namespace transfer {

// ============================================================================
// TransferResult - USB 传输结果结构体
//
// 封装一次 USB 传输操作的完整结果，包括：
//   - 成功/失败状态
//   - 实际传输的字节数
//   - 错误码和错误消息
//   - 接收到的数据（仅对读操作有效）
// ============================================================================
struct TransferResult {
    bool success = false;              // 传输是否成功
    int bytes_transferred = 0;         // 实际传输的字节数
    int error_code = 0;                // libusb 错误码（成功时为 0）
    std::string error_message;         // 错误描述信息（成功时为空）
    std::vector<uint8_t> data;         // 传输的数据（读操作时包含接收数据）
};

// ============================================================================
// SyncTransfer - USB 同步数据传输类
//
// 封装 libusb 的同步传输 API，提供类型安全的 C++ 接口。
// 支持四种传输类型：
//   - Bulk Read/Write      ：批量传输（用于大容量数据，如 U 盘）
//   - Interrupt Read/Write ：中断传输（用于 HID 设备，如键盘鼠标）
//   - Control Transfer     ：控制传输（用于设备配置和 HID 特性报告）
//
// 所有方法都是同步的，会阻塞调用线程直到传输完成或超时。
// ============================================================================
class SyncTransfer {
public:
    // 构造函数：绑定到指定的设备句柄
    // @param handle 已打开的 libusb 设备句柄（不能为空）
    explicit SyncTransfer(libusb_device_handle* handle);

    // 禁止拷贝（设备句柄所有权唯一）
    SyncTransfer(const SyncTransfer&) = delete;
    SyncTransfer& operator=(const SyncTransfer&) = delete;

    // 获取底层设备句柄
    // @return libusb 设备句柄指针
    libusb_device_handle* handle() const { return _handle; }

    // 批量读取（Bulk IN）
    // 用于从设备批量端点读取数据，如 U 盘的数据读取。
    // @param endpoint   端点地址（如 0x81 表示端点 1 IN）
    // @param length     要读取的字节数
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含读取结果和数据
    TransferResult bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);

    // 批量写入（Bulk OUT）
    // 用于向设备批量端点写入数据，如 U 盘的数据写入。
    // @param endpoint   端点地址（如 0x01 表示端点 1 OUT）
    // @param data       要写入的数据
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含写入结果
    TransferResult bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // 中断读取（Interrupt IN）
    // 用于从 HID 设备读取报告数据，如键盘按键、鼠标移动。
    // @param endpoint   端点地址
    // @param length     要读取的字节数
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含读取结果和数据
    TransferResult interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);

    // 中断写入（Interrupt OUT）
    // 用于向 HID 设备发送报告数据，如设置键盘 LED 状态。
    // @param endpoint   端点地址
    // @param data       要写入的数据
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含写入结果
    TransferResult interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // 控制传输（带数据输出）
    // 用于发送 USB 控制请求，如 HID Set_Report。
    // @param bmRequestType 请求类型字段（方向+类型+接收者）
    // @param bRequest      具体请求代码
    // @param wValue        请求值（2 字节）
    // @param wIndex        请求索引（2 字节）
    // @param data          要发送的数据
    // @param timeout_ms    超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含传输结果
    TransferResult control_transfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                                     const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // 控制读取（带数据输入）
    // 用于读取 USB 控制请求的响应数据，如 HID Get_Report。
    // @param bmRequestType 请求类型字段
    // @param bRequest      具体请求代码
    // @param wValue        请求值（2 字节）
    // @param wIndex        请求索引（2 字节）
    // @param wLength       期望读取的数据长度
    // @param timeout_ms    超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含读取结果和数据
    TransferResult control_read(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                                 uint16_t wLength, unsigned int timeout_ms = 1000);

private:
    libusb_device_handle* _handle; // libusb 设备句柄（不拥有所有权）
};

// ============================================================================
// 工具函数：将字节数组转换为十六进制字符串
//
// 输出格式示例：
//   48 65 6C 6C 6F 20 57 6F 72 6C 64 21
//
// @param data      字节数组
// @param max_bytes 最大显示字节数（超过部分显示 "... (N bytes total)"）
// @return 十六进制格式的字符串
// ============================================================================
std::string bytes_to_hex(const std::vector<uint8_t>& data, size_t max_bytes = 64);

// ============================================================================
// 工具函数：将字节数组转换为 ASCII 字符串
//
// 不可打印字符显示为 '.'。
//
// @param data      字节数组
// @param max_bytes 最大显示字节数
// @return ASCII 格式的字符串
// ============================================================================
std::string bytes_to_ascii(const std::vector<uint8_t>& data, size_t max_bytes = 64);

// ============================================================================
// 工具函数：格式化传输结果为可读字符串
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
std::string format_transfer_result(const TransferResult& r, size_t max_hex = 64);

} // namespace transfer
} // namespace usb_ctrl
