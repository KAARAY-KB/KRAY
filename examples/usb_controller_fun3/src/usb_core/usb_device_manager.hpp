#pragma once

#include "usb_device.hpp"
#include <memory>
#include <vector>
#include <string>

struct libusb_device;

namespace usb_ctrl {
namespace core {

class UsbContext;

class UsbDeviceManager {
public:
    explicit UsbDeviceManager(UsbContext& ctx);

    UsbDeviceManager(const UsbDeviceManager&) = delete;
    UsbDeviceManager& operator=(const UsbDeviceManager&) = delete;

    void refresh();
    size_t count() const;

    std::string list_summary() const;
    std::string list_detail() const;
    std::string list_tree() const;
    std::string list_hid_summary() const;

    std::vector<UsbDevice> find_by_vid_pid(uint16_t vid, uint16_t pid);
    std::vector<UsbDevice> find_hid_devices();
    std::vector<UsbDevice*> find_by_class(uint8_t cls);

    std::vector<UsbDevice>& devices() { return _devices; }
    const std::vector<UsbDevice>& devices() const { return _devices; }

private:
    std::vector<UsbDevice> _raw_to_list(libusb_device** list, ssize_t count);

    UsbContext& _ctx;
    std::vector<UsbDevice> _devices;
};

} // namespace core
} // namespace usb_ctrl
