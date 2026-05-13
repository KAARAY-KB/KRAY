#include "usb_device.hpp"
#include "libusb.h"
#include <cstring>
#include <algorithm>

namespace usb_ctrl {
namespace core {

const char* speed_to_string(int speed) {
    switch (speed) {
        case LIBUSB_SPEED_LOW:        return "Low (1.5Mbps)";
        case LIBUSB_SPEED_FULL:       return "Full (12Mbps)";
        case LIBUSB_SPEED_HIGH:       return "High (480Mbps)";
        case LIBUSB_SPEED_SUPER:      return "Super (5Gbps)";
        case LIBUSB_SPEED_SUPER_PLUS: return "Super+ (10Gbps)";
        default:                      return "Unknown";
    }
}

const char* class_to_string(uint8_t cls) {
    switch (cls) {
        case 0x00: return "Per Interface";
        case 0x01: return "Audio";
        case 0x02: return "CDC";
        case 0x03: return "HID";
        case 0x05: return "Physical";
        case 0x06: return "Image";
        case 0x07: return "Printer";
        case 0x08: return "Mass Storage";
        case 0x09: return "Hub";
        case 0x0A: return "CDC-Data";
        case 0x0B: return "Smart Card";
        case 0x0E: return "Video";
        case 0xDC: return "Diagnostic";
        case 0xE0: return "Wireless";
        case 0xEF: return "Misc";
        case 0xFE: return "App Specific";
        case 0xFF: return "Vendor Specific";
        default:   return "Unknown";
    }
}

const char* ep_type_to_string(uint8_t bmAttributes) {
    switch (bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) {
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_CONTROL:     return "Control";
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_ISOCHRONOUS: return "Isochronous";
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK:        return "Bulk";
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT:   return "Interrupt";
        default:                                        return "Unknown";
    }
}

const char* ep_dir_to_string(uint8_t addr) {
    return (addr & LIBUSB_ENDPOINT_DIR_MASK) ? "IN" : "OUT";
}

std::string EndpointInfo::direction_str() const { return ep_dir_to_string(address); }
std::string EndpointInfo::type_str() const      { return ep_type_to_string(attributes); }
int EndpointInfo::endpoint_id() const            { return address & LIBUSB_ENDPOINT_ADDRESS_MASK; }
bool EndpointInfo::is_in() const                 { return (address & LIBUSB_ENDPOINT_DIR_MASK) != 0; }
bool EndpointInfo::is_out() const                { return (address & LIBUSB_ENDPOINT_DIR_MASK) == 0; }
bool ConfigInfo::self_powered() const            { return (attributes & 0x40) != 0; }
bool ConfigInfo::remote_wakeup() const           { return (attributes & 0x20) != 0; }

UsbDevice::UsbDevice(libusb_device* raw_dev) : _dev(raw_dev) {
    libusb_ref_device(_dev);
}

UsbDevice::~UsbDevice() {
    if (_handle) {
        libusb_close(_handle);
    }
    if (_dev) {
        libusb_unref_device(_dev);
    }
}

UsbDevice::UsbDevice(UsbDevice&& other) noexcept
    : _dev(other._dev), _handle(other._handle), _opened(other._opened) {
    other._dev = nullptr;
    other._handle = nullptr;
    other._opened = false;
}

UsbDevice& UsbDevice::operator=(UsbDevice&& other) noexcept {
    if (this != &other) {
        if (_handle) libusb_close(_handle);
        if (_dev) libusb_unref_device(_dev);
        _dev = other._dev;
        _handle = other._handle;
        _opened = other._opened;
        other._dev = nullptr;
        other._handle = nullptr;
        other._opened = false;
    }
    return *this;
}

void UsbDevice::try_open() const {
    if (_opened) return;
    int rc = libusb_open(_dev, &_handle);
    if (rc == 0) _opened = true;
}

uint16_t UsbDevice::vid() const {
    libusb_device_descriptor desc;
    return libusb_get_device_descriptor(_dev, &desc) == 0 ? desc.idVendor : 0;
}

uint16_t UsbDevice::pid() const {
    libusb_device_descriptor desc;
    return libusb_get_device_descriptor(_dev, &desc) == 0 ? desc.idProduct : 0;
}

std::string UsbDevice::get_string_descriptor(uint8_t index, uint16_t langid) const {
    if (index == 0) return "";
    try_open();
    if (!_handle) return "";
    unsigned char buf[512];
    int len = libusb_get_string_descriptor_ascii(_handle, index, buf, sizeof(buf));
    return len > 0 ? std::string(reinterpret_cast<char*>(buf), len) : "";
}

std::string UsbDevice::_format_hex(const unsigned char* data, size_t len, size_t per_line) const {
    std::ostringstream oss;
    for (size_t i = 0; i < len; i += per_line) {
        for (size_t j = 0; j < per_line && (i + j) < len; ++j) {
            oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i + j]) << " ";
        }
        for (size_t j = len - i; j < per_line && j > 0; ++j) oss << "   ";
        oss << "  ";
        for (size_t j = 0; j < per_line && (i + j) < len; ++j) {
            char c = static_cast<char>(data[i + j]);
            oss << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
        }
        if (i + per_line < len) oss << "\n";
    }
    return oss.str();
}

