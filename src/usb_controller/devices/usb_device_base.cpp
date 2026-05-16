// ============================================================================
// usb_device_base.cpp - USB 设备基类（实现文件）
//
// 功能说明：
//   实现 UsbDeviceBase 的所有方法，将调用委托给 usb_ctrl::UsbController。
// ============================================================================

#include "usb_device_base.hpp"
#include "console.h"

// 构造函数
UsbDeviceBase::UsbDeviceBase(const UsbDeviceInfo& info)
    : m_info(info)
    , m_ctrl(std::make_unique<usb_ctrl::UsbController>()) // 创建 USB 控制器
{
}

// 析构函数
UsbDeviceBase::~UsbDeviceBase() {
    close(); // 确保设备关闭
}

// 打开设备
bool UsbDeviceBase::open() {
    if (!m_ctrl) return false;
    // 刷新设备列表
    m_ctrl->refresh_devices();
    // 按 VID/PID 打开 HID 设备
    bool ok = m_ctrl->open_hid_device_by_vid_pid(m_info.vid, m_info.pid);
    if (!ok) {
        Console::out() << "UsbDeviceBase: open failed, VID=0x"
                       << std::hex << m_info.vid << " PID=0x" << m_info.pid << std::dec << std::endl;
        return false;
    }
    // 调用子类的额外初始化
    if (!on_opened()) {
        m_ctrl->close_hid_device();
        return false;
    }
    Console::out() << "UsbDeviceBase: opened " << m_info.to_string() << std::endl;
    return true;
}

// 关闭设备
void UsbDeviceBase::close() {
    if (m_ctrl && m_ctrl->is_hid_open()) {
        on_closing();       // 子类清理
        stop_read();        // 停止异步读取
        m_ctrl->close_hid_device(); // 关闭 HID 设备
        Console::out() << "UsbDeviceBase: closed " << m_info.to_string() << std::endl;
    }
}

// 检查设备是否已打开
bool UsbDeviceBase::is_open() const {
    return m_ctrl && m_ctrl->is_hid_open();
}

// 获取设备信息
const UsbDeviceInfo& UsbDeviceBase::info() const {
    return m_info;
}

// 同步写入数据
bool UsbDeviceBase::write(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!is_open()) return false;
    auto result = m_ctrl->hid_write(data, timeout_ms);
    if (!result.success) {
        Console::out() << "UsbDeviceBase: write failed - " << result.error_message << std::endl;
    }
    return result.success;
}

// 同步读取数据
std::vector<uint8_t> UsbDeviceBase::read(int length, unsigned int timeout_ms) {
    if (!is_open()) return {};
    auto result = m_ctrl->hid_read(length, timeout_ms);
    if (!result.success) {
        Console::out() << "UsbDeviceBase: read failed - " << result.error_message << std::endl;
        return {};
    }
    return result.data;
}

// 启动连续异步读取
void UsbDeviceBase::start_read(int length, DataCallback cb, unsigned int timeout_ms) {
    if (!is_open()) return;
    m_read_cb = std::move(cb); // 保存回调
    // 启动异步引擎
    m_ctrl->async_start();
    // 启动连续读取，内部回调转发给用户回调
    m_ctrl->hid_read_continuous(length,
        [this](const usb_ctrl::io::TransferResult& result) {
            if (result.success && m_read_cb) {
                m_read_cb(result.data); // 转发数据给用户回调
            }
        },
        timeout_ms);
}

// 停止连续异步读取
void UsbDeviceBase::stop_read() {
    if (m_ctrl) {
        m_ctrl->hid_stop_continuous(); // 停止连续读取
    }
}

// 异步写入数据
void UsbDeviceBase::write_async(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!is_open()) return;
    m_ctrl->hid_write_async(data,
        [](const usb_ctrl::io::TransferResult& result) {
            if (!result.success) {
                Console::out() << "UsbDeviceBase: async write failed - " << result.error_message << std::endl;
            }
        },
        timeout_ms);
}
