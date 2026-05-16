// ============================================================================
// usb_hotplug_linux.hpp - USB 热插拔异步监听模块 - Linux 版本（头文件）
//
// 功能说明：
//   基于 libusb_hotplug API 实现 USB 设备异步热插拔监听。
//   继承 UsbHotplugBase 抽象基类，提供 Linux/macOS 平台实现。
//
// 工作原理：
//   1. 调用 libusb_hotplug_register_callback() 注册热插拔回调
//   2. 回调由 libusb 事件循环（UsbEventThread）异步触发
//   3. 设备到达/离开时，调用用户注册的回调函数
//   4. 析构时自动调用 libusb_hotplug_deregister_callback() 注销
//
// 依赖关系：
//   - usb_hotplug_base.hpp：抽象基类
//   - libusb.h：底层热插拔 API
// ============================================================================

#pragma once

#include "usb_hotplug_base.hpp"
#include "libusb.h"

struct libusb_context;
struct libusb_device;

namespace usb_ctrl {
namespace core {

// ============================================================================
// UsbHotplugLinux - USB 热插拔异步监听类（Linux/macOS 实现）
// ============================================================================
class UsbHotplugLinux : public UsbHotplugBase {
public:
    // 构造函数：绑定 libusb 上下文
    explicit UsbHotplugLinux(libusb_context* ctx);

    // 析构函数：自动注销热插拔回调
    ~UsbHotplugLinux() override;

    // 禁止拷贝
    UsbHotplugLinux(const UsbHotplugLinux&) = delete;
    UsbHotplugLinux& operator=(const UsbHotplugLinux&) = delete;

    // 检查当前平台是否支持热插拔
    static bool is_supported();

    // 设置热插拔回调函数
    void set_callback(HotplugCallback cb) override;

    // 设置 VID 过滤
    void set_vid_filter(uint16_t vid) override;

    // 设置 PID 过滤
    void set_pid_filter(uint16_t pid) override;

    // 设置设备类过滤
    void set_class_filter(uint8_t cls) override;

    // 启动热插拔监听
    bool start() override;

    // 停止热插拔监听
    void stop() override;

    // 检查是否正在监听
    bool is_listening() const override;

private:
    // libusb 热插拔回调的静态入口函数
    static int LIBUSB_CALL _hotplug_cb(
        libusb_context* ctx,
        libusb_device* dev,
        libusb_hotplug_event event,
        void* user_data);

    // 处理热插拔事件
    void _on_event(libusb_device* dev, libusb_hotplug_event event);

    libusb_context* _ctx = nullptr;                    // libusb 上下文
    libusb_hotplug_callback_handle _handle = 0;        // 热插拔回调句柄
    HotplugCallback _callback;                         // 用户回调
    uint16_t _vid = LIBUSB_HOTPLUG_MATCH_ANY;          // VID 过滤
    uint16_t _pid = LIBUSB_HOTPLUG_MATCH_ANY;          // PID 过滤
    uint8_t  _cls = LIBUSB_HOTPLUG_MATCH_ANY;          // 类过滤
};

} // namespace core
} // namespace usb_ctrl
