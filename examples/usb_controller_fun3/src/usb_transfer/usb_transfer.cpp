#include "usb_transfer.hpp"
#include "libusb.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace usb_ctrl {
namespace transfer {

SyncTransfer::SyncTransfer(libusb_device_handle* handle) : _handle(handle) {}

TransferResult SyncTransfer::bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    TransferResult result;
    result.data.resize(length);
    int transferred = 0;
    int rc = libusb_bulk_transfer(_handle, endpoint, result.data.data(), length, &transferred, timeout_ms);
    if (rc == 0 || rc == LIBUSB_ERROR_TIMEOUT) {
        result.success = true;
        result.bytes_transferred = transferred;
        result.data.resize(transferred > 0 ? transferred : 0);
    } else {
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

TransferResult SyncTransfer::bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    TransferResult result;
    int transferred = 0;
    int rc = libusb_bulk_transfer(_handle, endpoint,
                                   const_cast<unsigned char*>(data.data()),
                                   static_cast<int>(data.size()), &transferred, timeout_ms);
    if (rc == 0) {
        result.success = true;
        result.bytes_transferred = transferred;
    } else {
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

TransferResult SyncTransfer::interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    TransferResult result;
    result.data.resize(length);
    int transferred = 0;
    int rc = libusb_interrupt_transfer(_handle, endpoint, result.data.data(), length, &transferred, timeout_ms);
    if (rc == 0 || rc == LIBUSB_ERROR_TIMEOUT) {
        result.success = true;
        result.bytes_transferred = transferred;
        result.data.resize(transferred > 0 ? transferred : 0);
    } else {
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

TransferResult SyncTransfer::interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    TransferResult result;
    int transferred = 0;
    int rc = libusb_interrupt_transfer(_handle, endpoint,
                                        const_cast<unsigned char*>(data.data()),
                                        static_cast<int>(data.size()), &transferred, timeout_ms);
    if (rc == 0) {
        result.success = true;
        result.bytes_transferred = transferred;
    } else {
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

TransferResult SyncTransfer::control_transfer(uint8_t bmRequestType, uint8_t bRequest,
                                               uint16_t wValue, uint16_t wIndex,
                                               const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    TransferResult result;
    int rc = libusb_control_transfer(_handle, bmRequestType, bRequest,
                                      wValue, wIndex,
                                      const_cast<unsigned char*>(data.data()),
                                      static_cast<uint16_t>(data.size()), timeout_ms);
    if (rc >= 0) {
        result.success = true;
        result.bytes_transferred = rc;
    } else {
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

TransferResult SyncTransfer::control_read(uint8_t bmRequestType, uint8_t bRequest,
                                           uint16_t wValue, uint16_t wIndex,
                                           uint16_t wLength, unsigned int timeout_ms) {
    TransferResult result;
    result.data.resize(wLength);
    int rc = libusb_control_transfer(_handle, bmRequestType, bRequest,
                                      wValue, wIndex, result.data.data(), wLength, timeout_ms);
    if (rc >= 0) {
        result.success = true;
        result.bytes_transferred = rc;
        result.data.resize(rc);
    } else {
        result.error_code = rc;
        result.error_message = libusb_error_name(rc);
    }
    return result;
}

std::string bytes_to_hex(const std::vector<uint8_t>& data, size_t max_bytes) {
    std::ostringstream oss;
    size_t len = std::min(data.size(), max_bytes);
    for (size_t i = 0; i < len; ++i) {
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]) << " ";
        if ((i + 1) % 16 == 0 && i + 1 < len) oss << "\n";
    }
    if (data.size() > max_bytes) oss << "... (" << data.size() << " bytes total)";
    return oss.str();
}

std::string bytes_to_ascii(const std::vector<uint8_t>& data, size_t max_bytes) {
    std::ostringstream oss;
    size_t len = std::min(data.size(), max_bytes);
    for (size_t i = 0; i < len; ++i) {
        char c = static_cast<char>(data[i]);
        oss << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
    }
    return oss.str();
}

std::string format_transfer_result(const TransferResult& r, size_t max_hex) {
    std::ostringstream oss;
    oss << (r.success ? "SUCCESS" : "FAILED");
    if (r.success) {
        oss << " " << r.bytes_transferred << " bytes";
        if (!r.data.empty()) {
            oss << "\n  Hex  : " << bytes_to_hex(r.data, max_hex);
            oss << "\n  ASCII: " << bytes_to_ascii(r.data, max_hex);
        }
    } else {
        oss << " [" << r.error_message << "]";
    }
    return oss.str();
}

} // namespace transfer
} // namespace usb_ctrl
