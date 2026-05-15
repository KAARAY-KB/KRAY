// ============================================================================
// usb_hotplug.hpp - USB 热插拔异步监听模块（头文件）
//
// 功能说明：
//   本文件定义了 UsbHotplug 类，基于 libusb_hotplug API 实现
//   USB 设备的异步热插拔监听。当设备插入或拔出时，通过回调
//   通知用户，无需轮询查询设备列表变化。
//
// 工作原理：
//   1. 调用 libusb_hotplug_register_callback() 注册热插拔回调
//   2. 回调由 libusb 事件循环（UsbEventThread）异步触发
//   3. 设备到达/离开时，调用用户注册的回调函数
//   4. 析构时自动调用 libusb_hotplug_deregister_callback() 注销
//
// 依赖关系：
//   - libusb.h：底层热插拔 API
//   - usb_device.hpp：UsbDevice 类
//   - usb_event_thread.hpp：事件处理线程
// ============================================================================

#pragma once // 防止头文件重复包含

#include "usb_device.hpp"   // UsbDevice 类
#include <functional>       // std::function 回调类型
#include <cstdint>          // uint16_t 等定长整数
#include <libusb.h>        // libusb 热插拔 API

// 前向声明 libusb 上下文和热插拔回调句柄
struct libusb_context;
struct libusb_device;

namespace usb_ctrl {
namespace core {

// ============================================================================
// HotplugEvent - 热插拔事件类型枚举
//
// 标识设备是插入还是拔出。
// ============================================================================
enum class HotplugEvent {
    Arrived = 0, // 设备插入
    Left    = 1, // 设备拔出
};

// ============================================================================
// HotplugCallback - 热插拔回调类型
//
// 当设备插入或拔出时调用此回调。
// @param event  事件类型（Arrived/Left）
// @param device 发生事件的 USB 设备对象
// ============================================================================
using HotplugCallback = std::function<void(HotplugEvent event, UsbDevice& device)>;

// ============================================================================
// UsbHotplug - USB 热插拔异步监听类
//
// 封装 libusb_hotplug_register_callback / deregister_callback，
// 提供简洁的 C++ 接口监听 USB 设备的热插拔事件。
//
// 使用示例：
//   UsbHotplug hotplug(ctx.handle());
//   hotplug.set_callback([](HotplugEvent ev, UsbDevice& dev) {
//       if (ev == HotplugEvent::Arrived)
//           std::cout << "设备插入: " << dev.summary() << "\n";
//       else
//           std::cout << "设备拔出: " << dev.summary() << "\n";
//   });
//   hotplug.start();  // 注册回调
//   // ... 事件线程运行中，回调自动触发 ...
//   hotplug.stop();   // 注销回调
//
// 注意：
//   - 回调在 UsbEventThread 线程中执行，需注意线程安全
//   - 需要先启动 UsbEventThread，热插拔回调才会被触发
//   - 部分平台不支持热插拔，需调用 is_supported() 检查
// ============================================================================
class UsbHotplug {
public:
    // 构造函数：绑定 libusb 上下文
    // @param ctx 已初始化的 libusb_context 指针
    explicit UsbHotplug(libusb_context* ctx);

    // 析构函数：自动注销热插拔回调
    ~UsbHotplug();

    // 禁止拷贝（回调句柄唯一）
    UsbHotplug(const UsbHotplug&) = delete;
    UsbHotplug& operator=(const UsbHotplug&) = delete;

    // 检查当前平台是否支持热插拔
    // @return true 表示支持
    static bool is_supported();

    // 设置热插拔回调函数
    // @param cb 回调函数
    void set_callback(HotplugCallback cb);

    // 设置 VID 过滤（仅监听指定厂商的设备）
    // LIBUSB_HOTPLUG_MATCH_ANY 表示匹配所有厂商
    // @param vid 厂商 ID
    void set_vid_filter(uint16_t vid);

    // 设置 PID 过滤（仅监听指定产品）
    // LIBUSB_HOTPLUG_MATCH_ANY 表示匹配所有产品
    // @param pid 产品 ID
    void set_pid_filter(uint16_t pid);

    // 设置设备类过滤（仅监听指定类的设备）
    // LIBUSB_HOTPLUG_MATCH_ANY 表示匹配所有类
    // @param cls 设备类代码
    void set_class_filter(uint8_t cls);

    // 启动热插拔监听（注册回调）
    // @return true 表示注册成功
    bool start();

    // 停止热插拔监听（注销回调）
    void stop();

    // 检查是否正在监听
    // @return true 表示已注册回调
    bool is_listening() const { return _handle != 0; }

private:
    // libusb 热插拔回调的静态入口函数（静态，C 链接）
    // 由 libusb 在事件循环中调用，转发给 _on_event
    static int LIBUSB_CALL _hotplug_cb(
        libusb_context* ctx,
        libusb_device* dev,
        libusb_hotplug_event event,
        void* user_data);

    // 处理热插拔事件（实例方法）
    // @param dev   发生事件的 libusb 设备
    // @param event libusb 热插拔事件类型
    void _on_event(libusb_device* dev, libusb_hotplug_event event);

    libusb_context* _ctx = nullptr;     // libusb 上下文指针（不拥有所有权）
    libusb_hotplug_callback_handle _handle = 0; // 热插拔回调句柄（0 表示未注册）
    HotplugCallback _callback;          // 用户回调函数

    // 过滤条件（默认匹配所有设备）
    uint16_t _vid = LIBUSB_HOTPLUG_MATCH_ANY; // 厂商 ID 过滤
    uint16_t _pid = LIBUSB_HOTPLUG_MATCH_ANY; // 产品 ID 过滤
    uint8_t  _cls = LIBUSB_HOTPLUG_MATCH_ANY; // 设备类过滤
};

} // namespace core
} // namespace usb_ctrl
