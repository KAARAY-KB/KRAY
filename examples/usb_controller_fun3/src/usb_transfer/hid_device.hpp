#pragma once

#include "usb_transfer.hpp"
#include "usb_core/usb_device.hpp"
#include <memory>
#include <string>

struct libusb_device_handle;

namespace usb_ctrl {
namespace transfer {

class HidDevice {
public:
    explicit HidDevice(core::UsbDevice& device);
    ~HidDevice();

    HidDevice(const HidDevice&) = delete;
    HidDevice& operator=(const HidDevice&) = delete;

    bool open();
    void close();
    bool is_open() const { return _handle != nullptr; }

    int interface_number() const { return _iface_num; }
    core::EndpointInfo in_endpoint() const { return _in_ep; }
    core::EndpointInfo out_endpoint() const { return _out_ep; }

    TransferResult read_report(int length, unsigned int timeout_ms = 1000);
    TransferResult write_report(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);
    TransferResult get_feature_report(uint8_t report_id, int length, unsigned int timeout_ms = 1000);
    TransferResult send_feature_report(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    std::string device_summary() const;
    SyncTransfer& transfer() { return *_transfer; }
    libusb_device_handle* handle() const { return _handle; }

private:
    core::UsbDevice& _device;
    libusb_device_handle* _handle = nullptr;
    std::unique_ptr<SyncTransfer> _transfer;
    int _iface_num = -1;
    core::EndpointInfo _in_ep;
    core::EndpointInfo _out_ep;
    bool _kernel_detached = false;
};

} // namespace transfer
} // namespace usb_ctrl
