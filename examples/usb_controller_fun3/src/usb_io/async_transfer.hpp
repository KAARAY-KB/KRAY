// ============================================================================
// async_transfer.hpp - USB 异步数据传输模块（头文件）
//
// 功能说明：
//   本文件定义异步 USB 数据传输相关的类型和类：
//   - AsyncCallback       ：异步传输完成回调类型定义
//   - AsyncTransferType   ：异步传输类型枚举（Bulk/Interrupt 读写）
//   - AsyncTransferRequest：异步传输请求结构体（描述一次传输请求）
//   - AsyncTransfer       ：异步传输类（单类，合并引擎管理和传输操作）
//
// 异步传输工作原理：
//   1. AsyncTransfer 在独立线程中循环调用 libusb_handle_events_timeout
//   2. 每次提交的传输由 libusb 异步处理，完成后调用回调函数
//   3. 回调函数在事件处理线程中执行，用户需要考虑线程安全
//
// 与 SyncTransfer 的对比：
//   SyncTransfer：单类，同步阻塞，简单直接
//   AsyncTransfer：单类，异步非阻塞，支持连续读取
//
// 依赖关系：
//   - sync_transfer.hpp：TransferResult 结构体
//   - libusb.h：底层 libusb API
//   - C++11/17 标准库：<atomic>, <functional>, <thread>, <vector>
// ============================================================================

#pragma once

#include "sync_transfer.hpp"
#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include "libusb.h"

namespace usb_ctrl {
namespace io {

// ============================================================================
// AsyncCallback - 异步传输完成回调类型
//
// 当异步传输完成时，调用此回调函数，并传入传输结果。
// 回调函数在 AsyncTransfer 的事件处理线程中执行。
// ============================================================================
using AsyncCallback = std::function<void(const TransferResult&)>;

// ============================================================================
// AsyncTransferType - 异步传输类型枚举
//
// 定义支持的异步传输类型，对应 libusb 的批量传输和中断传输。
// 每种类型分为读（设备→主机）和写（主机→设备）两个方向。
// ============================================================================
enum class AsyncTransferType {
    BulkRead,       // 批量读取（Bulk IN）
    BulkWrite,      // 批量写入（Bulk OUT）
    InterruptRead,  // 中断读取（Interrupt IN）
    InterruptWrite, // 中断写入（Interrupt OUT）
};

// ============================================================================
// AsyncTransferRequest - 异步传输请求结构体
//
// 描述一次异步传输请求的完整信息，包括传输类型、端点、数据、回调和超时。
// 提交给 AsyncTransfer::submit() 执行。
// ============================================================================
struct AsyncTransferRequest {
    AsyncTransferType type;          // 传输类型（Bulk/Interrupt，读/写）
    unsigned char endpoint = 0;      // 端点地址
    std::vector<uint8_t> data;       // 写入数据（仅 Write 类型使用）
    int length = 0;                  // 读取长度（仅 Read 类型使用）
    AsyncCallback callback;          // 完成回调函数
    unsigned int timeout_ms = 1000;  // 超时时间（毫秒）
};

// ============================================================================
// AsyncTransfer - USB 异步数据传输类（单类）
//
// 封装 libusb 的异步传输 API，提供类型安全的 C++ 接口。
// 将事件处理引擎和传输操作合并为一个类，简化使用。
//
// 功能：
//   - 事件引擎管理：start/stop 控制事件处理线程
//   - 通用异步提交：submit() 支持 Bulk/Interrupt 读写
//   - 设备绑定：bind_device 绑定要操作的 HID 设备
//   - HID 便捷方法：read_async / write_async / read_continuous
//
// 使用示例（通用方式）：
//   AsyncTransfer async(ctx);
//   async.start();
//   AsyncTransferRequest req;
//   req.type = AsyncTransferType::InterruptRead;
//   req.endpoint = 0x81;
//   req.length = 64;
//   req.callback = [](const TransferResult& r) { ... };
//   async.submit(handle, std::move(req));
//   async.stop();
//
// 使用示例（HID 便捷方式）：
//   AsyncTransfer async(ctx);
//   async.start();
//   async.bind_device(handle, 0x81, 0x01);
//   async.read_async(64, [](const TransferResult& r) { ... });
//   async.stop();
//
// 线程安全：
//   - start/stop：线程安全（使用原子变量）
//   - submit/read_async/write_async：线程安全（libusb 保证提交异步传输是线程安全的）
//   - 回调在事件线程中执行，用户需自行处理线程安全
// ============================================================================
class AsyncTransfer {
public:
    // 构造函数：绑定到指定的 libusb 上下文
    // @param ctx 已初始化的 libusb_context 指针
    explicit AsyncTransfer(libusb_context* ctx);

