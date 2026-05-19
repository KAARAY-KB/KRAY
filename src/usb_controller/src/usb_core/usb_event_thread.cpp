#include "usb_event_thread.hpp"
#include "libusb.h"

namespace usb_ctrl {
namespace core {

UsbEventThread::UsbEventThread(libusb_context* ctx) : _ctx(ctx) {}

UsbEventThread::~UsbEventThread() { stop(); }

void UsbEventThread::start() {
    if (_running.load(std::memory_order_acquire)) return;
    _running.store(true, std::memory_order_release);
    _thread = std::thread(&UsbEventThread::_event_loop, this);
}

void UsbEventThread::stop() {
    if (!_running.load(std::memory_order_acquire)) return;
    _running.store(false, std::memory_order_release);
    if (_thread.joinable()) _thread.join();
}

void UsbEventThread::_event_loop() {
    while (_running.load(std::memory_order_acquire)) {
        struct timeval tv = {0, 100000};
        int rc = libusb_handle_events_timeout(_ctx, &tv);
        if (rc < 0 && _running.load(std::memory_order_acquire)) break;
    }
}

} // namespace core
} // namespace usb_ctrl
