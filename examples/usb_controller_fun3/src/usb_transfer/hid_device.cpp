#include "hid_device.hpp"
#include "libusb.h"
#include <sstream>
#include <iomanip>

namespace usb_ctrl {
namespace transfer {

HidDevice::HidDevice(core::UsbDevice& device) : _device(device) {}

HidDevice::~HidDevice() { close(); }

bool HidDevice::open() {
    if (_handle) return true;
    if (!_device.has_hid_interface()) return false;
    if (!_device.find_hid_interface(_iface_num, _in_ep, _out_ep)) return false;

    int rc = libusb_open(_device.handle(), &_handle);
    if (rc < 0) { _handle = nullptr; return false; }

    if (libusb_kernel_driver_active(_handle, _iface_num) == 1) {
        rc = libusb_detach_kernel_driver(_handle, _iface_num);
        if (rc == 0) _kernel_detached = true;
    }

    rc = libusb_claim_interface(_handle, _iface_num);
    if (rc < 0) { libusb_close(_handle); _handle = nullptr; return false; }

    _transfer = std::make_unique<SyncTransfer>(_handle);
    return true;
}

void HidDevice::close() {
    if (_handle) {
        _transfer.reset();
        libusb_release_interface(_handle, _iface_num);
        if (_kernel_detached) {
            libusb_attach_kernel_driver(_handle, _iface_num);
            _kernel_detached = false;
        }
        libusb_close(_handle);
        _handle = nullptr;
    }
}

TransferResult HidDevice::read_report(int length, unsigned int timeout_ms) {
    if (!_transfer) return {false, 0, 0, "Device not opened", {}};
    return _transfer->interrupt_read(_in_ep.address, length, timeout_ms);
}

TransferResult HidDevice::write_report(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_transfer) return {false, 0, 0, "Device not opened", {}};
    return _transfer->interrupt_write(_out_ep.address, data, timeout_ms);
}

TransferResult HidDevice::send_feature_report(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_transfer) return {false, 0, 0, "Device not opened", {}};
    return _transfer->control_transfer(
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09, // SET_REPORT
        static_cast<uint16_t>(0x0300 | data[0]),
        static_cast<uint16_t>(_iface_num),
        data, timeout_ms);
}

TransferResult HidDevice::get_feature_report(uint8_t report_id, int length, unsigned int timeout_ms) {
    if (!_transfer) return {false, 0, 0, "Device not opened", {}};
    return _transfer->control_read(
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        0x01, // GET_REPORT
        static_cast<uint16_t>(0x0300 | report_id),
        static_cast<uint16_t>(_iface_num),
        static_cast<uint16_t>(length), timeout_ms);
}

std::string HidDevice::device_summary() const {
    std::ostringstream oss;
    oss << "HID Device: Interface " << _iface_num
        << "  IN EP:0x" << std::hex << static_cast<int>(_in_ep.address) << std::dec
        << " (max " << _in_ep.max_packet_size << "B)"
        << "  OUT EP:0x" << std::hex << static_cast<int>(_out_ep.address) << std::dec
        << " (max " << _out_ep.max_packet_size << "B)";
    return oss.str();
}

} // namespace transfer
} // namespace usb_ctrl