DeviceInfo UsbDevice::get_info() const {
    DeviceInfo info;
    libusb_device_descriptor desc;
    int rc = libusb_get_device_descriptor(_dev, &desc);
    if (rc < 0) return info;

    info.vendor_id = desc.idVendor;
    info.product_id = desc.idProduct;
    info.bcd_device = desc.bcdDevice;
    info.bcd_usb = desc.bcdUSB;
    info.device_class = desc.bDeviceClass;
    info.device_subclass = desc.bDeviceSubClass;
    info.device_protocol = desc.bDeviceProtocol;
    info.max_packet_size0 = desc.bMaxPacketSize0;
    info.class_str = class_to_string(desc.bDeviceClass);
    info.device_speed = libusb_get_device_speed(_dev);
    info.speed_str = speed_to_string(info.device_speed);
    info.bus_number = libusb_get_bus_number(_dev);
    info.device_address = libusb_get_device_address(_dev);
    info.port_number = libusb_get_port_number(_dev);

    try_open();
    if (_handle) {
        info.manufacturer = get_string_descriptor(desc.iManufacturer);
        info.product = get_string_descriptor(desc.iProduct);
        info.serial_number = get_string_descriptor(desc.iSerialNumber);
    }

    libusb_config_descriptor* cfg = nullptr;
    rc = libusb_get_active_config_descriptor(_dev, &cfg);
    if (rc == 0 && cfg) {
        ConfigInfo ci;
        ci.value = cfg->bConfigurationValue;
        ci.num_interfaces = cfg->bNumInterfaces;
        ci.max_power = cfg->MaxPower;
        ci.attributes = cfg->bmAttributes;
        if (_handle) ci.name = get_string_descriptor(cfg->iConfiguration);

        for (int i = 0; i < cfg->bNumInterfaces; ++i) {
            auto& iface = cfg->interface[i];
            for (int j = 0; j < iface.num_altsetting; ++j) {
                auto& alt = iface.altsetting[j];
                InterfaceInfo ii;
                ii.number = alt.bInterfaceNumber;
                ii.alt_setting = alt.bAlternateSetting;
                ii.bclass = alt.bInterfaceClass;
                ii.subclass = alt.bInterfaceSubClass;
                ii.protocol = alt.bInterfaceProtocol;
                ii.i_interface = alt.iInterface;
                ii.class_str = class_to_string(alt.bInterfaceClass);
                if (_handle) ii.name = get_string_descriptor(alt.iInterface);

                for (int k = 0; k < alt.bNumEndpoints; ++k) {
                    auto& ep = alt.endpoint[k];
                    EndpointInfo ei;
                    ei.address = ep.bEndpointAddress;
                    ei.attributes = ep.bmAttributes;
                    ei.max_packet_size = ep.wMaxPacketSize;
                    ei.interval = ep.bInterval;
                    ii.endpoints.push_back(ei);
                }
                ci.interfaces.push_back(std::move(ii));
            }
        }
        info.configs.push_back(std::move(ci));
        libusb_free_config_descriptor(cfg);
    }
    return info;
}

