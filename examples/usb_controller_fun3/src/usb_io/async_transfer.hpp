// ============================================================================
// async_transfer.hpp - USB 异步数据传输模块（头文件）
//
// 功能说明：
//   本文件定义异步 USB 数据传输相关的类：
//   - AsyncTransferType：异步传输类型枚举（Bulk/Interrupt 读写）
//   - AsyncTransferRequest：异步传输请求结构体（描述一次传输请求）
//   - AsyncTransferEngine：异步传输引擎（管理事件处理线程）
//   - AsyncHidTransfer：HID 异步传输类（封装单次/连续读取）
//   - AsyncCallback：异步传输完成回调类型定义
//
// 异步传输工作原理：
//   1. AsyncTransferEngine 在独立线程中循环调用 libusb_handle_events_timeout
//   2. 每次提交的传输由 libusb 异步处理，完成后调用回调函数
//   3. 回调函数在事件处理线程中执行，用户需要考虑线程安全
//
// 依赖关系：
//   - sync_transfer.hpp：TransferResult 结构体
//   - libusb.h：底层 libusb API
//   - C++11/17 标准库：<atomic>, <condition_variable>, <functional>, <mutex>,
//                         <thread>, <vector>, <memory>
// ============================================================================

#pragma once

#include "sync_transfer.hpp"
#include <atomic>           // 原子操作（线程安全标志）
#include <chrono>           // 时间相关（未直接使用，但保留）
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "libusb.h"

namespace usb_ctrl {
namespace io {

// ============================================================================
// AsyncCallback - 异步传输完成回调类型
//
// 当异步传输完成时，调用此回调函数，并传入传输结果。
// 回调函数在 AsyncTransferEngine 的事件处理线程中执行。
// ============================================================================
using AsyncCallback = std::function<void(const TransferResult&)>;

// ============================================================================
// AsyncTransferType - 异步传输类型枚举
//
// 标识异步传输的类型和方向：
//   - BulkRead    : 批量传输，设备→主机
//   - BulkWrite   : 批量传输，主机→设备
//   - InterruptRead: 中断传输，设备→主机
//   - InterruptWrite: 中断传输，主机→设备
// ============================================================================
enum class AsyncTransferType {
    BulkRead,
    BulkWrite,
    InterruptRead,
    InterruptWrite
};

// ============================================================================
// AsyncTransferRequest - 异步传输请求结构体
//
// 描述一次异步传输请求，包含传输类型、端点、数据、长度、超时、回调。
// 提交给 AsyncTransferEngine::submit() 执行。
// ============================================================================
struct AsyncTransferRequest {
    AsyncTransferType type;          // 传输类型（Bulk/Interrupt，读/写）
    unsigned char endpoint;          // 端点地址（bit7=1 表示 IN）
    std::vector<uint8_t> data;       // 写入数据（仅对写操作有效）
    int length = 0;                  // 读取长度（仅对读操作有效）
    unsigned int timeout_ms = 1000;  // 传输超时（毫秒）
    AsyncCallback callback;          // 传输完成回调函数
};

// ============================================================================
// AsyncTransferEngine - 异步传输引擎类
//
// 管理 libusb 异步事件处理：
//   1. 在独立线程中循环调用 libusb_handle_events_timeout_completed
//   2. 驱动所有已提交的异步传输完成
//   3. 每个传输完成后调用用户注册的回调函数
//
// 线程安全：
//   - start/stop 方法：线程安全（使用原子变量）
//   - submit 方法：线程安全（libusb 保证提交异步传输是线程安全的）
//
// 使用方式：
//   1. 创建 AsyncTransferEngine，传入 libusb_context
//   2. 调用 start() 启动事件处理线程
//   3. 使用 AsyncHidTransfer 提交传输请求
//   4. 传输完成后回调函数被调用
//   5. 调用 stop() 停止事件处理线程
// ============================================================================
class AsyncTransferEngine {
public:
    // 构造函数：绑定到指定的 libusb 上下文
    // @param ctx 已初始化的 libusb_context 指针
    explicit AsyncTransferEngine(libusb_context* ctx);

    // 析构函数：自动调用 stop() 停止线程并清理资源
    ~AsyncTransferEngine();

    // 禁止拷贝（引擎拥有事件线程的唯一所有权）
    AsyncTransferEngine(const AsyncTransferEngine&) = delete;
    AsyncTransferEngine& operator=(const AsyncTransferEngine&) = delete;

    // 启动事件处理线程
    // 如果已经启动，此方法不执行任何操作（幂等操作）
    void start();

