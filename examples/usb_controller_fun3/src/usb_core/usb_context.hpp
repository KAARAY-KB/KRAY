#pragma once

#include <memory>
#include <string>
#include <stdexcept>

struct libusb_context;

namespace usb_ctrl {
namespace core {

class UsbException : public std::runtime_error {
public:
    UsbException(const std::string& msg, int code = 0)
        : std::runtime_error(msg), _code(code) {}
    int code() const { return _code; }
private:
    int _code;
};

class UsbContext {
public:
    UsbContext();
    ~UsbContext();

    UsbContext(const UsbContext&) = delete;
    UsbContext& operator=(const UsbContext&) = delete;
    UsbContext(UsbContext&&) = delete;
    UsbContext& operator=(UsbContext&&) = delete;

    libusb_context* handle() const { return _ctx; }
    void set_debug_level(int level);

private:
    libusb_context* _ctx = nullptr;
};

} // namespace core
} // namespace usb_ctrl
