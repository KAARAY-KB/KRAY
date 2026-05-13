// ============================================================================
// async_transfer.cpp - USB 异步数据传输模块（实现文件）
//
// 功能说明：
//   实现异步传输引擎和 HID 异步传输类的所有方法：
//   - AsyncTransferEngine：事件处理线程管理、传输提交、回调处理
//   - AsyncHidTransfer：单次/连续异步读写
//
// 异步传输工作流程：
//   1. 用户调用 AsyncHidTransfer::read_async() 提交读取请求
//   2. AsyncHidTransfer 构造 AsyncTransferRequest 并提交给引擎
//   3. AsyncTransferEngine::submit() 分配 libusb_transfer 并提交
//   4. 引擎的事件线程调用 libusb_handle_events_timeout 驱动传输
//   5. 传输完成后 libusb 调用 _transfer_cb 静态回调
//   6. _transfer_cb 解析结果并调用用户回调函数
//   7. 清理 libusb_transfer 和 TransferContext 资源
//
// 线程模型：
//   - 主线程：用户代码，调用 read_async/write_async 提交请求
//   - 事件线程：AsyncTransferEngine 内部线程，处理 libusb 事件
//   - 回调在事件线程中执行，用户需自行处理线程安全
// ============================================================================

#include "async_transfer.hpp"
#include <iostream>
#include <cstring>

