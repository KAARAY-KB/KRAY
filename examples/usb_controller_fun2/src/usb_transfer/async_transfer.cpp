#include "async_transfer.hpp"
#include "usb_core/usb_context.hpp"
#include "libusb.h"

#include <iostream>

namespace usb_transfer {

// AsyncTransferEngine 构造函数：接收 libusb 上下文
AsyncTransferEngine::AsyncTransferEngine(libusb_context* ctx)
    : _ctx(ctx) {
}

// AsyncTransferEngine 析构函数：确保引擎已停止
AsyncTransferEngine::~AsyncTransferEngine() {
    stop();
}

// 启动异步传输引擎
// 创建事件处理线程，开始处理异步传输请求
void AsyncTransferEngine::start() {
    if (_running.load(std::memory_order_acquire)) return;
    _running.store(true, std::memory_order_release);
    _event_thread = std::thread(&AsyncTransferEngine::_event_loop, this);
}

// 停止异步传输引擎
// 设置停止标志并等待事件处理线程退出
void AsyncTransferEngine::stop() {
    if (!_running.load(std::memory_order_acquire)) return;
    _running.store(false, std::memory_order_release);
    if (_event_thread.joinable()) {
        _event_thread.join();
    }
}

// 获取待处理的传输请求数量
size_t AsyncTransferEngine::pending_count() const {
    return _pending_count.load(std::memory_order_acquire);
}

// 提交异步传输请求
// handle: 设备句柄
// request: 传输请求 (移动语义)
// 返回: 是否提交成功
bool AsyncTransferEngine::submit(libusb_device_handle* handle, AsyncTransferRequest&& request) {
    if (!_running.load(std::memory_order_acquire)) return false;

    _handle = handle;

    // 分配 libusb 传输结构
    auto* transfer = libusb_alloc_transfer(0);
    if (!transfer) return false;

    // 创建传输上下文，保存回调函数
    auto* ctx = new TransferContext{this, transfer, std::move(request.callback)};
    transfer->user_data = ctx;

    // 分配传输缓冲区
    unsigned char* buf = nullptr;
    if (request.type == AsyncTransferType::BulkWrite ||
        request.type == AsyncTransferType::InterruptWrite) {
        // 写操作：复制发送数据
        buf = new unsigned char[request.data.size()];
        std::memcpy(buf, request.data.data(), request.data.size());
    } else {
        // 读操作：分配接收缓冲区
        buf = new unsigned char[request.length];
    }

    // 根据传输类型填充传输结构
    switch (request.type) {
        case AsyncTransferType::BulkRead:
            libusb_fill_bulk_transfer(transfer, handle, request.endpoint,
                                       buf, request.length, _transfer_cb, ctx,
                                       request.timeout_ms);
            break;
        case AsyncTransferType::BulkWrite:
            libusb_fill_bulk_transfer(transfer, handle, request.endpoint,
                                       buf, static_cast<int>(request.data.size()),
                                       _transfer_cb, ctx, request.timeout_ms);
            break;
        case AsyncTransferType::InterruptRead:
            libusb_fill_interrupt_transfer(transfer, handle, request.endpoint,
                                            buf, request.length, _transfer_cb, ctx,
                                            request.timeout_ms);
            break;
        case AsyncTransferType::InterruptWrite:
            libusb_fill_interrupt_transfer(transfer, handle, request.endpoint,
                                            buf, static_cast<int>(request.data.size()),
                                            _transfer_cb, ctx, request.timeout_ms);
            break;
    }

    // 设置自动释放缓冲区标志
    transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

    // 增加待处理计数
    _pending_count.fetch_add(1, std::memory_order_release);

    // 提交传输请求
    int rc = libusb_submit_transfer(transfer);
    if (rc < 0) {
        // 提交失败：清理资源
        delete[] buf;
        transfer->buffer = nullptr;
        delete ctx;
        libusb_free_transfer(transfer);
        _pending_count.fetch_sub(1, std::memory_order_release);
        return false;
    }

    return true;
}

// 事件循环线程函数
// 循环调用 libusb_handle_events_timeout 处理异步事件
// 每次循环等待 100ms 超时
void AsyncTransferEngine::_event_loop() {
    while (_running.load(std::memory_order_acquire)) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int rc = libusb_handle_events_timeout(_ctx, &tv);
        if (rc < 0 && _running.load(std::memory_order_acquire)) {
            break;
        }
    }
}

