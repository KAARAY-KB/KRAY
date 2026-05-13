#include "usb_device_manager.hpp"
#include "usb_context.hpp"
#include "usb_device.hpp"
#include "libusb.h"

#include <sstream>

namespace usb_core {

// UsbDeviceManager 构造函数：初始化设备管理器并刷新设备列表
UsbDeviceManager::UsbDeviceManager(UsbContext& ctx)
    : _ctx(ctx) {
    refresh();
}

// 将 libusb 原始设备指针数组转换为 UsbDevice 对象列表
std::vector<UsbDevice> UsbDeviceManager::_raw_to_list(libusb_device** list, ssize_t count) {
    std::vector<UsbDevice> result;
    result.reserve(static_cast<size_t>(count));
    for (ssize_t i = 0; i < count; ++i) {
        result.emplace_back(list[i]);
    }
    return result;
}

// 刷新设备列表
// 调用 libusb_get_device_list 枚举所有 USB 设备
// 释放旧列表并创建新的 UsbDevice 对象列表
void UsbDeviceManager::refresh() {
    _devices.clear();
    libusb_device** raw_list = nullptr;
    ssize_t count = libusb_get_device_list(_ctx.handle(), &raw_list);
    if (count < 0) {
        throw UsbException("libusb_get_device_list failed: " +
            std::string(libusb_error_name(static_cast<int>(count))),
            static_cast<int>(count));
    }
    _devices = _raw_to_list(raw_list, count);
    libusb_free_device_list(raw_list, 1);
}

// 获取当前设备列表中的设备数量
size_t UsbDeviceManager::count() const {
    return _devices.size();
}

// 获取所有设备的摘要信息
// 格式: [索引] VID:0xXXXX PID:0xXXXX Speed:xxx [Product] (Manufacturer)
std::string UsbDeviceManager::list_all_summary() const {
    std::ostringstream oss;
    oss << "Total USB devices: " << _devices.size() << "\n";
    oss << "------------------------------\n";
    for (size_t i = 0; i < _devices.size(); ++i) {
        oss << "[" << i << "] " << _devices[i].summary() << "\n";
    }
    return oss.str();
}

// 获取所有设备的详细信息
// 包含每个设备的摘要和完整描述符信息
std::string UsbDeviceManager::list_all_detail() const {
    std::ostringstream oss;
    oss << "Total USB devices: " << _devices.size() << "\n";
    oss << "==============================\n";
    for (size_t i = 0; i < _devices.size(); ++i) {
        oss << "[" << i << "] " << _devices[i].summary() << "\n";
        oss << _devices[i].detail() << "\n";
        oss << "==============================\n";
    }
    return oss.str();
}

// 获取所有 HID 设备的摘要信息
// 遍历设备列表，过滤出包含 HID 接口的设备
std::string UsbDeviceManager::list_hid_summary() const {
    std::ostringstream oss;
    int hid_count = 0;
    for (size_t i = 0; i < _devices.size(); ++i) {
        if (_devices[i].has_hid_interface()) {
            oss << "[" << i << "] " << _devices[i].summary() << "\n";
            ++hid_count;
        }
    }
    if (hid_count == 0) {
        oss << "No HID devices found.\n";
    } else {
        oss << "Total HID devices: " << hid_count << "\n";
    }
    return oss.str();
}

// 根据厂商 ID (VID) 和产品 ID (PID) 查找设备
// 返回所有匹配的设备列表
std::vector<UsbDevice> UsbDeviceManager::find_by_vid_pid(uint16_t vid, uint16_t pid) {
    std::vector<UsbDevice> result;
    for (auto& dev : _devices) {
        if (dev.vid() == vid && dev.pid() == pid) {
            result.emplace_back(dev.handle());
        }
    }
    return result;
}

// 查找所有 HID 设备
// 遍历设备列表，返回所有包含 HID 接口的设备
std::vector<UsbDevice> UsbDeviceManager::find_hid_devices() {
    std::vector<UsbDevice> result;
    for (auto& dev : _devices) {
        if (dev.has_hid_interface()) {
            result.emplace_back(dev.handle());
        }
    }
    return result;
}

} // namespace usb_core
