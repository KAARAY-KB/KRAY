#include "usb_context.hpp"
#include "libusb.h"

namespace usb_ctrl {
namespace core {

UsbContext::UsbContext() {
    int rc = libusb_init(&_ctx);
    if (rc < 0) {
        throw UsbException(
            std::string("libusb_init: ") + libusb_error_name(rc), rc);
    }
}

UsbContext::~UsbContext() {
    if (_ctx) {
        libusb_exit(_ctx);
    }
}

void UsbContext::set_debug_level(int level) {
    libusb_set_debug(_ctx, level);
}

} // namespace core
} // namespace usb_ctrl
