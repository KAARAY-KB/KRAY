// ============================================================================
// usb_device_manager.cpp - USB 设备管理器模块（实现文件）
//
// 功能说明：
//   实现 UsbDeviceManager 类的所有方法，包括：
//   - 设备列表的枚举与刷新（通过 libusb_get_device_list）
//   - 设备列表的多种格式化输出（摘要/详情/树形/HID 过滤）
//   - 设备查找功能（按 VID:PID、按类代码、按 HID 类型）
// ============================================================================

#include "usb_device_manager.hpp" // 自身头文件
#include "usb_context.hpp"       // UsbContext 类（需要完整定义）
#include "libusb.h"              // libusb 库 API
#include <sstream>               // std::ostringstream 字符串流

namespace usb_ctrl {
namespace core {

// ============================================================================
// UsbDeviceManager 构造函数
//
// 保存 USB 上下文引用，并立即调用 refresh() 枚举设备。
// ============================================================================
UsbDeviceManager::UsbDeviceManager(UsbContext& ctx) : _ctx(ctx) {
    refresh(); // 构造时自动枚举设备
}

// ============================================================================
// _raw_to_list - 将 libusb 原始设备列表转换为 UsbDevice 对象列表
//
// libusb_get_device_list() 返回的是 libusb_device* 数组，
// 本方法将其包装为 UsbDevice 对象，利用 RAII 管理设备生命周期。
//
// @param list  libusb 原始设备指针数组
// @param count 设备数量
// @return UsbDevice 对象列表
// ============================================================================
std::vector<UsbDevice> UsbDeviceManager::_raw_to_list(libusb_device** list, ssize_t count) {
    std::vector<UsbDevice> result; // 创建结果列表
    result.reserve(static_cast<size_t>(count)); // 预分配内存，避免多次扩容
    // 遍历原始设备列表，逐个创建 UsbDevice 对象
    for (ssize_t i = 0; i < count; ++i)
        result.emplace_back(list[i]); // 使用 emplace_back 就地构造 UsbDevice
    return result;
}

// ============================================================================
// refresh - 刷新设备列表
//
// 调用 libusb_get_device_list() 重新枚举系统所有 USB 设备。
// 先清空现有列表，再获取新列表。获取失败时抛出 UsbException。
// 注意：libusb_free_device_list 的第二个参数为 1 表示同时减少所有设备的引用计数。
// ============================================================================
void UsbDeviceManager::refresh() {
    _devices.clear(); // 清空现有设备列表
    libusb_device** raw_list = nullptr; // libusb 原始设备列表指针
    // 获取系统所有 USB 设备的列表
    ssize_t count = libusb_get_device_list(_ctx.handle(), &raw_list);
    // 检查返回值：负数表示获取失败
    if (count < 0) {
        throw UsbException(
            std::string("libusb_get_device_list: ") + libusb_error_name(static_cast<int>(count)),
            static_cast<int>(count));
    }
    // 将原始设备列表转换为 UsbDevice 对象列表
    _devices = _raw_to_list(raw_list, count);
    // 释放 libusb 原始设备列表（参数 1 表示同时 unref 所有设备）
    libusb_free_device_list(raw_list, 1);
}

// ============================================================================
// count - 获取当前设备数量
// ============================================================================
size_t UsbDeviceManager::count() const { return _devices.size(); }

// ============================================================================
// list_summary - 获取设备简要摘要列表
//
// 每个设备一行，包含索引、VID/PID、速度、产品名等信息。
// ============================================================================
std::string UsbDeviceManager::list_summary() const {
    std::ostringstream oss;
    oss << "Total USB devices: " << _devices.size() << "\n";
    oss << "--------------------------------------\n";
    // 遍历所有设备，输出摘要信息
    for (size_t i = 0; i < _devices.size(); ++i)
        oss << "[" << i << "] " << _devices[i].summary() << "\n"; // 索引 + 摘要
    return oss.str();
}

// ============================================================================
// list_detail - 获取设备详细列表
//
// 每个设备包含完整的描述符信息（设备/配置/接口/端点）。
// ============================================================================
std::string UsbDeviceManager::list_detail() const {
    std::ostringstream oss;
    oss << "Total USB devices: " << _devices.size() << "\n";
    // 遍历所有设备，输出摘要和详细信息
    for (size_t i = 0; i < _devices.size(); ++i) {
        oss << "[" << i << "] " << _devices[i].summary() << "\n"; // 摘要行
        oss << _devices[i].detail() << "\n\n";                     // 详细描述符
    }
    return oss.str();
}

// ============================================================================
// list_tree - 获取设备树形结构列表
//
// 以层级缩进格式显示设备→配置→接口→端点的树形结构。
// ============================================================================
std::string UsbDeviceManager::list_tree() const {
    std::ostringstream oss;
    oss << "USB Device Tree (" << _devices.size() << " devices):\n";
    oss << "========================================\n";
    // 遍历所有设备，输出树形结构
    for (size_t i = 0; i < _devices.size(); ++i)
        oss << "[" << i << "] " << _devices[i].descriptor_tree() << "\n";
    return oss.str();
}

// ============================================================================
// list_hid_summary - 获取 HID 设备摘要列表
//
// 仅显示类代码为 HID (0x03) 的设备。
// ============================================================================
std::string UsbDeviceManager::list_hid_summary() const {
    std::ostringstream oss;
    int hid_count = 0; // HID 设备计数器
    // 遍历所有设备，筛选 HID 设备
    for (size_t i = 0; i < _devices.size(); ++i) {
        if (_devices[i].has_hid_interface()) { // 检查是否有 HID 接口
            oss << "[" << i << "] " << _devices[i].summary() << "\n";
            ++hid_count; // 计数加 1
        }
    }
    // 输出统计信息
    if (hid_count == 0)
        oss << "No HID devices found.\n"; // 未找到 HID 设备
    else
        oss << "Total HID devices: " << hid_count << "\n"; // HID 设备总数
    return oss.str();
}

// ============================================================================
// find_by_vid_pid - 按 VID 和 PID 查找设备
//
// 遍历设备列表，匹配厂商 ID 和产品 ID。
// 返回的是独立的 UsbDevice 副本（通过 handle() 构造新对象）。
//
// @param vid 厂商 ID
// @param pid 产品 ID
// @return 匹配的设备列表
// ============================================================================
std::vector<UsbDevice> UsbDeviceManager::find_by_vid_pid(uint16_t vid, uint16_t pid) {
    std::vector<UsbDevice> result; // 结果列表
    // 遍历所有设备
    for (auto& dev : _devices)
        // 匹配 VID 和 PID
        if (dev.vid() == vid && dev.pid() == pid)
            result.emplace_back(dev.handle()); // 通过原始指针创建新 UsbDevice
    return result;
}

// ============================================================================
// find_hid_devices - 查找所有 HID 类设备
//
// 遍历设备列表，筛选包含 HID 接口的设备。
// @return HID 设备列表
// ============================================================================
std::vector<UsbDevice> UsbDeviceManager::find_hid_devices() {
    std::vector<UsbDevice> result; // 结果列表
    // 遍历所有设备
    for (auto& dev : _devices)
        // 检查是否有 HID 接口
        if (dev.has_hid_interface())
            result.emplace_back(dev.handle()); // 通过原始指针创建新 UsbDevice
    return result;
}

// ============================================================================
// find_by_class - 按设备类代码查找设备
//
// 注意：返回的是指针列表，指向内部 _devices 中的设备。
// 调用者不应释放这些指针，也不应在指针使用期间调用 refresh()。
//
// @param cls USB 设备类代码
// @return 匹配的设备指针列表
// ============================================================================
std::vector<UsbDevice*> UsbDeviceManager::find_by_class(uint8_t cls) {
    std::vector<UsbDevice*> result; // 结果列表
    // 遍历所有设备
    for (auto& dev : _devices) {
        auto info = dev.get_info(); // 获取设备信息
        // 匹配设备类代码
        if (info.device_class == cls) result.push_back(&dev);
    }
    return result;
}

} // namespace core
} // namespace usb_ctrl
