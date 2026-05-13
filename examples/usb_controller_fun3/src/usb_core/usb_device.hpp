#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

struct libusb_device;
struct libusb_device_handle;
struct libusb_device_descriptor;
struct libusb_config_descriptor;

namespace usb_ctrl {
namespace core {

const char* speed_to_string(int speed);
const char* class_to_string(uint8_t cls);
const char* ep_type_to_string(uint8_t bmAttributes);
const char* ep_dir_to_string(uint8_t addr);

struct EndpointInfo {
    uint8_t address = 0;
    uint8_t attributes = 0;
    uint16_t max_packet_size = 0;
    uint8_t interval = 0;

    std::string direction_str() const;
    std::string type_str() const;
    int endpoint_id() const;
    bool is_in() const;
    bool is_out() const;
};

struct InterfaceInfo {
    uint8_t number = 0;
    uint8_t alt_setting = 0;
    uint8_t bclass = 0;
    uint8_t subclass = 0;
    uint8_t protocol = 0;
    uint8_t i_interface = 0;
    std::string name;
    std::string class_str;
    std::vector<EndpointInfo> endpoints;
};

struct ConfigInfo {
    uint8_t value = 0;
    uint8_t num_interfaces = 0;
    uint8_t max_power = 0;
    uint8_t attributes = 0;
    std::string name;
    bool self_powered() const;
    bool remote_wakeup() const;
    std::vector<InterfaceInfo> interfaces;
};

struct DeviceInfo {
    uint16_t vendor_id = 0;
    uint16_t product_id = 0;
    uint16_t bcd_device = 0;
    uint16_t bcd_usb = 0;
    uint8_t device_class = 0;
    uint8_t device_subclass = 0;
    uint8_t device_protocol = 0;
    uint8_t max_packet_size0 = 0;
    int device_speed = 0;
    uint8_t bus_number = 0;
    uint8_t device_address = 0;
    uint8_t port_number = 0;
    std::string manufacturer;
    std::string product;
    std::string serial_number;
    std::string class_str;
    std::string speed_str;
    std::vector<ConfigInfo> configs;
};

class UsbDevice {
public:
    explicit UsbDevice(libusb_device* raw_dev);
    ~UsbDevice();

    UsbDevice(const UsbDevice&) = delete;
    UsbDevice& operator=(const UsbDevice&) = delete;
    UsbDevice(UsbDevice&& other) noexcept;
    UsbDevice& operator=(UsbDevice&& other) noexcept;

    libusb_device* handle() const { return _dev; }

    DeviceInfo get_info() const;
    std::string get_string_descriptor(uint8_t index, uint16_t langid = 0x0409) const;

    bool has_hid_interface() const;
    bool find_hid_interface(int& iface_num, EndpointInfo& in_ep, EndpointInfo& out_ep) const;

    std::string summary() const;
    std::string detail() const;
    std::string descriptor_tree() const;

    uint16_t vid() const;
    uint16_t pid() const;

private:
    void try_open() const;

    libusb_device* _dev = nullptr;
    mutable libusb_device_handle* _handle = nullptr;
    mutable bool _opened = false;

    std::string _format_hex(const unsigned char* data, size_t len, size_t per_line = 16) const;
    std::string _config_details(const DeviceInfo& info) const;
};

} // namespace core
} // namespace usb_ctrl
