// ============================================================================
// usb_handler.hpp - USB 请求处理器（头文件）
//
// 将协议消息映射到 UsbController API 调用，返回响应消息
// ============================================================================

#pragma once

#include "usb_protocol.hpp"
#include "usb_controller.hpp"
#include <memory>

namespace usb_srv {

// ============================================================================
// UsbHandler - USB 请求处理器
//
// 职责：
//   1. 接收解码后的请求消息
//   2. 调用 UsbController 对应 API
//   3. 构造响应消息返回
//   4. 管理热插拔通知回调
// ============================================================================
class UsbHandler {
public:
    // 构造函数
    UsbHandler();

    // 析构函数：停止热插拔监听
    ~UsbHandler();

    // 处理请求消息，返回响应消息
    // @param req 请求消息
    // @return 响应消息
    Message handle(const Message& req);

    // 设置热插拔事件通知回调
    // 当热插拔事件发生时，通过此回调推送通知消息
    // @param cb 通知回调函数
    using NotifyCallback = std::function<void(const Message&)>;
    void set_notify_callback(NotifyCallback cb);

private:
    // USB 控制器实例
    std::unique_ptr<usb_ctrl::UsbController> _ctrl;

    // 热插拔通知回调
    NotifyCallback _notify_cb;

    // ---- 请求处理函数 ----

    // 刷新设备列表
    Message _on_refresh_devices(const std::vector<uint8_t>& payload);

    // 获取设备简要列表
    Message _on_list_devices();

    // 获取设备详细列表
    Message _on_list_devices_detail();

    // 获取设备数量
    Message _on_device_count();

    // 获取指定设备详情
    Message _on_device_detail(const std::vector<uint8_t>& payload);

    // 按 VID/PID 查找设备
    Message _on_find_by_vid_pid(const std::vector<uint8_t>& payload);

    // 打开 HID 设备（按索引）
    Message _on_open_hid(const std::vector<uint8_t>& payload);

    // 按 VID/PID 打开 HID
    Message _on_open_hid_vid_pid(const std::vector<uint8_t>& payload);

    // 关闭 HID 设备
    Message _on_close_hid();

    // 查询 HID 是否打开
    Message _on_is_hid_open();

    // HID 读取
    Message _on_hid_read(const std::vector<uint8_t>& payload);

    // HID 写入
    Message _on_hid_write(const std::vector<uint8_t>& payload);

    // 获取 HID 特性报告
    Message _on_hid_get_feature(const std::vector<uint8_t>& payload);

    // 发送 HID 特性报告
    Message _on_hid_send_feature(const std::vector<uint8_t>& payload);

    // 启动热插拔监听
    Message _on_hotplug_start();

    // 停止热插拔监听
    Message _on_hotplug_stop();

    // ---- 辅助函数 ----

    // 构造成功响应（字符串载荷）
    Message _ok(const std::string& text);

    // 构造成功响应（二进制载荷）
    Message _ok(const std::vector<uint8_t>& data);

    // 构造成功响应（无载荷）
    Message _ok();

    // 构造错误响应
    Message _error(const std::string& msg);
};

} // namespace usb_srv
