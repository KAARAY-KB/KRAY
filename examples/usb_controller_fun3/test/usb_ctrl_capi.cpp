// ============================================================================
// usb_ctrl_capi.cpp - USB 控制器 C 语言接口实现
//
// 功能说明：
//   实现 usb_ctrl_capi.h 中定义的 C 接口，内部使用 UsbController C++ 类。
//   编译为共享库供 Python ctypes 加载。
//
// 设计要点：
//   - 使用不透明指针封装 C++ 对象
//   - 异步读取和热插拔事件通过内部队列缓存
//   - Python 端通过 poll 函数轮询获取结果
// ============================================================================

#include "usb_ctrl_capi.h"
#include "usb_controller.hpp"

#include <cstring>
#include <mutex>
#include <queue>
#include <string>

using namespace usb_ctrl;

// ============================================================================
// 内部辅助：复制字符串到 malloc 分配的内存
// ============================================================================
static char* dup_str(const std::string& s) {
    char* p = (char*)malloc(s.size() + 1); // 分配内存
    if (p) memcpy(p, s.c_str(), s.size() + 1); // 复制字符串
    return p;
}

// ============================================================================
// 内部辅助：TransferResult 转 JSON 字符串
// ============================================================================
static char* result_to_json(const TransferResult& r) {
    std::string json = "{"; // JSON 开始
    json += "\"success\":" + std::string(r.success ? "true" : "false"); // 成功标志
    json += ",\"bytes\":" + std::to_string(r.bytes_transferred); // 传输字节数
    json += ",\"error_code\":" + std::to_string(r.error_code); // 错误码
    json += ",\"error_msg\":\"" + r.error_message + "\""; // 错误消息
    // 数据的十六进制表示
    json += ",\"data_hex\":\"";
    for (size_t i = 0; i < r.data.size() && i < 256; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", r.data[i]); // 转十六进制
        json += buf;
    }
    json += "\"";
    json += "}";
    return dup_str(json); // 返回 JSON 字符串
}

// ============================================================================
// 内部结构：异步读取结果队列
// ============================================================================
struct AsyncQueue {
    std::mutex mtx; // 互斥锁
    std::queue<std::string> items; // 结果队列
};

// ============================================================================
// 内部结构：热插拔事件队列
// ============================================================================
struct HotplugQueue {
    std::mutex mtx; // 互斥锁
    std::queue<std::string> items; // 事件队列
};

// ============================================================================
// 内部结构：控制器上下文（包含 UsbController 和队列）
// ============================================================================
struct CtrlContext {
    UsbController ctrl; // USB 控制器
    AsyncQueue async_q; // 异步结果队列
    HotplugQueue hotplug_q; // 热插拔事件队列
};

// ============================================================================
// 创建/销毁
// ============================================================================
UsbCtrlHandle usb_ctrl_create(void) {
    CtrlContext* ctx = new CtrlContext(); // 创建上下文
    ctx->ctrl.set_debug_level(0); // 关闭调试
    ctx->ctrl.refresh_devices(); // 刷新设备
    return (UsbCtrlHandle)ctx; // 返回句柄
}

void usb_ctrl_destroy(UsbCtrlHandle h) {
    if (!h) return; // 空指针检查
    CtrlContext* ctx = (CtrlContext*)h; // 转换句柄
    ctx->ctrl.close_hid_device(); // 关闭 HID 设备
    ctx->ctrl.async_stop(); // 停止异步引擎
    if (ctx->ctrl.is_hotplug_listening()) {
        ctx->ctrl.hotplug_stop(); // 停止热插拔
    }
    delete ctx; // 释放上下文
}

// ============================================================================
// 基础设置
// ============================================================================
void usb_ctrl_set_debug(UsbCtrlHandle h, int level) {
    if (!h) return; // 空指针检查
    ((CtrlContext*)h)->ctrl.set_debug_level(level); // 设置调试级别
}

void usb_ctrl_refresh(UsbCtrlHandle h) {
    if (!h) return; // 空指针检查
    ((CtrlContext*)h)->ctrl.refresh_devices(); // 刷新设备
}

int usb_ctrl_device_count(UsbCtrlHandle h) {
    if (!h) return 0; // 空指针检查
    return (int)((CtrlContext*)h)->ctrl.device_count(); // 返回设备数量
}

// ============================================================================
// 设备查询
// ============================================================================
char* usb_ctrl_list_devices(UsbCtrlHandle h) {
    if (!h) return dup_str(""); // 空指针检查
    return dup_str(((CtrlContext*)h)->ctrl.list_devices()); // 返回设备列表
}

char* usb_ctrl_list_devices_detail(UsbCtrlHandle h) {
    if (!h) return dup_str(""); // 空指针检查
    return dup_str(((CtrlContext*)h)->ctrl.list_devices_detail()); // 返回设备详情
}

