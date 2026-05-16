// ============================================================================
// usb_device_info.hpp - USB 设备信息结构体（头文件）
//
// 功能说明：
//   定义 UsbDeviceInfo 结构体，作为 Qt 上层识别和展示 USB 设备的信息载体。
//   替代旧模块的 USBHelper::DevMsg_t，提供设备类型判断、信息格式化等功能。
//
// 设计思路：
//   - 从 usb_ctrl::core::UsbDevice 提取关键信息
//   - 保留设备类型枚举（DevType），用于 Qt 层决定打开哪种设备窗口
//   - 提供 VID/PID 到设备类型的自动检测
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace usb_ctrl {
namespace core {
class UsbDevice; // 前向声明
}
}

// ============================================================================
// UsbDeviceInfo - USB 设备信息结构体
//
// 包含设备标识信息和可读描述信息，用于 Qt 上层展示和设备路由。
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

    DevType type = DEV_UNKNOWN; // 设备类型
    uint16_t vid = 0;           // 厂商 ID
    uint16_t pid = 0;           // 产品 ID
    uint8_t bus = 0;            // 总线编号
    uint8_t port = 0;           // 端口号
    uint8_t addr = 0;           // 设备地址
    std::string mfr;            // 制造商
    std::string prod;           // 产品名称
    std::string sn;             // 序列号

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
