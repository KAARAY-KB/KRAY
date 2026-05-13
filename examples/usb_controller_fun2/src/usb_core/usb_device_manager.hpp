#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>

// 前向声明 libusb 设备结构体
struct libusb_device;

namespace usb_core {

class UsbContext;
class UsbDevice;

// USB 设备管理器类
// 负责枚举和管理系统中所有 USB 设备
// 提供设备列表刷新、查找和过滤功能
class UsbDeviceManager {
public:
    explicit UsbDeviceManager(UsbContext& ctx);

    // 禁止拷贝
    UsbDeviceManager(const UsbDeviceManager&) = delete;
    UsbDeviceManager& operator=(const UsbDeviceManager&) = delete;

    // 刷新设备列表 (重新枚举所有 USB 设备)
    void refresh();

    // 获取设备总数
    size_t count() const;

    // 获取所有设备的摘要信息列表
    std::string list_all_summary() const;

    // 获取所有设备的详细信息列表
    std::string list_all_detail() const;

    // 获取所有 HID 设备的摘要信息列表
    std::string list_hid_summary() const;

    // 根据 VID/PID 查找设备
    std::vector<UsbDevice> find_by_vid_pid(uint16_t vid, uint16_t pid);

    // 查找所有 HID 设备
    std::vector<UsbDevice> find_hid_devices();

    // 获取设备列表引用
    std::vector<UsbDevice>& devices() { return _devices; }
    const std::vector<UsbDevice>& devices() const { return _devices; }

private:
    UsbContext& _ctx;
    std::vector<UsbDevice> _devices;

    // 内部辅助函数：将 libusb 设备数组转换为 UsbDevice 列表
    std::vector<UsbDevice> _raw_to_list(libusb_device** list, ssize_t count);
};

} // namespace usb_core
