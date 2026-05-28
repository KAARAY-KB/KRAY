// ============================================================================
// usb_device_base.cpp - USB 设备基类（实现文件）
//
// 功能说明：
//   实现 UsbDeviceBase 的所有方法，将调用委托给 usb_ctrl::UsbController。
//   每个关键操作步骤都添加了详细日志，方便排查问题。
// ============================================================================

#include "usb_device_base.hpp"
#include "console.h"

static const char *_dn = {"UsbDeviceBase"};
// 构造函数
UsbDeviceBase::UsbDeviceBase(const UsbDeviceInfo& info)
    : m_info(info)
    , m_ctrl(std::make_unique<usb_ctrl::UsbController>()) // 创建 USB 控制器
{
    Console::info(_dn) << " construct: device=" << m_info.to_string() << std::endl;
}

// 析构函数
UsbDeviceBase::~UsbDeviceBase() {
    Console::info(_dn) << " destruct: device=" << m_info.to_string() << std::endl;
    close(); // 确保设备关闭
}

// 打开设备
bool UsbDeviceBase::open() {
    Console::info(_dn) << " open: start, device=" << m_info.to_string() << std::endl;
    if (!m_ctrl) {
        Console::error(_dn) << " open: FAILED, controller is null" << std::endl;
        return false;
    }
    // 刷新设备列表
    Console::info(_dn) << " open: refreshing device list..." << std::endl;
    m_ctrl->refresh_devices();
    Console::info(_dn) << " open: device count=" << m_ctrl->device_count() << std::endl;
    // 按 VID/PID 打开 HID 设备
    Console::info(_dn) << " open: opening HID device VID=0x"
                   << std::hex << m_info.di.vendor_id << " PID=0x" << m_info.di.product_id << std::dec << std::endl;
    bool ok = m_ctrl->open_hid_device_by_vid_pid(m_info.di.vendor_id, m_info.di.product_id);
    if (!ok) {
        Console::error(_dn) << " open: FAILED, HID open failed, VID=0x"
                       << std::hex << m_info.di.vendor_id << " PID=0x" << m_info.di.product_id << std::dec << std::endl;
        return false;
    }
    Console::info(_dn) << " open: HID device opened successfully" << std::endl;
    // 调用子类的额外初始化
    Console::info(_dn) << " open: calling on_opened()..." << std::endl;
    if (!on_opened()) {
        Console::error(_dn) << " open: FAILED, on_opened() returned false" << std::endl;
        m_ctrl->close_hid_device();
        return false;
    }
    Console::info(_dn) << " open: SUCCESS, device=" << m_info.to_string() << std::endl;
    return true;
}

// 关闭设备
void UsbDeviceBase::close() {
    Console::info(_dn) << " close: start, is_hid_open=" << (m_ctrl && m_ctrl->is_hid_open()) << std::endl;
    if (m_ctrl && m_ctrl->is_hid_open()) {
        Console::info(_dn) << " close: calling on_closing()..." << std::endl;
        on_closing();       // 子类清理
        Console::info(_dn) << " close: stopping async read..." << std::endl;
        stop_read();        // 停止异步读取
        Console::info(_dn) << " close: closing HID device..." << std::endl;
        m_ctrl->close_hid_device(); // 关闭 HID 设备
        Console::info(_dn) << " close: SUCCESS, device=" << m_info.to_string() << std::endl;
    } else {
        Console::error(_dn) << " close: device not open, skip" << std::endl;
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
    Console::info(_dn) << " write: size=" << data.size() << " timeout=" << timeout_ms << "ms" << std::endl;
    if (!is_open()) {
        Console::error(_dn) << " write: FAILED, device not open" << std::endl;
        return false;
    }
    auto result = m_ctrl->hid_write(data, timeout_ms);
    if (!result.success) {
        Console::error(_dn) << " write: FAILED, error=" << result.error_message << std::endl;
    } else {
        Console::info(_dn) << " write: SUCCESS, transferred=" << result.data.size() << " bytes" << std::endl;
    }
    return result.success;
}

// 同步读取数据
std::vector<uint8_t> UsbDeviceBase::read(int length, unsigned int timeout_ms) {
    Console::info(_dn) << " read: length=" << length << " timeout=" << timeout_ms << "ms" << std::endl;
    if (!is_open()) {
        Console::error(_dn) << " read: FAILED, device not open" << std::endl;
        return {};
    }
    auto result = m_ctrl->hid_read(length, timeout_ms);
    if (!result.success) {
        Console::error(_dn) << " read: FAILED, error=" << result.error_message << std::endl;
        return {};
    }
    Console::info(_dn) << " read: SUCCESS, received=" << result.data.size() << " bytes" << std::endl;
    return result.data;
}

// 启动连续异步读取
void UsbDeviceBase::start_read(int length, DataCallback cb, unsigned int timeout_ms) {
    Console::info(_dn) << " start_read: length=" << length << " timeout=" << timeout_ms << "ms" << std::endl;
    if (!is_open()) {
        Console::error(_dn) << " start_read: FAILED, device not open" << std::endl;
        return;
    }
    m_read_cb = std::move(cb); // 保存回调
    // 启动异步引擎
    Console::info(_dn) << " start_read: starting async engine..." << std::endl;
    m_ctrl->async_start();
    // 启动连续读取，内部回调转发给用户回调
    Console::info(_dn) << " start_read: starting continuous read..." << std::endl;
    m_ctrl->hid_read_continuous(length,
        [this](const usb_ctrl::io::TransferResult& result) {
            if (result.success && m_read_cb) {
                m_read_cb(result.data); // 转发数据给用户回调
            } else if (!result.success) {
                Console::error(_dn) << " start_read: async read error=" << result.error_message << std::endl;
            }
        },
        timeout_ms);
    Console::info(_dn) << " start_read: continuous read started" << std::endl;
}

// 停止连续异步读取
void UsbDeviceBase::stop_read() {
    Console::info(_dn) << " stop_read: stopping continuous read..." << std::endl;
    if (m_ctrl) {
        m_ctrl->hid_stop_continuous(); // 停止连续读取
        Console::info(_dn) << " stop_read: continuous read stopped" << std::endl;
    } else {
        Console::error(_dn) << " stop_read: controller is null, skip" << std::endl;
    }
}

// 异步写入数据
void UsbDeviceBase::write_async(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    Console::info(_dn) << " write_async: size=" << data.size() << " timeout=" << timeout_ms << "ms" << std::endl;
    if (!is_open()) {
        Console::error(_dn) << " write_async: FAILED, device not open" << std::endl;
        return;
    }
    m_ctrl->hid_write_async(data,
        [](const usb_ctrl::io::TransferResult& result) {
            if (!result.success) {
                Console::error(_dn) << " write_async: FAILED, error=" << result.error_message << std::endl;
            } else {
                Console::info(_dn) << " write_async: SUCCESS, transferred=" << result.data.size() << " bytes" << std::endl;
            }
        },
        timeout_ms);
}
