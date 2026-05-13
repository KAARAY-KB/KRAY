// ============================================================================
// hid_device.hpp - HID 设备操作模块（头文件）
//
// 功能说明：
//   本文件定义了 HidDevice 类，专门用于 HID（人机接口设备）的操作。
//   HID 设备包括键盘、鼠标、游戏手柄等，使用中断端点进行数据传输。
//
// HID 设备打开流程：
//   1. 检测设备是否有 HID 接口
//   2. 查找 HID 接口的 IN/OUT 中断端点
//   3. 打开设备（libusb_open）
//   4. 分离内核驱动（如果已被内核占用，如 Linux hid-generic）
//   5. 声明接口（libusb_claim_interface）
//   6. 创建 SyncTransfer 对象用于数据传输
//
// 关闭时按相反顺序释放资源，并重新附加内核驱动。
//
// 依赖关系：
//   - usb_transfer.hpp：SyncTransfer 类和 TransferResult 结构体
//   - usb_device.hpp：UsbDevice 类和 EndpointInfo 结构体
// ============================================================================

#pragma once

#include "usb_transfer.hpp"
#include "usb_core/usb_device.hpp"
#include <memory>
#include <string>

// 前向声明 libusb 设备句柄
struct libusb_device_handle;

namespace usb_ctrl {
namespace transfer {

// ============================================================================
// HidDevice - HID 设备操作类
//
// 封装 HID 设备的打开、关闭和数据传输操作。
// 自动处理内核驱动分离/附加，确保在 Linux 等系统上能正常访问 HID 设备。
//
// 数据传输通过中断端点进行：
//   - read_report()  ：从 IN 端点读取输入报告（如键盘按键）
//   - write_report() ：向 OUT 端点发送输出报告（如设置键盘 LED）
//   - get_feature_report() ：通过控制传输读取特性报告
//   - send_feature_report()：通过控制传输发送特性报告
// ============================================================================
class HidDevice {
public:
    // 构造函数：绑定到指定的 USB 设备
    // 注意：构造时不打开设备，需要调用 open() 方法
    // @param device USB 设备引用
    explicit HidDevice(core::UsbDevice& device);

    // 析构函数：自动关闭设备并释放资源
    ~HidDevice();

    // 禁止拷贝（设备所有权唯一）
    HidDevice(const HidDevice&) = delete;
    HidDevice& operator=(const HidDevice&) = delete;

    // 打开 HID 设备
    // 执行完整的打开流程：检测 HID 接口 → 打开设备 → 分离内核驱动 → 声明接口
    // @return true 表示打开成功，false 表示失败
    bool open();

    // 关闭 HID 设备
    // 按相反顺序释放资源：释放接口 → 重新附加内核驱动 → 关闭设备
    void close();

    // 检查设备是否已打开
    // @return true 表示设备已打开且可用
    bool is_open() const { return _handle != nullptr; }

    // 获取 HID 接口编号
    // @return 接口编号（从 0 开始）
    int interface_number() const { return _iface_num; }

    // 获取 IN 端点信息（设备→主机）
    // @return IN 中断端点信息
    core::EndpointInfo in_endpoint() const { return _in_ep; }

    // 获取 OUT 端点信息（主机→设备）
    // @return OUT 中断端点信息
    core::EndpointInfo out_endpoint() const { return _out_ep; }

    // 读取输入报告（通过中断 IN 端点）
    // 用于读取 HID 设备发送的数据，如键盘按键、鼠标移动。
    // @param length     要读取的最大字节数
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含读取结果和数据
    TransferResult read_report(int length, unsigned int timeout_ms = 1000);

    // 读取最新的输入报告（先排空缓冲区，再等待设备新数据）
//
// 两步策略：
//   1. 以短超时循环读取，排空 libusb 内部已有的缓冲数据
//   2. 以 1100ms 长超时等待设备的下一个数据包，确保拿到最新数据
//
// 适用场景：设备持续发送数据（如每 1 秒发一次），但你只想获取当前最新值。
//
// @param length            要读取的最大字节数
// @param drain_timeout_ms  排空缓冲区时每次读取的超时（毫秒），默认 10ms
// @return TransferResult 包含最新读取结果和数据
TransferResult read_latest(int length, unsigned int drain_timeout_ms = 10);

    // 写入输出报告（通过中断 OUT 端点）
    // 用于向 HID 设备发送数据，如设置键盘 LED 状态。
    // @param data       要写入的数据
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含写入结果
    TransferResult write_report(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // 获取特性报告（通过控制传输）
    // HID Get_Report 请求，用于读取设备的配置/状态信息。
    // @param report_id  报告 ID（0 表示默认报告）
    // @param length     期望读取的数据长度
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含读取结果和数据
    TransferResult get_feature_report(uint8_t report_id, int length, unsigned int timeout_ms = 1000);

    // 发送特性报告（通过控制传输）
    // HID Set_Report 请求，用于设置设备的配置/状态。
    // @param data       要发送的数据（第一个字节为报告 ID）
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    // @return TransferResult 包含发送结果
    TransferResult send_feature_report(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // 获取设备摘要信息
    // @return 包含接口编号和端点信息的字符串
    std::string device_summary() const;

    // 获取同步传输对象引用
    // @return SyncTransfer 引用
    SyncTransfer& transfer() { return *_transfer; }

    // 获取底层 libusb 设备句柄
    // @return libusb 设备句柄指针（未打开时返回 nullptr）
    libusb_device_handle* handle() const { return _handle; }

private:
    core::UsbDevice& _device;                  // USB 设备引用（不拥有所有权）
    libusb_device_handle* _handle = nullptr;   // libusb 设备句柄
    std::unique_ptr<SyncTransfer> _transfer;   // 同步传输对象
    int _iface_num = -1;                       // HID 接口编号
    core::EndpointInfo _in_ep;                 // IN 中断端点信息
    core::EndpointInfo _out_ep;                // OUT 中断端点信息
    bool _kernel_detached = false;             // 是否已分离内核驱动
};

} // namespace transfer
} // namespace usb_ctrl
