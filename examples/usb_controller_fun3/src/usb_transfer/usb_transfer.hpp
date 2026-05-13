#pragma once

#include <vector>
#include <cstdint>
#include <string>

struct libusb_device_handle;

namespace usb_ctrl {
namespace transfer {

struct TransferResult {
    bool success = false;
    int bytes_transferred = 0;
    int error_code = 0;
    std::string error_message;
    std::vector<uint8_t> data;
};

class SyncTransfer {
public:
    explicit SyncTransfer(libusb_device_handle* handle);

    SyncTransfer(const SyncTransfer&) = delete;
    SyncTransfer& operator=(const SyncTransfer&) = delete;

    libusb_device_handle* handle() const { return _handle; }

    TransferResult bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);
    TransferResult bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);
    TransferResult interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);
    TransferResult interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);
    TransferResult control_transfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                                     const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);
    TransferResult control_read(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                                 uint16_t wLength, unsigned int timeout_ms = 1000);

private:
    libusb_device_handle* _handle;
};

std::string bytes_to_hex(const std::vector<uint8_t>& data, size_t max_bytes = 64);
std::string bytes_to_ascii(const std::vector<uint8_t>& data, size_t max_bytes = 64);
std::string format_transfer_result(const TransferResult& r, size_t max_hex = 64);

} // namespace transfer
} // namespace usb_ctrl
