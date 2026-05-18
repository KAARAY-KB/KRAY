// ============================================================================
// usb_device_info.hpp - USB 设备信息结构体（头文件）
//
// 功能说明：
//   定义 UsbDeviceInfo 结构体，作为 Qt 上层识别和展示 USB 设备的信息载体。
//   直接包含底层 DeviceInfo（含完整接口/端点树），只扩展 Qt 层需要的：
//   - DevType：设备类型（决定打开哪个窗口）
//   - is_hid：是否包含 HID 接口（便捷标志）
//
//   HID 接口信息通过 di.configs[].interfaces[] 遍历获取，
//   接口类 bclass==0x03 即为 HID，端点信息在 InterfaceInfo::endpoints 中。
// ============================================================================

#pragma once

#include "usb_core/usb_device.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <QMetaType>

// ============================================================================
// UsbDeviceInfo - USB 设备信息结构体
//
// 直接包含 DeviceInfo，额外添加 Qt 层所需的扩展字段。
// 访问底层设备信息通过 di 成员，如 info.di.vendor_id。
// HID 接口信息通过 di.configs 遍历，如：
//   for (auto& cfg : info.di.configs)
//     for (auto& iface : cfg.interfaces)
//       if (iface.bclass == 0x03) // HID 接口
//         for (auto& ep : iface.endpoints) ...
// ============================================================================
struct UsbDeviceInfo {
    // 设备类型枚举
    enum DevType {
        DEV_DBG = 0,        // 调试设备
        DEV_KB_GT64HE,      // GT-64HE 键盘
        DEV_KB_MADE68PRO,   // MADE68PRO 键盘
        DEV_KB_WOOTING_60HE,    // Wooting 60HE 键盘
        DEV_KB_WOOTING_TWOHE,   // Wooting TwoHE 键盘
        DEV_MS_G102,        // G102 鼠标
        DEV_UNKNOWN,        // 未知设备
    };

    // --- 底层设备信息（直接包含，含完整接口/端点树） ---
    usb_ctrl::core::DeviceInfo di;  // 完整设备信息

    // --- Qt 层扩展字段 ---
    DevType type = DEV_UNKNOWN;     // 设备类型
    bool is_hid = false;            // 是否包含 HID 接口

    // 从 UsbDevice 创建 UsbDeviceInfo
    static UsbDeviceInfo from_usb_device(const usb_ctrl::core::UsbDevice& dev);

    // 根据 VID/PID 检测设备类型
    static DevType detect_type(uint16_t vid, uint16_t pid);

    // 格式化为可读字符串
    std::string to_string() const;

    // 比较：按 vid/pid/bus/port/addr 判断是否同一设备
    bool operator==(const UsbDeviceInfo& other) const;

    // 获取设备类型名称
    const char* type_name() const;
};

Q_DECLARE_METATYPE(UsbDeviceInfo)