std::string UsbDevice::_config_details(const DeviceInfo& info) const {
    std::ostringstream oss;
    oss << "\n  ======================== USB Device ========================\n\n";
    oss << "      +++++++++++++++++ Device Information ++++++++++++++++++\n";
    oss << "Device Description       : " << info.product << "\n";
    oss << "Manufacturer             : " << info.manufacturer << "\n";
    oss << "Serial Number            : " << info.serial_number << "\n";
    oss << "Device Speed             : " << info.speed_str << "\n";
    oss << "Bus Number               : " << static_cast<int>(info.bus_number) << "\n";
    oss << "Device Address           : 0x" << std::hex << std::uppercase
        << static_cast<int>(info.device_address) << std::dec << "\n";
    oss << "Port Number              : " << static_cast<int>(info.port_number) << "\n\n";

    oss << "  ---------------------- Device Descriptor ----------------------\n";
    oss << std::hex << std::uppercase << std::setfill('0');
    oss << "bcdUSB                   : 0x" << std::setw(4) << info.bcd_usb
        << "  (USB " << ((info.bcd_usb >> 8) & 0xFF) << "." << (info.bcd_usb & 0xFF) << ")\n";
    oss << "bDeviceClass             : 0x" << std::setw(2) << static_cast<int>(info.device_class)
        << "  (" << info.class_str << ")\n";
    oss << "bDeviceSubClass          : 0x" << std::setw(2) << static_cast<int>(info.device_subclass) << "\n";
    oss << "bDeviceProtocol          : 0x" << std::setw(2) << static_cast<int>(info.device_protocol) << "\n";
    oss << "bMaxPacketSize0          : 0x" << std::setw(2) << static_cast<int>(info.max_packet_size0)
        << "  (" << static_cast<int>(info.max_packet_size0) << " bytes)\n";
    oss << "idVendor                 : 0x" << std::setw(4) << info.vendor_id << "\n";
    oss << "idProduct                : 0x" << std::setw(4) << info.product_id << "\n";
    oss << "bcdDevice                : 0x" << std::setw(4) << info.bcd_device << "\n";
    oss << "iManufacturer            : \"" << info.manufacturer << "\"\n";
    oss << "iProduct                 : \"" << info.product << "\"\n";
    oss << "iSerialNumber            : \"" << info.serial_number << "\"\n";
    oss << "bNumConfigurations       : " << info.configs.size() << "\n" << std::dec;

    for (const auto& cfg : info.configs) {
        oss << "\n  ------------------ Configuration Descriptor -------------------\n";
        oss << "bNumInterfaces           : " << static_cast<int>(cfg.num_interfaces) << "\n";
        oss << "bConfigurationValue      : " << static_cast<int>(cfg.value) << "\n";
        oss << "iConfiguration           : \"" << cfg.name << "\"\n";
        oss << "bmAttributes             : 0x" << std::hex << std::setw(2)
            << static_cast<int>(cfg.attributes) << std::dec << "\n";
        oss << "  Self Powered           : " << (cfg.self_powered() ? "yes" : "no") << "\n";
        oss << "  Remote Wakeup          : " << (cfg.remote_wakeup() ? "yes" : "no") << "\n";
        oss << "MaxPower                 : " << static_cast<int>(cfg.max_power) * 2 << " mA\n";

        for (const auto& iface : cfg.interfaces) {
            oss << "\n  ---------------- Interface Descriptor -----------------\n";
            oss << "bInterfaceNumber         : " << static_cast<int>(iface.number) << "\n";
            oss << "bAlternateSetting        : " << static_cast<int>(iface.alt_setting) << "\n";
            oss << "bNumEndpoints            : " << iface.endpoints.size() << "\n";
            oss << "bInterfaceClass          : 0x" << std::hex << std::setw(2)
                << static_cast<int>(iface.bclass) << std::dec << "  (" << iface.class_str << ")\n";
            oss << "bInterfaceSubClass       : 0x" << std::hex << std::setw(2)
                << static_cast<int>(iface.subclass) << std::dec << "\n";
            oss << "bInterfaceProtocol       : 0x" << std::hex << std::setw(2)
                << static_cast<int>(iface.protocol) << std::dec << "\n";
            if (!iface.name.empty())
                oss << "iInterface               : \"" << iface.name << "\"\n";

            for (const auto& ep : iface.endpoints) {
                oss << "\n  ----------------- Endpoint Descriptor -----------------\n";
                oss << "bEndpointAddress         : 0x" << std::hex << std::setw(2)
                    << static_cast<int>(ep.address) << std::dec
                    << "  (Direction=" << ep.direction_str()
                    << ", EndpointID=" << ep.endpoint_id() << ")\n";
                oss << "bmAttributes             : 0x" << std::hex << std::setw(2)
                    << static_cast<int>(ep.attributes) << std::dec
                    << "  (TransferType=" << ep.type_str() << ")\n";
                oss << "wMaxPacketSize           : " << std::dec << ep.max_packet_size << " bytes\n";
                oss << "bInterval                : " << static_cast<int>(ep.interval) << "\n";
            }
        }
    }
    return oss.str();
}

