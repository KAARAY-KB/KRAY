#include "async_transfer.hpp"
#include <iostream>
#include <cstring>

namespace usb_ctrl {
namespace transfer {

AsyncTransferEngine::AsyncTransferEngine(libusb_context* ctx) : _ctx(ctx) {}

AsyncTransferEngine::~AsyncTransferEngine() { stop(); }

void AsyncTransferEngine::start() {
    if (_running.load(std::memory_order_acquire)) return;
    _running.store(true, std::memory_order_release);
    _event_thread = std::thread(&AsyncTransferEngine::_event_loop, this);
}

void AsyncTransferEngine::stop() {
    if (!_running.load(std::memory_order_acquire)) return;
    _running.store(false, std::memory_order_release);
    if (_event_thread.joinable()) _event_thread.join();
}

bool AsyncTransferEngine::submit(libusb_device_handle* handle, AsyncTransferRequest&& request) {
    if (!_running.load(std::memory_order_acquire)) return false;
    _handle = handle;

    auto* transfer = libusb_alloc_transfer(0);
    if (!transfer) return false;

    auto* ctx = new TransferContext{this, std::move(request.callback)};
    transfer->user_data = ctx;

    unsigned char* buf = nullptr;
    if (request.type == AsyncTransferType::BulkWrite || request.type == AsyncTransferType::InterruptWrite) {
        buf = new unsigned char[request.data.size()];
        std::memcpy(buf, request.data.data(), request.data.size());
    } else {
        buf = new unsigned char[request.length];
    }

    switch (request.type) {
        case AsyncTransferType::BulkRead:
            libusb_fill_bulk_transfer(transfer, handle, request.endpoint, buf, request.length, _transfer_cb, ctx, request.timeout_ms);
            break;
        case AsyncTransferType::BulkWrite:
            libusb_fill_bulk_transfer(transfer, handle, request.endpoint, buf, static_cast<int>(request.data.size()), _transfer_cb, ctx, request.timeout_ms);
            break;
        case AsyncTransferType::InterruptRead:
            libusb_fill_interrupt_transfer(transfer, handle, request.endpoint, buf, request.length, _transfer_cb, ctx, request.timeout_ms);
            break;
        case AsyncTransferType::InterruptWrite:
            libusb_fill_interrupt_transfer(transfer, handle, request.endpoint, buf, static_cast<int>(request.data.size()), _transfer_cb, ctx, request.timeout_ms);
            break;
    }

    transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
    _pending.fetch_add(1, std::memory_order_release);

    int rc = libusb_submit_transfer(transfer);
    if (rc < 0) {
        delete[] buf;
        transfer->buffer = nullptr;
        delete ctx;
        libusb_free_transfer(transfer);
        _pending.fetch_sub(1, std::memory_order_release);
        return false;
    }
    return true;
}

void AsyncTransferEngine::_event_loop() {
    while (_running.load(std::memory_order_acquire)) {
        struct timeval tv = {0, 100000};
        int rc = libusb_handle_events_timeout(_ctx, &tv);
        if (rc < 0 && _running.load(std::memory_order_acquire)) break;
    }
}

void LIBUSB_CALL AsyncTransferEngine::_transfer_cb(libusb_transfer* transfer) {
    auto* ctx = static_cast<TransferContext*>(transfer->user_data);
    if (!ctx || !ctx->engine) return;

    TransferResult result;
    result.success = (transfer->status == LIBUSB_TRANSFER_COMPLETED);
    result.bytes_transferred = transfer->actual_length;
    result.error_code = static_cast<int>(transfer->status);

    if (!result.success) {
        switch (transfer->status) {
            case LIBUSB_TRANSFER_ERROR:     result.error_message = "Transfer error"; break;
            case LIBUSB_TRANSFER_TIMED_OUT: result.error_message = "Timed out"; break;
            case LIBUSB_TRANSFER_CANCELLED: result.error_message = "Cancelled"; break;
            case LIBUSB_TRANSFER_STALL:     result.error_message = "Stalled"; break;
            case LIBUSB_TRANSFER_NO_DEVICE: result.error_message = "Device disconnected"; break;
            case LIBUSB_TRANSFER_OVERFLOW:  result.error_message = "Overflow"; break;
            default:                        result.error_message = "Unknown error"; break;
        }
    }

    if (result.success && result.bytes_transferred > 0 && transfer->buffer)
        result.data.assign(transfer->buffer, transfer->buffer + result.bytes_transferred);

    if (ctx->callback) ctx->callback(result);

    ctx->engine->_pending.fetch_sub(1, std::memory_order_release);
    libusb_free_transfer(transfer);
    delete ctx;
}

AsyncHidTransfer::AsyncHidTransfer(AsyncTransferEngine& engine, libusb_device_handle* handle,
                                     unsigned char in_ep, unsigned char out_ep)
    : _engine(engine), _handle(handle), _in_ep(in_ep), _out_ep(out_ep) {}

void AsyncHidTransfer::read_async(int length, AsyncCallback callback, unsigned int timeout_ms) {
    AsyncTransferRequest req;
    req.type = AsyncTransferType::InterruptRead;
    req.endpoint = _in_ep;
    req.length = length;
    req.timeout_ms = timeout_ms;
    req.callback = std::move(callback);
    _engine.submit(_handle, std::move(req));
}

void AsyncHidTransfer::write_async(const std::vector<uint8_t>& data, AsyncCallback callback, unsigned int timeout_ms) {
    AsyncTransferRequest req;
    req.type = AsyncTransferType::InterruptWrite;
    req.endpoint = _out_ep;
    req.data = data;
    req.timeout_ms = timeout_ms;
    req.callback = std::move(callback);
    _engine.submit(_handle, std::move(req));
}

void AsyncHidTransfer::read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms) {
    _continuous.store(true, std::memory_order_release);
    auto continuous_cb = [this, length, callback = std::move(callback), timeout_ms](const TransferResult& result) {
        callback(result);
        if (_continuous.load(std::memory_order_acquire))
            read_continuous(length, callback, timeout_ms);
    };
    read_async(length, continuous_cb, timeout_ms);
}

void AsyncHidTransfer::stop_continuous() {
    _continuous.store(false, std::memory_order_release);
}

} // namespace transfer
} // namespace usb_ctrl
