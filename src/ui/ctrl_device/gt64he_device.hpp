// ============================================================================
// gt64he_device.hpp - GT-64HE 键盘设备（头文件）
//
// 功能说明：
//   定义 GT64HeDevice 类，继承 UsbDeviceBase，
//   为 GT-64HE 键盘提供设备特定的常量和操作。
//
// 设备信息：
//   VID: 0x9013  PID: 0x2601
//   HID 接口：接口 0（键盘接口）
//   自定义端点：EP IN=0x81, EP OUT=0x01, 包大小=64 字节
// ============================================================================

#pragma once

#include "usb_device_base.hpp"

// ============================================================================
// GT64HeDevice - GT-64HE 键盘设备类
//
// 继承 UsbDeviceBase，提供 GT-64HE 键盘特定的常量定义。
// Qt 上层通过此类操作 GT-64HE 键盘。
// ============================================================================
class GT64HeDevice : public UsbDeviceBase {
public:
    // 构造函数：传入设备信息
    explicit GT64HeDevice(const UsbDeviceInfo& info);

    // 析构函数
    ~GT64HeDevice() override;

    // GT-64HE 设备常量
    static constexpr uint16_t VID = 0x9013;     // 厂商 ID
    static constexpr uint16_t PID = 0x2601;     // 产品 ID
    static constexpr int EP_SIZE = 64;           // 端点包大小（字节）

    // 接口编号
    enum Interface {
        INTERFACE_KEYBOARD = 0,  // 键盘接口
        INTERFACE_STANDARD,      // 标准接口
        INTERFACE_CUSTOM,        // 自定义接口
        INTERFACE_LAMP,          // 灯效接口
    };
};
