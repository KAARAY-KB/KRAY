// ============================================================================
// usb_handler.cpp - USB 请求处理器（实现文件）
//
// 将协议消息映射到 UsbController API 调用
// ============================================================================

#include "usb_handler.hpp"
#include "console.h"

namespace usb_srv {

// ============================================================================
// 构造/析构
// ============================================================================

// 构造函数：创建 USB 控制器实例
UsbHandler::UsbHandler()
    : _ctrl(std::make_unique<usb_ctrl::UsbController>()) {
}

// 析构函数：停止热插拔监听
UsbHandler::~UsbHandler() {
    if (_ctrl) {
        _ctrl->hotplug_stop();
    }
}

// ============================================================================
// 设置热插拔通知回调
// ============================================================================
void UsbHandler::set_notify_callback(NotifyCallback cb) {
    _notify_cb = std::move(cb);
}

// ============================================================================
// 请求分发入口
// ============================================================================
Message UsbHandler::handle(const Message& req) {
    switch (req.type) {
        // 设备查询
        case REQ_REFRESH_DEVICES:      return _on_refresh_devices(req.payload);
        case REQ_LIST_DEVICES:         return _on_list_devices();
        case REQ_LIST_DEVICES_DETAIL:  return _on_list_devices_detail();
        case REQ_DEVICE_COUNT:         return _on_device_count();
        case REQ_DEVICE_DETAIL:        return _on_device_detail(req.payload);
        case REQ_FIND_BY_VID_PID:      return _on_find_by_vid_pid(req.payload);

        // HID 操作
        case REQ_OPEN_HID:             return _on_open_hid(req.payload);
        case REQ_OPEN_HID_VID_PID:     return _on_open_hid_vid_pid(req.payload);
        case REQ_CLOSE_HID:            return _on_close_hid();
        case REQ_IS_HID_OPEN:          return _on_is_hid_open();
        case REQ_HID_READ:             return _on_hid_read(req.payload);
        case REQ_HID_WRITE:            return _on_hid_write(req.payload);
        case REQ_HID_GET_FEATURE:      return _on_hid_get_feature(req.payload);
        case REQ_HID_SEND_FEATURE:     return _on_hid_send_feature(req.payload);

        // 热插拔
        case REQ_HOTPLUG_START:        return _on_hotplug_start();
        case REQ_HOTPLUG_STOP:         return _on_hotplug_stop();

        // 心跳
        case REQ_PING:                 return Message{RSP_PONG, {}};

        default:
            return _error("Unknown request type: " + std::to_string(req.type));
    }
}

// ============================================================================
// 设备查询处理
// ============================================================================

// 刷新设备列表
Message UsbHandler::_on_refresh_devices(const std::vector<uint8_t>& payload) {
    // 载荷可选：0字节=刷新全部，或带 debug level
    if (payload.size() >= 2) {
        _ctrl->set_debug_level(static_cast<int>(get_u16(payload, 0)));
    }
    _ctrl->refresh_devices();
    return _ok();
}

// 获取设备简要列表
Message UsbHandler::_on_list_devices() {
    return _ok(_ctrl->list_devices());
}

// 获取设备详细列表
Message UsbHandler::_on_list_devices_detail() {
    return _ok(_ctrl->list_devices_detail());
}

// 获取设备数量
Message UsbHandler::_on_device_count() {
    std::vector<uint8_t> data;
    put_u32(data, static_cast<uint32_t>(_ctrl->device_count()));
    return _ok(data);
}

// 获取指定设备详情
Message UsbHandler::_on_device_detail(const std::vector<uint8_t>& payload) {
    if (payload.size() < 4) {
        return _error("Missing device index");
    }
    uint32_t idx = get_u32(payload, 0);
    if (idx >= _ctrl->device_count()) {
        return _error("Index out of range");
    }
    return _ok(_ctrl->device_detail(idx));
}

// 按 VID/PID 查找设备
Message UsbHandler::_on_find_by_vid_pid(const std::vector<uint8_t>& payload) {
    if (payload.size() < 4) {
        return _error("Missing VID/PID");
    }
    uint16_t vid = get_u16(payload, 0);
    uint16_t pid = get_u16(payload, 2);
    auto devs = _ctrl->find_by_vid_pid(vid, pid);

    // 响应：4B 设备数量 + 每个设备 4B 索引
    std::vector<uint8_t> data;
    put_u32(data, static_cast<uint32_t>(devs.size()));
    for (size_t i = 0; i < devs.size(); i++) {
        // 在完整列表中查找索引
        const auto& all = _ctrl->devices();
        for (size_t j = 0; j < all.size(); j++) {
            if (&all[j] == &devs[i]) {
                put_u32(data, static_cast<uint32_t>(j));
                break;
            }
        }
    }
    return _ok(data);
}

// ============================================================================
// HID 操作处理
// ============================================================================

// 打开 HID 设备（按索引）
Message UsbHandler::_on_open_hid(const std::vector<uint8_t>& payload) {
    if (payload.size() < 4) {
        return _error("Missing device index");
    }
    uint32_t idx = get_u32(payload, 0);
    bool ok = _ctrl->open_hid_device(idx);
    return ok ? _ok() : _error("Failed to open HID device");
}

// 按 VID/PID 打开 HID
Message UsbHandler::_on_open_hid_vid_pid(const std::vector<uint8_t>& payload) {
    if (payload.size() < 4) {
        return _error("Missing VID/PID");
    }
    uint16_t vid = get_u16(payload, 0);
    uint16_t pid = get_u16(payload, 2);
    bool ok = _ctrl->open_hid_device_by_vid_pid(vid, pid);
    return ok ? _ok() : _error("Failed to open HID device by VID/PID");
}

// 关闭 HID 设备
Message UsbHandler::_on_close_hid() {
    _ctrl->close_hid_device();
    return _ok();
}

// 查询 HID 是否打开
Message UsbHandler::_on_is_hid_open() {
    std::vector<uint8_t> data;
    data.push_back(_ctrl->is_hid_open() ? 1 : 0);
    return _ok(data);
}

// HID 读取
Message UsbHandler::_on_hid_read(const std::vector<uint8_t>& payload) {
    if (payload.size() < 6) {
        return _error("Missing length/timeout");
    }
    int length = static_cast<int>(get_u16(payload, 0));
    unsigned int timeout = get_u32(payload, 2);
    auto result = _ctrl->hid_read(length, timeout);

    // 响应：1B success + 4B bytes_transferred + NB data
    std::vector<uint8_t> data;
    data.push_back(result.success ? 1 : 0);
    put_u32(data, static_cast<uint32_t>(result.bytes_transferred));
    if (result.success) {
        put_bytes(data, result.data.data(), result.data.size());
    } else {
        put_str(data, result.error_message);
    }
    return _ok(data);
}

// HID 写入
Message UsbHandler::_on_hid_write(const std::vector<uint8_t>& payload) {
    if (payload.size() < 6) {
        return _error("Missing timeout/data");
    }
    unsigned int timeout = get_u32(payload, 0);
    std::vector<uint8_t> write_data(payload.begin() + 4, payload.end());
    auto result = _ctrl->hid_write(write_data, timeout);

    // 响应：1B success + 4B bytes_transferred + error_msg(if failed)
    std::vector<uint8_t> data;
    data.push_back(result.success ? 1 : 0);
    put_u32(data, static_cast<uint32_t>(result.bytes_transferred));
    if (!result.success) {
        put_str(data, result.error_message);
    }
    return _ok(data);
}

// 获取 HID 特性报告
Message UsbHandler::_on_hid_get_feature(const std::vector<uint8_t>& payload) {
    if (payload.size() < 7) {
        return _error("Missing report_id/length/timeout");
    }
    uint8_t report_id = payload[0];
    int length = static_cast<int>(get_u16(payload, 1));
    unsigned int timeout = get_u32(payload, 3);
    auto result = _ctrl->hid_get_feature_report(report_id, length, timeout);

    // 响应：1B success + NB data or error_msg
    std::vector<uint8_t> data;
    data.push_back(result.success ? 1 : 0);
    if (result.success) {
        put_u16(data, static_cast<uint16_t>(result.data.size()));
        put_bytes(data, result.data.data(), result.data.size());
    } else {
        put_u16(data, 0);
        put_str(data, result.error_message);
    }
    return _ok(data);
}

// 发送 HID 特性报告
Message UsbHandler::_on_hid_send_feature(const std::vector<uint8_t>& payload) {
    if (payload.size() < 4) {
        return _error("Missing timeout/data");
    }
    unsigned int timeout = get_u32(payload, 0);
    std::vector<uint8_t> send_data(payload.begin() + 4, payload.end());
    auto result = _ctrl->hid_send_feature_report(send_data, timeout);

    std::vector<uint8_t> data;
    data.push_back(result.success ? 1 : 0);
    if (!result.success) {
        put_str(data, result.error_message);
    }
    return _ok(data);
}

// ============================================================================
// 热插拔处理
// ============================================================================

// 启动热插拔监听
Message UsbHandler::_on_hotplug_start() {
    if (!usb_ctrl::UsbController::is_hotplug_supported()) {
        return _error("Hotplug not supported on this platform");
    }

    // 设置热插拔回调：将事件转为通知消息推送
    bool ok = _ctrl->hotplug_start([this](usb_ctrl::core::HotplugEvent event,
                                           const usb_ctrl::UsbDevice& dev) {
        if (!_notify_cb) return;

        // 构造热插拔通知载荷：1B event + 2B vid + 2B pid + str serial
        std::vector<uint8_t> payload;
        payload.push_back(static_cast<uint8_t>(event)); // 0=arrived, 1=left
        auto info = dev.get_info();
        put_u16(payload, info.vendor_id);
        put_u16(payload, info.product_id);
        put_str(payload, info.serial_number);

        Message ntf;
        ntf.type = NTF_HOTPLUG_EVENT;
        ntf.payload = std::move(payload);
        _notify_cb(ntf);
    });

    return ok ? _ok() : _error("Failed to start hotplug");
}

// 停止热插拔监听
Message UsbHandler::_on_hotplug_stop() {
    _ctrl->hotplug_stop();
    return _ok();
}

// ============================================================================
// 辅助函数
// ============================================================================

// 构造成功响应（字符串载荷）
Message UsbHandler::_ok(const std::string& text) {
    Message msg;
    msg.type = RSP_OK;
    put_str(msg.payload, text);
    return msg;
}

// 构造成功响应（二进制载荷）
Message UsbHandler::_ok(const std::vector<uint8_t>& data) {
    Message msg;
    msg.type = RSP_OK;
    msg.payload = data;
    return msg;
}

// 构造成功响应（无载荷）
Message UsbHandler::_ok() {
    Message msg;
    msg.type = RSP_OK;
    return msg;
}

// 构造错误响应
Message UsbHandler::_error(const std::string& msg) {
    Message m;
    m.type = RSP_ERROR;
    put_str(m.payload, msg);
    return m;
}

} // namespace usb_srv