char* usb_ctrl_list_devices_tree(UsbCtrlHandle h) {
    if (!h) return dup_str(""); // 空指针检查
    return dup_str(((CtrlContext*)h)->ctrl.list_devices_tree()); // 返回设备树
}

char* usb_ctrl_list_hid_devices(UsbCtrlHandle h) {
    if (!h) return dup_str(""); // 空指针检查
    return dup_str(((CtrlContext*)h)->ctrl.list_hid_devices()); // 返回 HID 列表
}

char* usb_ctrl_device_detail(UsbCtrlHandle h, int index) {
    if (!h) return dup_str(""); // 空指针检查
    return dup_str(((CtrlContext*)h)->ctrl.device_detail(index)); // 返回设备详情
}

// ============================================================================
// HID 设备操作
// ============================================================================
int usb_ctrl_open_hid(UsbCtrlHandle h, int index) {
    if (!h) return 0; // 空指针检查
    return ((CtrlContext*)h)->ctrl.open_hid_device(index) ? 1 : 0; // 按索引打开
}

int usb_ctrl_open_hid_vid_pid(UsbCtrlHandle h, uint16_t vid, uint16_t pid) {
    if (!h) return 0; // 空指针检查
    return ((CtrlContext*)h)->ctrl.open_hid_device_by_vid_pid(vid, pid) ? 1 : 0; // 按 VID:PID 打开
}

void usb_ctrl_close_hid(UsbCtrlHandle h) {
    if (!h) return; // 空指针检查
    ((CtrlContext*)h)->ctrl.close_hid_device(); // 关闭 HID 设备
}

int usb_ctrl_is_hid_open(UsbCtrlHandle h) {
    if (!h) return 0; // 空指针检查
    return ((CtrlContext*)h)->ctrl.is_hid_open() ? 1 : 0; // 检查是否打开
}

char* usb_ctrl_hid_info(UsbCtrlHandle h) {
    if (!h) return dup_str(""); // 空指针检查
    return dup_str(((CtrlContext*)h)->ctrl.hid_device_info()); // 返回设备信息
}

// ============================================================================
// HID 数据传输
// ============================================================================
char* usb_ctrl_hid_read(UsbCtrlHandle h, int len, unsigned int timeout_ms) {
    if (!h) return result_to_json(TransferResult()); // 空指针检查
    auto r = ((CtrlContext*)h)->ctrl.hid_read(len, timeout_ms); // 同步读取
    return result_to_json(r); // 返回 JSON 结果
}

char* usb_ctrl_hid_read_latest(UsbCtrlHandle h, int len, unsigned int drain_timeout_ms) {
    if (!h) return result_to_json(TransferResult()); // 空指针检查
    auto r = ((CtrlContext*)h)->ctrl.hid_read_latest(len, drain_timeout_ms); // 读取最新
    return result_to_json(r); // 返回 JSON 结果
}

char* usb_ctrl_hid_write(UsbCtrlHandle h, const uint8_t* data, int data_len, unsigned int timeout_ms) {
    if (!h) return result_to_json(TransferResult()); // 空指针检查
    std::vector<uint8_t> vec(data, data + data_len); // 构造数据向量
    auto r = ((CtrlContext*)h)->ctrl.hid_write(vec, timeout_ms); // 同步写入
    return result_to_json(r); // 返回 JSON 结果
}

char* usb_ctrl_hid_get_feature(UsbCtrlHandle h, uint8_t report_id, int len, unsigned int timeout_ms) {
    if (!h) return result_to_json(TransferResult()); // 空指针检查
    auto r = ((CtrlContext*)h)->ctrl.hid_get_feature_report(report_id, len, timeout_ms); // 获取特性报告
    return result_to_json(r); // 返回 JSON 结果
}

char* usb_ctrl_hid_send_feature(UsbCtrlHandle h, const uint8_t* data, int data_len, unsigned int timeout_ms) {
    if (!h) return result_to_json(TransferResult()); // 空指针检查
    std::vector<uint8_t> vec(data, data + data_len); // 构造数据向量
    auto r = ((CtrlContext*)h)->ctrl.hid_send_feature_report(vec, timeout_ms); // 发送特性报告
    return result_to_json(r); // 返回 JSON 结果
}

// ============================================================================
// 批量传输
// ============================================================================
char* usb_ctrl_bulk_read(UsbCtrlHandle h, uint8_t endpoint, int len, unsigned int timeout_ms) {
    if (!h) return result_to_json(TransferResult()); // 空指针检查
    auto r = ((CtrlContext*)h)->ctrl.bulk_read(endpoint, len, timeout_ms); // 批量读取
    return result_to_json(r); // 返回 JSON 结果
}

char* usb_ctrl_bulk_write(UsbCtrlHandle h, uint8_t endpoint, const uint8_t* data, int data_len, unsigned int timeout_ms) {
    if (!h) return result_to_json(TransferResult()); // 空指针检查
    std::vector<uint8_t> vec(data, data + data_len); // 构造数据向量
    auto r = ((CtrlContext*)h)->ctrl.bulk_write(endpoint, vec, timeout_ms); // 批量写入
    return result_to_json(r); // 返回 JSON 结果
}