    // 停止事件处理线程
    // 如果未启动，此方法不执行任何操作（幂等操作）
    // 阻塞等待线程退出后返回
    void stop();

    // 提交一个异步传输请求
    // @param handle  已打开的 libusb 设备句柄
    // @param request 传输请求（移动语义，请求会被移动）
    // @return true 表示提交成功，false 表示提交失败
    bool submit(libusb_device_handle* handle, AsyncTransferRequest&& request);

    // 检查引擎是否正在运行
    // @return true 表示事件线程正在运行
    bool is_running() const { return _running.load(std::memory_order_acquire); }

    // 获取当前待处理的传输数量
    // @return 待处理传输计数
    size_t pending_count() const { return _pending.load(std::memory_order_acquire); }

private:
    // 事件循环（在独立线程中运行）
    void _event_loop();

    // 处理传输完成（回调处理）
    void _handle_completion(libusb_transfer* transfer);

    // libusb 传输完成静态回调函数
    // 该函数由 libusb 调用，转发给 _handle_completion
    static void LIBUSB_CALL _transfer_cb(libusb_transfer* transfer);

    // ========================================================================
    // TransferContext - 传输上下文结构体
    //
    // 保存传输引擎指针和用户回调函数。
    // 此结构体通过 new 分配，在回调完成后 delete 释放。
    // ========================================================================
    struct TransferContext {
        AsyncTransferEngine* engine; // 指向异步传输引擎
        AsyncCallback callback;      // 用户回调函数
    };

    libusb_context* _ctx;                // libusb 上下文指针（不拥有所有权）
    libusb_device_handle* _handle = nullptr; // 设备句柄（当前提交的传输使用）
    std::atomic<bool> _running{false};  // 运行标志（线程安全，true 表示正在运行）
    std::thread _event_thread;          // 事件处理线程对象
    std::atomic<size_t> _pending{0};    // 待处理传输计数（线程安全）
};

// ============================================================================
// AsyncHidTransfer - HID 异步传输类
//
// 封装 HID 设备的异步数据传输操作。
// 使用 AsyncTransferEngine 驱动事件处理，支持：
//   - 单次异步读取（read_async）
//   - 单次异步写入（write_async）
//   - 连续异步读取（read_continuous）- 每次读取完成自动重新提交
//   - 停止连续读取（stop_continuous）
// ============================================================================
class AsyncHidTransfer {
public:
    // 构造函数：绑定到异步引擎、设备句柄和端点
    // @param engine  异步传输引擎引用（引擎必须已启动）
    // @param handle  已打开的 libusb 设备句柄
    // @param in_ep   IN 端点地址（设备→主机）
    // @param out_ep  OUT 端点地址（主机→设备）
    AsyncHidTransfer(AsyncTransferEngine& engine, libusb_device_handle* handle,
                     unsigned char in_ep, unsigned char out_ep);

    // 析构函数：自动停止连续读取（无需额外操作）
    ~AsyncHidTransfer() = default;

    // 禁止拷贝（引擎和句柄所有权唯一）
    AsyncHidTransfer(const AsyncHidTransfer&) = delete;
    AsyncHidTransfer& operator=(const AsyncHidTransfer&) = delete;

    // 单次异步读取（中断 IN 端点）
    // @param length     要读取的字节数
    // @param callback   读取完成回调函数
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    void read_async(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 单次异步写入（中断 OUT 端点）
    // @param data       要写入的数据
    // @param callback   写入完成回调函数
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    void write_async(const std::vector<uint8_t>& data, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 连续异步读取（自动循环读取）
    // 每次读取完成后，自动重新提交下一次读取。
    // 适用于需要持续接收数据的场景（如键盘、鼠标输入）。
    //
    // @param length     每次读取的字节数
    // @param callback   每次读取完成后的回调函数
    // @param timeout_ms 每次读取的超时时间（毫秒），默认 1000ms
    void read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 停止连续异步读取
    // 设置 _continuous 标志为 false，当前读取完成后不会重新提交。
    void stop_continuous();

private:
    AsyncTransferEngine& _engine;    // 异步传输引擎引用（不拥有所有权）
    libusb_device_handle* _handle;   // libusb 设备句柄（不拥有所有权）
    unsigned char _in_ep;            // IN 端点地址
    unsigned char _out_ep;           // OUT 端点地址
    std::atomic<bool> _continuous{false}; // 连续读取标志（线程安全）
};

} // namespace io
} // namespace usb_ctrl
