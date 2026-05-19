// ============================================================================
// usb_event_thread.hpp - libusb 事件处理线程模块（头文件）
//
// 功能说明：
//   定义 UsbEventThread 类，将 libusb 事件处理循环封装为独立的可重用线程。
//   从 AsyncTransfer 中分离出来，使得事件线程可以被多个模块共享。
//
// 工作方式：
//   1. 构造时绑定 libusb_context
//   2. start() 创建独立线程，循环调用 libusb_handle_events_timeout
//   3. stop() 停止线程（析构时自动调用）
//   4. 线程驱动所有通过 libusb_submit_transfer 提交的异步传输完成
//
// 使用示例：
//   UsbEventThread event_thread(ctx);
//   event_thread.start();
//   // ... 提交异步传输 ...
//   event_thread.stop();
//
// 线程安全：
//   - start/stop：使用原子变量保证线程安全
//   - 一个 UsbEventThread 可以驱动多个 AsyncTransfer 的传输
// ============================================================================

#pragma once

#include <atomic>
#include <thread>

struct libusb_context;

namespace usb_ctrl {
namespace core {

class UsbEventThread {
public:
    // 构造函数：绑定 libusb 上下文
    // @param ctx 已初始化的 libusb_context 指针
    explicit UsbEventThread(libusb_context* ctx);

    // 析构函数：自动停止事件处理线程
    ~UsbEventThread();

    // 禁止拷贝和移动（线程所有权唯一）
    UsbEventThread(const UsbEventThread&) = delete;
    UsbEventThread& operator=(const UsbEventThread&) = delete;
    UsbEventThread(UsbEventThread&&) = delete;
    UsbEventThread& operator=(UsbEventThread&&) = delete;

    // 启动事件处理线程（幂等操作）
    void start();

    // 停止事件处理线程（阻塞等待线程退出）
    void stop();

    // 检查事件处理线程是否正在运行
    bool is_running() const { return _running.load(std::memory_order_acquire); }

    // 获取 libusb 上下文指针
    libusb_context* context() const { return _ctx; }

private:
    // 事件处理循环（在独立线程中运行）
    void _event_loop();

    libusb_context* _ctx = nullptr;          // libusb 上下文指针（不拥有所有权）
    std::atomic<bool> _running{false};       // 运行标志（线程安全）
    std::thread _thread;                     // 事件处理线程对象
};

} // namespace core
} // namespace usb_ctrl