// ============================================================================
// 异步传输
// ============================================================================
void usb_ctrl_async_start(UsbCtrlHandle h) {
    if (!h) return; // 空指针检查
    ((CtrlContext*)h)->ctrl.async_start(); // 启动异步引擎
}

void usb_ctrl_async_stop(UsbCtrlHandle h) {
    if (!h) return; // 空指针检查
    ((CtrlContext*)h)->ctrl.async_stop(); // 停止异步引擎
}

int usb_ctrl_async_pending(UsbCtrlHandle h) {
    if (!h) return 0; // 空指针检查
    return (int)((CtrlContext*)h)->ctrl.async_pending_count(); // 返回待处理数量
}

void usb_ctrl_async_read_continuous(UsbCtrlHandle h, int len, unsigned int timeout_ms) {
    if (!h) return; // 空指针检查
    CtrlContext* ctx = (CtrlContext*)h; // 获取上下文
    AsyncQueue* q = &ctx->async_q; // 获取异步队列
    // 启动连续异步读取，回调中将结果推入队列
    ctx->ctrl.hid_read_continuous(len, [q](const TransferResult& r) {
        std::string json = "{"; // 构建 JSON
        json += "\"success\":" + std::string(r.success ? "true" : "false");
        json += ",\"bytes\":" + std::to_string(r.bytes_transferred);
        json += ",\"data_hex\":\"";
        for (size_t i = 0; i < r.data.size() && i < 256; ++i) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02X", r.data[i]);
            json += buf;
        }
        json += "\"}";
        std::lock_guard<std::mutex> lock(q->mtx); // 加锁
        q->items.push(json); // 推入队列
    }, timeout_ms);
}

void usb_ctrl_async_stop_continuous(UsbCtrlHandle h) {
    if (!h) return; // 空指针检查
    ((CtrlContext*)h)->ctrl.hid_stop_continuous(); // 停止连续读取
}

char* usb_ctrl_async_poll(UsbCtrlHandle h) {
    if (!h) return nullptr; // 空指针检查
    CtrlContext* ctx = (CtrlContext*)h; // 获取上下文
    AsyncQueue* q = &ctx->async_q; // 获取异步队列
    std::lock_guard<std::mutex> lock(q->mtx); // 加锁
    if (q->items.empty()) return nullptr; // 队列为空
    std::string item = q->items.front(); // 取出队首
    q->items.pop(); // 弹出
    return dup_str(item); // 返回结果
}

// ============================================================================
// 热插拔
// ============================================================================
int usb_ctrl_is_hotplug_supported(void) {
    return UsbController::is_hotplug_supported() ? 1 : 0; // 检查支持
}

int usb_ctrl_hotplug_start(UsbCtrlHandle h) {
    if (!h) return 0; // 空指针检查
    CtrlContext* ctx = (CtrlContext*)h; // 获取上下文
    HotplugQueue* q = &ctx->hotplug_q; // 获取热插拔队列
    // 启动热插拔监听，回调中将事件推入队列
    bool ok = ctx->ctrl.hotplug_start([q](HotplugEvent ev, UsbDevice& dev) {
        std::string json = "{"; // 构建 JSON
        json += "\"event\":\"" + std::string(ev == HotplugEvent::Arrived ? "arrived" : "left") + "\"";
        json += ",\"summary\":\"" + dev.summary() + "\"";
        json += "}";
        std::lock_guard<std::mutex> lock(q->mtx); // 加锁
        q->items.push(json); // 推入队列
    });
    return ok ? 1 : 0; // 返回结果
}

void usb_ctrl_hotplug_stop(UsbCtrlHandle h) {
    if (!h) return; // 空指针检查
    ((CtrlContext*)h)->ctrl.hotplug_stop(); // 停止热插拔
}

int usb_ctrl_is_hotplug_listening(UsbCtrlHandle h) {
    if (!h) return 0; // 空指针检查
    return ((CtrlContext*)h)->ctrl.is_hotplug_listening() ? 1 : 0; // 检查监听状态
}

char* usb_ctrl_hotplug_poll(UsbCtrlHandle h) {
    if (!h) return nullptr; // 空指针检查
    CtrlContext* ctx = (CtrlContext*)h; // 获取上下文
    HotplugQueue* q = &ctx->hotplug_q; // 获取热插拔队列
    std::lock_guard<std::mutex> lock(q->mtx); // 加锁
    if (q->items.empty()) return nullptr; // 队列为空
    std::string item = q->items.front(); // 取出队首
    q->items.pop(); // 弹出
    return dup_str(item); // 返回事件
}

// ============================================================================
// 内存管理
// ============================================================================
void usb_ctrl_free_str(char* s) {
    if (s) free(s); // 释放字符串
}