// libusb 传输完成静态回调函数
// 当异步传输完成时被 libusb 调用
// 负责解析传输结果并调用用户回调函数
void LIBUSB_CALL AsyncTransferEngine::_transfer_cb(libusb_transfer* transfer) {
    auto* ctx = static_cast<TransferContext*>(transfer->user_data);
    if (!ctx || !ctx->engine) return;

    // 构建传输结果
    TransferResult result;
    result.success = (transfer->status == LIBUSB_TRANSFER_COMPLETED);
    result.bytes_transferred = transfer->actual_length;
    result.error_code = static_cast<int>(transfer->status);

    // 设置错误描述
    if (!result.success) {
        switch (transfer->status) {
            case LIBUSB_TRANSFER_ERROR:     result.error_message = "Transfer error"; break;
            case LIBUSB_TRANSFER_TIMED_OUT: result.error_message = "Transfer timed out"; break;
            case LIBUSB_TRANSFER_CANCELLED: result.error_message = "Transfer cancelled"; break;
            case LIBUSB_TRANSFER_STALL:     result.error_message = "Endpoint stalled"; break;
            case LIBUSB_TRANSFER_NO_DEVICE: result.error_message = "Device disconnected"; break;
            case LIBUSB_TRANSFER_OVERFLOW:  result.error_message = "Data overflow"; break;
            default:                         result.error_message = "Unknown error"; break;
        }
    }

    // 复制接收到的数据
    if (result.success && result.bytes_transferred > 0 && transfer->buffer) {
        result.data.assign(transfer->buffer, transfer->buffer + result.bytes_transferred);
    }

    // 调用用户回调函数
    if (ctx->callback) {
        ctx->callback(result);
    }

    // 减少待处理计数
    ctx->engine->_pending_count.fetch_sub(1, std::memory_order_release);

    // 释放传输资源
    libusb_free_transfer(transfer);
    delete ctx;
}

// AsyncHidTransfer 构造函数：关联异步引擎和端点信息
AsyncHidTransfer::AsyncHidTransfer(AsyncTransferEngine& engine, libusb_device_handle* handle,
                                     unsigned char in_ep, unsigned char out_ep)
    : _engine(engine), _handle(handle), _in_ep(in_ep), _out_ep(out_ep) {
}

// 异步读取 HID 报告 (单次)
// length: 期望读取的字节数
// callback: 读取完成后的回调函数
void AsyncHidTransfer::read_async(int length, AsyncCallback callback, unsigned int timeout_ms) {
    AsyncTransferRequest req;
    req.type = AsyncTransferType::InterruptRead;
    req.endpoint = _in_ep;
    req.length = length;
    req.timeout_ms = timeout_ms;
    req.callback = std::move(callback);
    _engine.submit(_handle, std::move(req));
}

// 异步写入 HID 报告 (单次)
// data: 要发送的报告数据
// callback: 发送完成后的回调函数
void AsyncHidTransfer::write_async(const std::vector<uint8_t>& data, AsyncCallback callback,
                                    unsigned int timeout_ms) {
    AsyncTransferRequest req;
    req.type = AsyncTransferType::InterruptWrite;
    req.endpoint = _out_ep;
    req.data = data;
    req.timeout_ms = timeout_ms;
    req.callback = std::move(callback);
    _engine.submit(_handle, std::move(req));
}

// 连续异步读取 HID 报告
// 自动循环提交读取请求，直到调用 stop_continuous()
// 每次读取完成后调用回调函数，然后自动提交下一次读取
void AsyncHidTransfer::read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms) {
    _continuous.store(true, std::memory_order_release);

    // 创建包装回调：用户回调 + 自动续传逻辑
    auto continuous_cb = [this, length, callback = std::move(callback), timeout_ms](TransferResult result) {
        callback(result);
        if (_continuous.load(std::memory_order_acquire)) {
            read_continuous(length, callback, timeout_ms);
        }
    };

    read_async(length, continuous_cb, timeout_ms);
}

// 停止连续读取
// 设置停止标志，下次传输完成后不会自动续传
void AsyncHidTransfer::stop_continuous() {
    _continuous.store(false, std::memory_order_release);
}

} // namespace usb_transfer