namespace usb_ctrl {
namespace transfer {

// ============================================================================
// AsyncTransferEngine 构造函数
//
// 保存 libusb 上下文指针。注意：引擎不拥有上下文的所有权，
// 调用者需确保上下文在引擎使用期间保持有效。
//
// @param ctx 已初始化的 libusb_context 指针
// ============================================================================
AsyncTransferEngine::AsyncTransferEngine(libusb_context* ctx) : _ctx(ctx) {}

// ============================================================================
// AsyncTransferEngine 析构函数
//
// 自动调用 stop() 停止事件处理线程，确保线程安全退出。
// ============================================================================
AsyncTransferEngine::~AsyncTransferEngine() { stop(); }

// ============================================================================
// start - 启动事件处理线程
//
// 创建并启动一个独立线程，在其中循环调用 libusb_handle_events_timeout。
// 如果引擎已经在运行，此方法不执行任何操作（幂等操作）。
//
// 线程安全：使用 atomic 变量 _running 控制线程生命周期。
// ============================================================================
void AsyncTransferEngine::start() {
    // 如果已经在运行，直接返回（幂等操作）
    if (_running.load(std::memory_order_acquire)) return;
    // 设置运行标志为 true（release 语义确保后续操作可见）
    _running.store(true, std::memory_order_release);
    // 创建事件处理线程，执行 _event_loop 成员函数
    _event_thread = std::thread(&AsyncTransferEngine::_event_loop, this);
}

// ============================================================================
// stop - 停止事件处理线程
//
// 设置 _running 标志为 false，等待事件线程退出。
// 如果引擎未运行，此方法不执行任何操作（幂等操作）。
//
// 线程安全：使用 atomic 变量通知线程退出，join() 等待线程结束。
// ============================================================================
void AsyncTransferEngine::stop() {
    // 如果未运行，直接返回（幂等操作）
    if (!_running.load(std::memory_order_acquire)) return;
    // 设置运行标志为 false（release 语义确保线程可见）
    _running.store(false, std::memory_order_release);
    // 等待事件处理线程退出
    if (_event_thread.joinable()) _event_thread.join();
}

// ============================================================================
// submit - 提交异步传输请求
//
// 分配并配置 libusb_transfer 结构体，提交给 libusb 异步处理。
// 传输完成后，libusb 会调用 _transfer_cb 静态回调函数。
//
// 内存管理：
//   - TransferContext 通过 new 分配，在回调中 delete 释放
//   - 数据缓冲区通过 new[] 分配，设置 LIBUSB_TRANSFER_FREE_BUFFER 标志
//     让 libusb 在释放 transfer 时自动释放缓冲区
//
// @param handle  已打开的 libusb 设备句柄
// @param request 传输请求（移动语义）
// @return true 表示提交成功，false 表示提交失败
// ============================================================================
bool AsyncTransferEngine::submit(libusb_device_handle* handle, AsyncTransferRequest&& request) {
    // 检查引擎是否在运行
    if (!_running.load(std::memory_order_acquire)) return false;
    // 保存设备句柄
    _handle = handle;

    // 分配 libusb_transfer 结构体（参数 0 表示同步数据包数为 0）
    auto* transfer = libusb_alloc_transfer(0);
    // 分配失败：返回 false
    if (!transfer) return false;

    // 创建传输上下文（保存引擎指针和用户回调）
    auto* ctx = new TransferContext{this, std::move(request.callback)};
    // 将上下文指针保存到 transfer->user_data，回调时取出
    transfer->user_data = ctx;

    // 分配数据缓冲区
    unsigned char* buf = nullptr;
    // 写操作：分配缓冲区并拷贝用户数据
    if (request.type == AsyncTransferType::BulkWrite || request.type == AsyncTransferType::InterruptWrite) {
        buf = new unsigned char[request.data.size()];
        std::memcpy(buf, request.data.data(), request.data.size()); // 拷贝数据
    } else {
        // 读操作：分配指定长度的空缓冲区
        buf = new unsigned char[request.length];
    }

    // 根据传输类型填充 libusb_transfer 结构体
    switch (request.type) {
        case AsyncTransferType::BulkRead:
            // 批量读取：填充批量传输参数
            libusb_fill_bulk_transfer(transfer, handle, request.endpoint, buf, request.length, _transfer_cb, ctx, request.timeout_ms);
            break;
        case AsyncTransferType::BulkWrite:
            // 批量写入：填充批量传输参数
            libusb_fill_bulk_transfer(transfer, handle, request.endpoint, buf, static_cast<int>(request.data.size()), _transfer_cb, ctx, request.timeout_ms);
            break;
        case AsyncTransferType::InterruptRead:
            // 中断读取：填充中断传输参数
            libusb_fill_interrupt_transfer(transfer, handle, request.endpoint, buf, request.length, _transfer_cb, ctx, request.timeout_ms);
            break;
        case AsyncTransferType::InterruptWrite:
            // 中断写入：填充中断传输参数
            libusb_fill_interrupt_transfer(transfer, handle, request.endpoint, buf, static_cast<int>(request.data.size()), _transfer_cb, ctx, request.timeout_ms);
            break;
    }

    // 设置标志：libusb 在释放 transfer 时自动释放缓冲区
    transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
    // 增加待处理传输计数
    _pending.fetch_add(1, std::memory_order_release);

    // 提交传输给 libusb 异步处理
    int rc = libusb_submit_transfer(transfer);
    // 提交失败：清理已分配的资源
    if (rc < 0) {
        delete[] buf;           // 释放数据缓冲区
        transfer->buffer = nullptr; // 防止 libusb_free_transfer 重复释放
        delete ctx;             // 释放传输上下文
        libusb_free_transfer(transfer); // 释放 transfer 结构体
        _pending.fetch_sub(1, std::memory_order_release); // 减少计数
        return false;
    }
    return true;
}

// ============================================================================
// _event_loop - 事件处理循环（在独立线程中运行）
//
// 循环调用 libusb_handle_events_timeout 驱动异步传输完成。
// 每次循环等待最多 100ms（0.1 秒），以便及时响应 _running 标志变化。
// 当 _running 变为 false 时退出循环。
// ============================================================================
void AsyncTransferEngine::_event_loop() {
    // 循环直到 _running 变为 false
    while (_running.load(std::memory_order_acquire)) {
        // 设置超时时间为 100ms（0.1 秒）
        struct timeval tv = {0, 100000};
        // 调用 libusb 事件处理，驱动异步传输完成
        int rc = libusb_handle_events_timeout(_ctx, &tv);
        // 如果发生错误且引擎仍在运行，退出循环
        if (rc < 0 && _running.load(std::memory_order_acquire)) break;
    }
}

// ============================================================================
// _transfer_cb - libusb 传输完成静态回调函数
//
// 由 libusb 在传输完成时调用（在事件处理线程中）。
// 从 transfer->user_data 中取出 TransferContext，解析传输结果，
// 调用用户回调函数，最后清理资源。
//
// @param transfer 已完成的 libusb_transfer 结构体
// ============================================================================
void LIBUSB_CALL AsyncTransferEngine::_transfer_cb(libusb_transfer* transfer) {
    // 从 user_data 中取出传输上下文
    auto* ctx = static_cast<TransferContext*>(transfer->user_data);
    // 安全检查：上下文或引擎指针为空则直接返回
    if (!ctx || !ctx->engine) return;

    // 创建传输结果结构体
    TransferResult result;
    // 判断传输是否成功（LIBUSB_TRANSFER_COMPLETED 表示成功）
    result.success = (transfer->status == LIBUSB_TRANSFER_COMPLETED);
    // 记录实际传输的字节数
    result.bytes_transferred = transfer->actual_length;
    // 记录 libusb 状态码
    result.error_code = static_cast<int>(transfer->status);

    // 传输失败时：根据状态码设置错误消息
    if (!result.success) {
        switch (transfer->status) {
            case LIBUSB_TRANSFER_ERROR:     result.error_message = "Transfer error"; break;     // 传输错误
            case LIBUSB_TRANSFER_TIMED_OUT: result.error_message = "Timed out"; break;          // 超时
            case LIBUSB_TRANSFER_CANCELLED: result.error_message = "Cancelled"; break;          // 已取消
            case LIBUSB_TRANSFER_STALL:     result.error_message = "Stalled"; break;            // 端点停止
            case LIBUSB_TRANSFER_NO_DEVICE: result.error_message = "Device disconnected"; break; // 设备断开
            case LIBUSB_TRANSFER_OVERFLOW:  result.error_message = "Overflow"; break;           // 数据溢出
            default:                        result.error_message = "Unknown error"; break;      // 未知错误
        }
    }

    // 传输成功且有数据：将缓冲区数据拷贝到结果中
    if (result.success && result.bytes_transferred > 0 && transfer->buffer)
        result.data.assign(transfer->buffer, transfer->buffer + result.bytes_transferred);

    // 调用用户回调函数（在事件线程中执行）
    if (ctx->callback) ctx->callback(result);

    // 减少待处理传输计数
    ctx->engine->_pending.fetch_sub(1, std::memory_order_release);
    // 释放 libusb_transfer 结构体（同时释放缓冲区，因为设置了 FREE_BUFFER 标志）
    libusb_free_transfer(transfer);
    // 释放传输上下文
    delete ctx;
}

// ============================================================================
// AsyncHidTransfer 构造函数
//
// 保存异步引擎引用、设备句柄和端点地址。
// 注意：AsyncHidTransfer 不拥有引擎和句柄的所有权。
//
// @param engine  异步传输引擎引用
// @param handle  已打开的 libusb 设备句柄
// @param in_ep   IN 端点地址
// @param out_ep  OUT 端点地址
// ============================================================================
AsyncHidTransfer::AsyncHidTransfer(AsyncTransferEngine& engine, libusb_device_handle* handle,
                                     unsigned char in_ep, unsigned char out_ep)
    : _engine(engine), _handle(handle), _in_ep(in_ep), _out_ep(out_ep) {}

// ============================================================================
// read_async - 单次异步读取（中断 IN 端点）
//
// 构造 AsyncTransferRequest 并提交给引擎。
// 传输完成后，用户回调函数在事件线程中被调用。
//
// @param length     要读取的字节数
// @param callback   读取完成回调函数
// @param timeout_ms 超时时间（毫秒）
// ============================================================================
void AsyncHidTransfer::read_async(int length, AsyncCallback callback, unsigned int timeout_ms) {
    // 构造异步传输请求
    AsyncTransferRequest req;
    req.type = AsyncTransferType::InterruptRead; // 中断读取
    req.endpoint = _in_ep;                       // IN 端点地址
    req.length = length;                         // 读取长度
    req.timeout_ms = timeout_ms;                 // 超时时间
    req.callback = std::move(callback);          // 移动回调函数
    // 提交给异步引擎处理
    _engine.submit(_handle, std::move(req));
}

// ============================================================================
// write_async - 单次异步写入（中断 OUT 端点）
//
// 构造 AsyncTransferRequest 并提交给引擎。
// 传输完成后，用户回调函数在事件线程中被调用。
//
// @param data       要写入的数据
// @param callback   写入完成回调函数
// @param timeout_ms 超时时间（毫秒）
// ============================================================================
void AsyncHidTransfer::write_async(const std::vector<uint8_t>& data, AsyncCallback callback, unsigned int timeout_ms) {
    // 构造异步传输请求
    AsyncTransferRequest req;
    req.type = AsyncTransferType::InterruptWrite; // 中断写入
    req.endpoint = _out_ep;                       // OUT 端点地址
    req.data = data;                              // 拷贝写入数据
    req.timeout_ms = timeout_ms;                  // 超时时间
    req.callback = std::move(callback);           // 移动回调函数
    // 提交给异步引擎处理
    _engine.submit(_handle, std::move(req));
}

// ============================================================================
// read_continuous - 连续异步读取（自动循环读取）
//
// 设置 _continuous 标志为 true，然后提交第一次读取。
// 每次读取完成后，如果 _continuous 仍为 true，自动重新提交下一次读取。
// 这形成了一个递归调用链：回调 → 检查标志 → 重新提交 → 回调 → ...
//
// 注意：由于回调在事件线程中执行，递归深度受限于事件循环的迭代次数，
// 不会导致栈溢出（每次回调都是独立的事件循环迭代）。
//
// @param length     每次读取的字节数
// @param callback   每次读取完成后的回调函数
// @param timeout_ms 每次读取的超时时间（毫秒）
// ============================================================================
void AsyncHidTransfer::read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms) {
    // 设置连续读取标志为 true
    _continuous.store(true, std::memory_order_release);
    // 创建包装回调：先调用用户回调，再检查是否需要继续读取
    auto continuous_cb = [this, length, callback = std::move(callback), timeout_ms](const TransferResult& result) {
        // 调用用户回调函数
        callback(result);
        // 如果连续读取标志仍为 true，重新提交下一次读取
        if (_continuous.load(std::memory_order_acquire))
            read_continuous(length, callback, timeout_ms);
    };
    // 提交第一次异步读取
    read_async(length, continuous_cb, timeout_ms);
}

// ============================================================================
// stop_continuous - 停止连续异步读取
//
// 设置 _continuous 标志为 false。
// 当前正在进行的读取完成后，不会重新提交下一次读取。
// ============================================================================
void AsyncHidTransfer::stop_continuous() {
    // 设置连续读取标志为 false（release 语义确保事件线程可见）
    _continuous.store(false, std::memory_order_release);
}

} // namespace transfer
} // namespace usb_ctrl
