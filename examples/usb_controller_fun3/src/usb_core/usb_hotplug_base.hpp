// ============================================================================
// usb_hotplug_base.hpp - USB 热插拔抽象基类（头文件）
//
// 功能说明：
//   定义 UsbHotplugBase 抽象基类，提供跨平台热插拔的统一接口。
//   Linux 和 Windows 各自实现子类，UsbController 只依赖此基类，
//   通过工厂函数创建平台相关实例，消除条件编译耦合。
//
// 设计模式：策略模式（Strategy Pattern）
//   UsbHotplugBase = 策略接口
//   UsbHotplug     = Linux 策略（libusb 原生热插拔）
//   UsbHotplugWin  = Windows 策略（WM_DEVICECHANGE + SetupDi）
// ============================================================================

#pragma once

#include "usb_device.hpp"
#include <functional>
#include <cstdint>
#include <memory>

// 前向声明 libusb_context（避免头文件中依赖 libusb.h）
struct libusb_context;

namespace usb_ctrl {
namespace core {

// 热插拔事件类型
enum class HotplugEvent {
    Arrived = 0, // 设备插入
    Left    = 1, // 设备拔出
};

// 热插拔回调类型
using HotplugCallback = std::function<void(HotplugEvent event, UsbDevice& device)>;

// ============================================================================
// UsbHotplugBase - USB 热插拔抽象基类
//
// 定义热插拔监听的统一接口，所有平台实现必须继承此类。
// ============================================================================
class UsbHotplugBase {
public:
    virtual ~UsbHotplugBase() = default;

    // 检查当前平台是否支持热插拔
    static bool is_supported();

    // 设置热插拔回调函数
    virtual void set_callback(HotplugCallback cb) = 0;

    // 设置 VID 过滤
    virtual void set_vid_filter(uint16_t vid) = 0;

    // 设置 PID 过滤
    virtual void set_pid_filter(uint16_t pid) = 0;

    // 设置设备类过滤
    virtual void set_class_filter(uint8_t cls) = 0;

    // 启动热插拔监听
    virtual bool start() = 0;

    // 停止热插拔监听
    virtual void stop() = 0;

    // 检查是否正在监听
    virtual bool is_listening() const = 0;
};

// 工厂函数：创建平台相关的热插拔实例
// @param ctx libusb 上下文指针（Linux 需要，Windows 忽略）
// @return 平台相关的热插拔实例
std::unique_ptr<UsbHotplugBase> create_hotplug(libusb_context* ctx);

} // namespace core
} // namespace usb_ctrl
