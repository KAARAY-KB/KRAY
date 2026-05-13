#pragma once

#include "usb_transfer.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <cstdint>

#include "libusb.h"

namespace usb_transfer {

// 异步传输回调函数类型
using AsyncCallback = std::function<void(TransferResult)>;

// 异步传输类型枚举
enum class AsyncTransferType {
    BulkRead,        // Bulk 读
    BulkWrite,       // Bulk 写
    InterruptRead,   // 中断读
    InterruptWrite   // 中断写
};

// 异步传输请求结构体
// 封装一次异步传输的所有参数
struct AsyncTransferRequest {
    AsyncTransferType type;          // 传输类型
    unsigned char endpoint;          // 端点地址
    std::vector<uint8_t> data;       // 发送的数据 (写操作)
    int length;                      // 期望读取的字节数 (读操作)
    unsigned int timeout_ms;         // 超时时间 (毫秒)
    AsyncCallback callback;          // 传输完成后的回调函数
};

// 异步传输引擎类
// 使用独立线程处理 libusb 异步事件
// 支持提交多个异步传输请求并在后台处理
class AsyncTransferEngine {
public:
    explicit AsyncTransferEngine(libusb_context* ctx);
    ~AsyncTransferEngine();

    // 禁止拷贝
    AsyncTransferEngine(const AsyncTransferEngine&) = delete;
    AsyncTransferEngine& operator=(const AsyncTransferEngine&) = delete;

    // 启动异步传输引擎 (创建事件处理线程)
    void start();

    // 停止异步传输引擎 (等待线程退出)
    void stop();

    // 提交异步传输请求
    bool submit(libusb_device_handle* handle, AsyncTransferRequest&& request);

    // 判断引擎是否正在运行
    bool is_running() const { return _running.load(std::memory_order_acquire); }

    // 获取待处理的传输请求数量
    size_t pending_count() const;

private:
    // 事件循环线程函数：处理 libusb 异步事件
    void _event_loop();

    // 处理传输完成回调
    void _handle_completion(libusb_transfer* transfer);

    // libusb 传输完成静态回调函数
    static void LIBUSB_CALL _transfer_cb(libusb_transfer* transfer);

    // 传输上下文结构体：保存回调函数和引擎指针
    struct TransferContext {
        AsyncTransferEngine* engine;
        libusb_transfer* transfer;
        AsyncCallback callback;
    };

    libusb_context* _ctx;
    libusb_device_handle* _handle = nullptr;
    std::atomic<bool> _running{false};
    std::thread _event_thread;
    std::mutex _mutex;
    std::atomic<size_t> _pending_count{0};
};

// HID 异步传输封装类
// 基于 AsyncTransferEngine 提供 HID 专用的异步读写接口
class AsyncHidTransfer {
public:
    AsyncHidTransfer(AsyncTransferEngine& engine, libusb_device_handle* handle,
                     unsigned char in_ep, unsigned char out_ep);

    // 异步读取 HID 报告 (单次)
    void read_async(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 异步写入 HID 报告 (单次)
    void write_async(const std::vector<uint8_t>& data, AsyncCallback callback,
                      unsigned int timeout_ms = 1000);

    // 连续异步读取 HID 报告 (循环读取直到调用 stop_continuous)
    void read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 停止连续读取
    void stop_continuous();

private:
    AsyncTransferEngine& _engine;
    libusb_device_handle* _handle;
    unsigned char _in_ep;
    unsigned char _out_ep;
    std::atomic<bool> _continuous{false};
};

} // namespace usb_transfer
