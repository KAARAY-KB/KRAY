#pragma once

#include "usb_transfer.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "libusb.h"

namespace usb_ctrl {
namespace transfer {

using AsyncCallback = std::function<void(const TransferResult&)>;

enum class AsyncTransferType {
    BulkRead,
    BulkWrite,
    InterruptRead,
    InterruptWrite
};

struct AsyncTransferRequest {
    AsyncTransferType type;
    unsigned char endpoint;
    std::vector<uint8_t> data;
    int length = 0;
    unsigned int timeout_ms = 1000;
    AsyncCallback callback;
};

class AsyncTransferEngine {
public:
    explicit AsyncTransferEngine(libusb_context* ctx);
    ~AsyncTransferEngine();

    AsyncTransferEngine(const AsyncTransferEngine&) = delete;
    AsyncTransferEngine& operator=(const AsyncTransferEngine&) = delete;

    void start();
    void stop();
    bool submit(libusb_device_handle* handle, AsyncTransferRequest&& request);
    bool is_running() const { return _running.load(std::memory_order_acquire); }
    size_t pending_count() const { return _pending.load(std::memory_order_acquire); }

private:
    void _event_loop();
    void _handle_completion(libusb_transfer* transfer);
    static void LIBUSB_CALL _transfer_cb(libusb_transfer* transfer);

    struct TransferContext {
        AsyncTransferEngine* engine;
        AsyncCallback callback;
    };

    libusb_context* _ctx;
    libusb_device_handle* _handle = nullptr;
    std::atomic<bool> _running{false};
    std::thread _event_thread;
    std::atomic<size_t> _pending{0};
};

class AsyncHidTransfer {
public:
    AsyncHidTransfer(AsyncTransferEngine& engine, libusb_device_handle* handle,
                     unsigned char in_ep, unsigned char out_ep);

    void read_async(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);
    void write_async(const std::vector<uint8_t>& data, AsyncCallback callback, unsigned int timeout_ms = 1000);
    void read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);
    void stop_continuous();

private:
    AsyncTransferEngine& _engine;
    libusb_device_handle* _handle;
    unsigned char _in_ep;
    unsigned char _out_ep;
    std::atomic<bool> _continuous{false};
};

} // namespace transfer
} // namespace usb_ctrl