    // 析构函数：自动调用 stop() 停止线程并清理资源
    ~AsyncTransfer();

    // 禁止拷贝（引擎拥有事件线程的唯一所有权）
    AsyncTransfer(const AsyncTransfer&) = delete;
    AsyncTransfer& operator=(const AsyncTransfer&) = delete;

    // ========================================================================
    // 引擎管理
    // ========================================================================

    // 启动事件处理线程
    // 如果已经启动，此方法不执行任何操作（幂等操作）
    void start();

    // 停止事件处理线程
    // 如果未启动，此方法不执行任何操作（幂等操作）
    // 阻塞等待线程退出后返回
    void stop();

    // 检查引擎是否正在运行
    // @return true 表示事件线程正在运行
    bool is_running() const { return _running.load(std::memory_order_acquire); }

    // 获取当前待处理的传输数量
    // @return 待处理传输计数
    size_t pending_count() const { return _pending.load(std::memory_order_acquire); }

    // ========================================================================
    // 通用异步提交
    // ========================================================================

    // 提交异步传输请求（通用接口）
    // 支持 BulkRead/BulkWrite/InterruptRead/InterruptWrite 四种类型。
    // 提交成功后，传输由事件线程异步驱动，完成后调用请求中的回调函数。
    //
    // @param handle  已打开的 libusb 设备句柄
    // @param request 传输请求（包含类型、端点、数据、回调等）
    // @return true 表示提交成功，false 表示提交失败
    bool submit(libusb_device_handle* handle, AsyncTransferRequest&& request);

    // ========================================================================
    // 设备绑定（HID 便捷方法使用）
    // ========================================================================

    // 绑定要操作的 HID 设备
    // 设置设备句柄和端点地址，后续的 read_async/write_async 将操作此设备。
    // @param handle 已打开的 libusb 设备句柄
    // @param in_ep  IN 端点地址（设备→主机）
    // @param out_ep OUT 端点地址（主机→设备）
    void bind_device(libusb_device_handle* handle, unsigned char in_ep, unsigned char out_ep);

    // 检查设备是否已绑定
    // @return true 表示设备已绑定
    bool is_bound() const { return _handle != nullptr; }

    // ========================================================================
    // HID 便捷方法（自动使用绑定的设备和端点）
    // ========================================================================

    // 单次异步读取（中断 IN 端点）
    // 提交一次中断读取请求，完成后调用回调函数。
    // @param length     要读取的字节数
    // @param callback   读取完成回调函数
    // @param timeout_ms 超时时间（毫秒），默认 1000ms
    void read_async(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 单次异步写入（中断 OUT 端点）
    // 提交一次中断写入请求，完成后调用回调函数。
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
    // 设置连续读取标志为 false，当前读取完成后不会重新提交。
    void stop_continuous();

private:
    // ========================================================================
    // 内部方法
    // ========================================================================

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
        AsyncTransfer* engine;   // 指向异步传输引擎
        AsyncCallback callback;  // 用户回调函数
    };

    // ========================================================================
    // 成员变量
    // ========================================================================

    libusb_context* _ctx = nullptr;          // libusb 上下文指针（不拥有所有权）
    libusb_device_handle* _handle = nullptr; // 设备句柄（不拥有所有权，bind_device 设置）
    unsigned char _in_ep = 0;                // IN 端点地址（bind_device 设置）
    unsigned char _out_ep = 0;               // OUT 端点地址（bind_device 设置）
    std::atomic<bool> _running{false};       // 运行标志（线程安全）
    std::thread _event_thread;               // 事件处理线程对象
    std::atomic<size_t> _pending{0};         // 待处理传输计数（线程安全）
    std::atomic<bool> _continuous{false};    // 连续读取标志（线程安全）
};

} // namespace io
} // namespace usb_ctrl