std::string UsbDevice::summary() const {
    auto info = get_info();
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    oss << "VID:0x" << std::setw(4) << info.vendor_id
        << " PID:0x" << std::setw(4) << info.product_id
        << " Speed:" << info.speed_str
        << " USB:" << ((info.bcd_usb >> 8) & 0xFF) << "." << (info.bcd_usb & 0xFF);
    if (!info.product.empty()) oss << " [" << info.product << "]";
    if (!info.manufacturer.empty()) oss << " (" << info.manufacturer << ")";
    if (!info.configs.empty())
        oss << " Ifaces:" << static_cast<int>(info.configs[0].num_interfaces);
    return oss.str();
}

std::string UsbDevice::detail() const {
    auto info = get_info();
    std::ostringstream oss;
    oss << "  ========================== Summary =========================\n";
    oss << std::hex << std::uppercase << std::setfill('0');
    oss << "Vendor ID                : 0x" << std::setw(4) << info.vendor_id
        << (info.manufacturer.empty() ? "" : " (" + info.manufacturer + ")") << "\n";
    oss << "Product ID               : 0x" << std::setw(4) << info.product_id
        << (info.product.empty() ? "" : " (" + info.product + ")") << "\n";
    oss << "USB Version              : " << ((info.bcd_usb >> 8) & 0xFF)
        << "." << (info.bcd_usb & 0xFF) << "\n";
    oss << "Device Speed             : " << info.speed_str << "\n";
    if (!info.configs.empty()) {
        oss << "Self powered             : " << (info.configs[0].self_powered() ? "yes" : "no") << "\n";
        oss << "MaxPower                 : " << static_cast<int>(info.configs[0].max_power) * 2 << " mA\n";
        int ep_count = 0;
        for (auto& iface : info.configs[0].interfaces) ep_count += static_cast<int>(iface.endpoints.size());
        oss << "Used Endpoints           : " << ep_count << "\n";
    }
    oss << std::dec;
    oss << _config_details(info);
    return oss.str();
}

std::string UsbDevice::descriptor_tree() const {
    auto info = get_info();
    std::ostringstream oss;
    oss << "Device: " << info.product << " (";
    oss << std::hex << std::uppercase << "0x" << info.vendor_id << ":0x" << info.product_id << std::dec << ")\n";
    for (const auto& cfg : info.configs) {
        oss << "  Config " << static_cast<int>(cfg.value) << "\n";
        for (const auto& iface : cfg.interfaces) {
            oss << "    Interface " << static_cast<int>(iface.number)
                << " [" << iface.class_str << "]";
            if (!iface.name.empty()) oss << " \"" << iface.name << "\"";
            oss << "\n";
            for (const auto& ep : iface.endpoints) {
                oss << "      EP 0x" << std::hex << static_cast<int>(ep.address) << std::dec
                    << " " << ep.type_str() << " " << ep.direction_str()
                    << " (" << ep.max_packet_size << "B)\n";
            }
        }
    }
    return oss.str();
}

bool UsbDevice::has_hid_interface() const {
    auto info = get_info();
    for (const auto& cfg : info.configs)
        for (const auto& iface : cfg.interfaces)
            if (iface.bclass == LIBUSB_CLASS_HID) return true;
    return false;
}

bool UsbDevice::find_hid_interface(int& iface_num, EndpointInfo& in_ep, EndpointInfo& out_ep) const {
    auto info = get_info();
    for (const auto& cfg : info.configs) {
        for (const auto& iface : cfg.interfaces) {
            if (iface.bclass == LIBUSB_CLASS_HID) {
                iface_num = static_cast<int>(iface.number);
                for (const auto& ep : iface.endpoints) {
                    if (ep.is_in() && ep.type_str() == std::string("Interrupt"))
                        in_ep = ep;
                    if (ep.is_out() && ep.type_str() == std::string("Interrupt"))
                        out_ep = ep;
                }
                return true;
            }
        }
    }
    return false;
}

} // namespace core
} // namespace usb_ctrl
