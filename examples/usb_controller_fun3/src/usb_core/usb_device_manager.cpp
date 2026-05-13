#include "usb_device_manager.hpp"
#include "usb_context.hpp"
#include "libusb.h"
#include <sstream>

namespace usb_ctrl {
namespace core {

UsbDeviceManager::UsbDeviceManager(UsbContext& ctx) : _ctx(ctx) {
    refresh();
}

std::vector<UsbDevice> UsbDeviceManager::_raw_to_list(libusb_device** list, ssize_t count) {
    std::vector<UsbDevice> result;
    result.reserve(static_cast<size_t>(count));
    for (ssize_t i = 0; i < count; ++i)
        result.emplace_back(list[i]);
    return result;
}

void UsbDeviceManager::refresh() {
    _devices.clear();
    libusb_device** raw_list = nullptr;
    ssize_t count = libusb_get_device_list(_ctx.handle(), &raw_list);
    if (count < 0) {
        throw UsbException(
            std::string("libusb_get_device_list: ") + libusb_error_name(static_cast<int>(count)),
            static_cast<int>(count));
    }
    _devices = _raw_to_list(raw_list, count);
    libusb_free_device_list(raw_list, 1);
}

size_t UsbDeviceManager::count() const { return _devices.size(); }

std::string UsbDeviceManager::list_summary() const {
    std::ostringstream oss;
    oss << "Total USB devices: " << _devices.size() << "\n";
    oss << "--------------------------------------\n";
    for (size_t i = 0; i < _devices.size(); ++i)
        oss << "[" << i << "] " << _devices[i].summary() << "\n";
    return oss.str();
}

std::string UsbDeviceManager::list_detail() const {
    std::ostringstream oss;
    oss << "Total USB devices: " << _devices.size() << "\n";
    for (size_t i = 0; i < _devices.size(); ++i) {
        oss << "[" << i << "] " << _devices[i].summary() << "\n";
        oss << _devices[i].detail() << "\n\n";
    }
    return oss.str();
}

std::string UsbDeviceManager::list_tree() const {
    std::ostringstream oss;
    oss << "USB Device Tree (" << _devices.size() << " devices):\n";
    oss << "========================================\n";
    for (size_t i = 0; i < _devices.size(); ++i)
        oss << "[" << i << "] " << _devices[i].descriptor_tree() << "\n";
    return oss.str();
}

std::string UsbDeviceManager::list_hid_summary() const {
    std::ostringstream oss;
    int hid_count = 0;
    for (size_t i = 0; i < _devices.size(); ++i) {
        if (_devices[i].has_hid_interface()) {
            oss << "[" << i << "] " << _devices[i].summary() << "\n";
            ++hid_count;
        }
    }
    if (hid_count == 0)
        oss << "No HID devices found.\n";
    else
        oss << "Total HID devices: " << hid_count << "\n";
    return oss.str();
}

std::vector<UsbDevice> UsbDeviceManager::find_by_vid_pid(uint16_t vid, uint16_t pid) {
    std::vector<UsbDevice> result;
    for (auto& dev : _devices)
        if (dev.vid() == vid && dev.pid() == pid)
            result.emplace_back(dev.handle());
    return result;
}

std::vector<UsbDevice> UsbDeviceManager::find_hid_devices() {
    std::vector<UsbDevice> result;
    for (auto& dev : _devices)
        if (dev.has_hid_interface())
            result.emplace_back(dev.handle());
    return result;
}

std::vector<UsbDevice*> UsbDeviceManager::find_by_class(uint8_t cls) {
    std::vector<UsbDevice*> result;
    for (auto& dev : _devices) {
        auto info = dev.get_info();
        if (info.device_class == cls) result.push_back(&dev);
    }
    return result;
}

} // namespace core
} // namespace usb_ctrl
