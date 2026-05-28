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
#include <vector>

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

    // LED位置
    struct LightPos {
        uint16_t x;
        uint16_t y;
    };

    // 按键LED信息：num=LED数量，pos=LED位置列表
    struct LightMsg {
        uint16_t num = 0;
        std::vector<LightPos> pos;
        // 单颗LED构造：{1, {x, y}}
        LightMsg(uint16_t n, LightPos p) : num(n), pos({p}) {}
        // 多颗LED构造：{5, {{x1,y1},{x2,y2},...}}
        LightMsg(uint16_t n, std::vector<LightPos> ps) : num(n), pos(std::move(ps)) {}
        // 默认构造
        LightMsg() = default;
    };

    // 按键LED映射表，按下标索引（key id → LED信息）
    // 单LED按键：{1, {x, y}}
    // 多LED按键：{N, {{x1,y1},{x2,y2},...}}
    struct LightMsg light_msg[61] = {
        // row0 (y=4): Esc ~ Backspace
        {1,{0, 4}}, {1,{1, 4}}, {1,{2, 4}}, {1,{3, 4}}, {1,{4, 4}}, {1,{5, 4}}, {1,{6, 4}}, {1,{7, 4}}, {1,{8, 4}}, {1,{9, 4}}, {1,{10, 4}}, {1,{11, 4}}, {1,{12, 4}}, {1,{13, 4}},
        // row1 (y=3): Tab ~ \ */
        {1,{0, 3}}, {1,{1, 3}}, {1,{2, 3}}, {1,{3, 3}}, {1,{4, 3}}, {1,{5, 3}}, {1,{6, 3}}, {1,{7, 3}}, {1,{8, 3}}, {1,{9, 3}}, {1,{10, 3}}, {1,{11, 3}}, {1,{12, 3}}, {1,{13, 3}},
        // row2 (y=2): Caps ~ Enter
        {1,{0, 2}}, {1,{1, 2}}, {1,{2, 2}}, {1,{3, 2}}, {1,{4, 2}}, {1,{5, 2}}, {1,{6, 2}}, {1,{7, 2}}, {1,{8, 2}}, {1,{9, 2}}, {1,{10, 2}}, {1,{11, 2}}, {1,{12, 2}},
        // row3 (y=1): LShift ~ RShift
        {1,{0, 1}}, {1,{1, 1}}, {1,{2, 1}}, {1,{3, 1}}, {1,{4, 1}}, {1,{5, 1}}, {1,{6, 1}}, {1,{7, 1}}, {1,{8, 1}}, {1,{9, 1}}, {1,{10, 1}}, {1,{11, 1}},
        // row4 (y=0): Ctrl ~ → (Space占5颗LED)
        {1,{0, 0}}, {1,{1, 0}}, {1,{2, 0}}, {5,{{3, 0},{4, 0},{5, 0},{6, 0},{7, 0}}}, {1,{8, 0}}, {1,{9, 0}}, {1,{10, 0}}, {1,{11, 0}},
    };

};
