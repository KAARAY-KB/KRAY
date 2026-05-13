#include "hid_device.hpp"
#include "usb_core/usb_context.hpp"
#include "libusb.h"

#include <sstream>

namespace usb_transfer {

// HidDevice 构造函数：关联 UsbDevice 对象
HidDevice::HidDevice(usb_core::UsbDevice& device)
    : _device(device) {
}

// HidDevice 析构函数：确保设备被正确关闭
HidDevice::~HidDevice() {
    close();
}

// 打开 HID 设备
// 1. 检查设备是否有 HID 接口
// 2. 查找 HID 接口编号和端点
// 3. 打开设备句柄
// 4. 分离内核驱动 (如果已绑定)
// 5. 声明接口
// 6. 创建同步传输对象
bool HidDevice::open() {
    if (_handle) return true;

    if (!_device.has_hid_interface()) {
        return false;
    }

    if (!_device.find_hid_interface(_interface_number, _in_ep, _out_ep)) {
        return false;
    }

    int rc = libusb_open(_device.handle(), &_handle);
    if (rc < 0) {
        _handle = nullptr;
        return false;
    }

    // 检查并分离内核驱动 (Linux 上可能需要)
    if (libusb_kernel_driver_active(_handle, _interface_number) == 1) {
        rc = libusb_detach_kernel_driver(_handle, _interface_number);
        if (rc == 0) {
            _kernel_detached = true;
        }
    }

    // 声明接口
    rc = libusb_claim_interface(_handle, _interface_number);
    if (rc < 0) {
        libusb_close(_handle);
        _handle = nullptr;
        return false;
    }

    _transfer = std::make_unique<SyncTransfer>(_handle);
    return true;
}

// 关闭 HID 设备
// 1. 释放接口
// 2. 恢复内核驱动 (如果之前分离了)
// 3. 关闭设备句柄
// 4. 销毁传输对象
void HidDevice::close() {
    if (_handle) {
        libusb_release_interface(_handle, _interface_number);
        if (_kernel_detached) {
            libusb_attach_kernel_driver(_handle, _interface_number);
            _kernel_detached = false;
        }
        libusb_close(_handle);
        _handle = nullptr;
    }
    _transfer.reset();
}

// 读取 HID 输入报告
// 通过中断 IN 端点接收数据
// length: 期望读取的字节数
TransferResult HidDevice::read_report(int length, unsigned int timeout_ms) {
    if (!_transfer) {
        TransferResult r;
        r.success = false;
        r.error_message = "Device not opened";
        return r;
    }
    return _transfer->interrupt_read(_in_ep.address, length, timeout_ms);
}

// 写入 HID 输出报告
// 通过中断 OUT 端点发送数据
// data: 要发送的报告数据 (第一个字节通常为报告 ID)
TransferResult HidDevice::write_report(const std::vector<uint8_t>& data,
                                        unsigned int timeout_ms) {
    if (!_transfer) {
        TransferResult r;
        r.success = false;
        r.error_message = "Device not opened";
        return r;
    }
    return _transfer->interrupt_write(_out_ep.address, data, timeout_ms);
}

// 发送 HID 特性报告
// 通过控制传输发送 SET_REPORT 请求
// data: 特性报告数据 (第一个字节为报告 ID)
TransferResult HidDevice::send_feature_report(const std::vector<uint8_t>& data,
                                               unsigned int timeout_ms) {
    if (!_transfer) {
        TransferResult r;
        r.success = false;
        r.error_message = "Device not opened";
        return r;
    }
    return _transfer->control_transfer(
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09, // SET_REPORT (HID class request)
        static_cast<uint16_t>(0x0300 | data[0]), // HID feature report
        static_cast<uint16_t>(_interface_number),
        data,
        timeout_ms);
}

// 获取 HID 特性报告
// 通过控制传输发送 GET_REPORT 请求
// report_id: 报告 ID
// length: 期望读取的字节数
TransferResult HidDevice::get_feature_report(uint8_t report_id, int length,
                                              unsigned int timeout_ms) {
    if (!_transfer) {
        TransferResult r;
        r.success = false;
        r.error_message = "Device not opened";
        return r;
    }
    return _transfer->control_read(
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        0x01, // GET_REPORT (HID class request)
        static_cast<uint16_t>(0x0300 | report_id), // HID feature report
        static_cast<uint16_t>(_interface_number),
        static_cast<uint16_t>(length),
        timeout_ms);
}

// 获取设备摘要信息
// 显示接口编号和端点信息
std::string HidDevice::device_summary() const {
    std::ostringstream oss;
    oss << "HID Device: Interface " << _interface_number
        << "  IN EP:0x" << std::hex << static_cast<int>(_in_ep.address)
        << " (max " << std::dec << _in_ep.max_packet_size << " bytes)"
        << "  OUT EP:0x" << std::hex << static_cast<int>(_out_ep.address)
        << " (max " << std::dec << _out_ep.max_packet_size << " bytes)";
    return oss.str();
}

} // namespace usb_transfer
