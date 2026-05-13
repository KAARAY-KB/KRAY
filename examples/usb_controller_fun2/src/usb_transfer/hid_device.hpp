#pragma once

#include "usb_transfer.hpp"
#include "usb_core/usb_device.hpp"

#include <memory>
#include <string>
#include <cstdint>

// 前向声明 libusb 相关结构体
struct libusb_device;
struct libusb_device_handle;

namespace usb_transfer {

// HID 设备操作类
// 封装 HID 设备的打开、关闭和数据传输操作
// 自动处理内核驱动分离和接口声明
class HidDevice {
public:
    explicit HidDevice(usb_core::UsbDevice& device);

    ~HidDevice();

    // 禁止拷贝
    HidDevice(const HidDevice&) = delete;
    HidDevice& operator=(const HidDevice&) = delete;

    // 打开 HID 设备 (声明接口、分离内核驱动)
    bool open();

    // 关闭 HID 设备 (释放接口、恢复内核驱动)
    void close();

    // 判断设备是否已打开
    bool is_open() const { return _handle != nullptr; }

    // 获取 HID 接口编号
    int interface_number() const { return _interface_number; }

    // 获取 IN 端点信息
    usb_core::UsbEndpointInfo in_endpoint() const { return _in_ep; }

    // 获取 OUT 端点信息
    usb_core::UsbEndpointInfo out_endpoint() const { return _out_ep; }

    // 读取 HID 输入报告 (通过中断端点)
    TransferResult read_report(int length, unsigned int timeout_ms = 1000);

    // 写入 HID 输出报告 (通过中断端点)
    TransferResult write_report(const std::vector<uint8_t>& data,
                                 unsigned int timeout_ms = 1000);

    // 发送 HID 特性报告 (通过控制传输)
    TransferResult send_feature_report(const std::vector<uint8_t>& data,
                                        unsigned int timeout_ms = 1000);

    // 获取 HID 特性报告 (通过控制传输)
    TransferResult get_feature_report(uint8_t report_id, int length,
                                       unsigned int timeout_ms = 1000);

    // 获取设备摘要信息
    std::string device_summary() const;

    // 获取同步传输对象引用
    SyncTransfer& transfer() { return *_transfer; }

private:
    usb_core::UsbDevice& _device;
    libusb_device_handle* _handle = nullptr;
    std::unique_ptr<SyncTransfer> _transfer;
    int _interface_number = -1;
    usb_core::UsbEndpointInfo _in_ep;
    usb_core::UsbEndpointInfo _out_ep;
    bool _kernel_detached = false;
};

} // namespace usb_transfer
