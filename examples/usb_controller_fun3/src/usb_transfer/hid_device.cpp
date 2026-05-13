// ============================================================================
// hid_device.cpp - HID 设备操作模块（实现文件）
//
// 功能说明：
//   实现 HidDevice 类的所有方法，包括：
//   - HID 设备的打开与关闭（内核驱动分离/附加）
//   - HID 输入/输出报告的读写（通过中断端点）
//   - HID 特性报告的获取与发送（通过控制传输）
//   - 设备摘要信息的格式化输出
//
// HID 设备打开流程：
//   1. 检测设备是否有 HID 接口
//   2. 查找 HID 接口的 IN/OUT 中断端点
//   3. 调用 libusb_open() 打开设备
//   4. 检测并分离内核驱动（如 Linux hid-generic）
//   5. 调用 libusb_claim_interface() 声明接口
//   6. 创建 SyncTransfer 对象用于后续数据传输
//
// 关闭时按相反顺序释放资源，并重新附加内核驱动。
// ============================================================================

#include "hid_device.hpp"
#include "libusb.h"
#include <sstream>
#include <iomanip>

namespace usb_ctrl {
namespace transfer {

// ============================================================================
// HidDevice 构造函数
//
// 保存 USB 设备引用。注意：构造时不打开设备，需要调用 open() 方法。
// _device 是引用类型，调用者需确保设备在 HidDevice 使用期间保持有效。
//
// @param device USB 设备引用（来自 UsbDeviceManager 的设备列表）
// ============================================================================
HidDevice::HidDevice(core::UsbDevice& device) : _device(device) {}

// ============================================================================
// HidDevice 析构函数
//
// 自动调用 close() 释放所有资源，确保设备被正确关闭。
// ============================================================================
HidDevice::~HidDevice() { close(); }

// ============================================================================
// open - 打开 HID 设备
//
// 执行完整的 HID 设备打开流程：
//   1. 如果已打开则直接返回 true（幂等操作）
//   2. 检查设备是否有 HID 接口
//   3. 查找 HID 接口的 IN/OUT 中断端点
//   4. 调用 libusb_open() 获取设备句柄
//   5. 检测内核驱动是否占用，如果是则分离（detach）
//   6. 调用 libusb_claim_interface() 声明接口使用权
//   7. 创建 SyncTransfer 对象用于数据传输
//
// 任何步骤失败都会清理已分配的资源并返回 false。
//
// @return true 表示打开成功，false 表示失败
// ============================================================================
bool HidDevice::open() {
    // 如果已经打开，直接返回成功（幂等操作）
    if (_handle) return true;
    // 检查设备是否有 HID 接口（类代码为 0x03）
    if (!_device.has_hid_interface()) return false;
    // 查找 HID 接口的 IN/OUT 中断端点地址
    if (!_device.find_hid_interface(_iface_num, _in_ep, _out_ep)) return false;

    // 调用 libusb_open() 打开设备，获取操作句柄
    int rc = libusb_open(_device.handle(), &_handle);
    // 打开失败：将句柄置空并返回 false
    if (rc < 0) { _handle = nullptr; return false; }

    // 检查内核驱动是否占用了该接口（Linux 上常见 hid-generic 驱动）
    if (libusb_kernel_driver_active(_handle, _iface_num) == 1) {
        // 尝试分离内核驱动，以便用户空间程序访问
        rc = libusb_detach_kernel_driver(_handle, _iface_num);
        // 记录分离状态，关闭时需要重新附加
        if (rc == 0) _kernel_detached = true;
    }

    // 声明接口使用权（确保独占访问）
    rc = libusb_claim_interface(_handle, _iface_num);
    // 声明失败：关闭设备句柄并返回 false
    if (rc < 0) { libusb_close(_handle); _handle = nullptr; return false; }

    // 创建同步传输对象，用于后续的数据读写操作
    _transfer = std::make_unique<SyncTransfer>(_handle);
    return true;
}

// ============================================================================
// close - 关闭 HID 设备
//
// 按打开顺序的逆序释放资源：
//   1. 释放 SyncTransfer 对象
//   2. 调用 libusb_release_interface() 释放接口
//   3. 如果之前分离了内核驱动，重新附加（attach）
//   4. 调用 libusb_close() 关闭设备句柄
//
// 如果设备未打开（_handle 为空），此方法不执行任何操作。
// ============================================================================
void HidDevice::close() {
    // 仅当设备已打开时才执行关闭操作
    if (_handle) {
        // 释放同步传输对象（先于句柄关闭）
        _transfer.reset();
        // 释放接口使用权
        libusb_release_interface(_handle, _iface_num);
        // 如果之前分离了内核驱动，重新附加
        if (_kernel_detached) {
            libusb_attach_kernel_driver(_handle, _iface_num);
            _kernel_detached = false; // 重置标志
        }
        // 关闭设备句柄
        libusb_close(_handle);
        _handle = nullptr; // 置空指针，防止重复关闭
    }
}

// ============================================================================
// read_report - 读取 HID 输入报告（通过中断 IN 端点）
//
// 从 HID 设备的中断 IN 端点读取数据。
// 这是 HID 设备最常用的操作，用于读取键盘按键、鼠标移动等输入数据。
//
// @param length     要读取的最大字节数
// @param timeout_ms 超时时间（毫秒），默认 1000ms
// @return TransferResult 包含读取结果和数据
// ============================================================================
TransferResult HidDevice::read_report(int length, unsigned int timeout_ms) {
    // 检查设备是否已打开
    if (!_transfer) return {false, 0, 0, "Device not opened", {}};
    // 委托给 SyncTransfer 执行中断读取
    return _transfer->interrupt_read(_in_ep.address, length, timeout_ms);
}

// ============================================================================
// write_report - 写入 HID 输出报告（通过中断 OUT 端点）
//
// 向 HID 设备的中断 OUT 端点写入数据。
// 用于向 HID 设备发送输出报告，如设置键盘 LED 状态、发送力反馈指令等。
//
// @param data       要写入的数据（输出报告内容）
// @param timeout_ms 超时时间（毫秒），默认 1000ms
// @return TransferResult 包含写入结果
// ============================================================================
TransferResult HidDevice::write_report(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    // 检查设备是否已打开
    if (!_transfer) return {false, 0, 0, "Device not opened", {}};
    // 委托给 SyncTransfer 执行中断写入
    return _transfer->interrupt_write(_out_ep.address, data, timeout_ms);
}

// ============================================================================
// send_feature_report - 发送 HID 特性报告（通过控制传输）
//
// 使用 USB 控制传输的 SET_REPORT 请求向 HID 设备发送特性报告。
// 特性报告用于配置设备参数，如设置键盘的重复速率、鼠标的灵敏度等。
//
// USB 控制传输参数说明：
//   - bmRequestType = 0x21（Host-to-Device, Class, Interface）
//   - bRequest = 0x09（SET_REPORT）
//   - wValue = 0x0300 | report_id（Report Type=Feature, Report ID）
//   - wIndex = interface_number
//
// @param data       要发送的特性报告数据（第一个字节为报告 ID）
// @param timeout_ms 超时时间（毫秒），默认 1000ms
// @return TransferResult 包含发送结果
// ============================================================================
TransferResult HidDevice::send_feature_report(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    // 检查设备是否已打开
    if (!_transfer) return {false, 0, 0, "Device not opened", {}};
    // 委托给 SyncTransfer 执行控制传输
    return _transfer->control_transfer(
        // bmRequestType: 主机到设备 | 类请求 | 接口接收者
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09, // bRequest: SET_REPORT（HID 规范定义）
        // wValue: 高字节=0x03（Feature Report），低字节=报告 ID
        static_cast<uint16_t>(0x0300 | data[0]),
        // wIndex: 接口编号
        static_cast<uint16_t>(_iface_num),
        data, timeout_ms);
}

// ============================================================================
// get_feature_report - 获取 HID 特性报告（通过控制传输）
//
// 使用 USB 控制传输的 GET_REPORT 请求从 HID 设备读取特性报告。
// 用于读取设备的配置参数。
//
// USB 控制传输参数说明：
//   - bmRequestType = 0xA1（Device-to-Host, Class, Interface）
//   - bRequest = 0x01（GET_REPORT）
//   - wValue = 0x0300 | report_id（Report Type=Feature, Report ID）
//   - wIndex = interface_number
//
// @param report_id  要获取的报告 ID
// @param length     期望读取的数据长度
// @param timeout_ms 超时时间（毫秒），默认 1000ms
// @return TransferResult 包含读取结果和数据
// ============================================================================
TransferResult HidDevice::get_feature_report(uint8_t report_id, int length, unsigned int timeout_ms) {
    // 检查设备是否已打开
    if (!_transfer) return {false, 0, 0, "Device not opened", {}};
    // 委托给 SyncTransfer 执行控制读取
    return _transfer->control_read(
        // bmRequestType: 设备到主机 | 类请求 | 接口接收者
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        0x01, // bRequest: GET_REPORT（HID 规范定义）
        // wValue: 高字节=0x03（Feature Report），低字节=报告 ID
        static_cast<uint16_t>(0x0300 | report_id),
        // wIndex: 接口编号
        static_cast<uint16_t>(_iface_num),
        // wLength: 期望读取的数据长度
        static_cast<uint16_t>(length), timeout_ms);
}

// ============================================================================
// device_summary - 获取 HID 设备摘要信息
//
// 返回包含接口编号、IN/OUT 端点地址和最大包大小的格式化字符串。
//
// 输出格式示例：
//   HID Device: Interface 0  IN EP:0x81 (max 64B)  OUT EP:0x01 (max 64B)
//
// @return 格式化的设备摘要字符串
// ============================================================================
std::string HidDevice::device_summary() const {
    std::ostringstream oss; // 字符串输出流
    // 格式化输出：接口编号、IN 端点地址和最大包大小、OUT 端点地址和最大包大小
    oss << "HID Device: Interface " << _iface_num
        << "  IN EP:0x" << std::hex << static_cast<int>(_in_ep.address) << std::dec
        << " (max " << _in_ep.max_packet_size << "B)"
        << "  OUT EP:0x" << std::hex << static_cast<int>(_out_ep.address) << std::dec
        << " (max " << _out_ep.max_packet_size << "B)";
    return oss.str();
}

} // namespace transfer
} // namespace usb_ctrl
