#include "usb_device.hpp"
#include "usb_context.hpp"
#include "libusb.h"

#include <iomanip>
#include <cstring>
#include <algorithm>

namespace usb_core {

const char* usb_speed_to_string(int speed) {
    switch (speed) {
        case LIBUSB_SPEED_LOW:        return "Low Speed (1.5 Mbps)";
        case LIBUSB_SPEED_FULL:       return "Full Speed (12 Mbps)";
        case LIBUSB_SPEED_HIGH:       return "High Speed (480 Mbps)";
        case LIBUSB_SPEED_SUPER:      return "SuperSpeed (5 Gbps)";
        case LIBUSB_SPEED_SUPER_PLUS: return "SuperSpeed+ (10 Gbps)";
        case LIBUSB_SPEED_SUPER_PLUS_X2: return "SuperSpeed+ x2 (20 Gbps)";
        default:                       return "Unknown";
    }
}

// 将 USB 设备类代码转换为可读字符串
const char* usb_class_to_string(uint8_t cls) {
    switch (cls) {
        case 0x00: return "Per Interface";
        case 0x01: return "Audio";
        case 0x02: return "Communications (CDC)";
        case 0x03: return "HID (Human Interface Device)";
        case 0x05: return "Physical";
        case 0x06: return "Image / PTP";
        case 0x07: return "Printer";
        case 0x08: return "Mass Storage";
        case 0x09: return "Hub";
        case 0x0A: return "CDC-Data";
        case 0x0B: return "Smart Card";
        case 0x0D: return "Content Security";
        case 0x0E: return "Video";
        case 0x0F: return "Personal Healthcare";
        case 0xDC: return "Diagnostic Device";
        case 0xE0: return "Wireless";
        case 0xEF: return "Miscellaneous";
        case 0xFE: return "Application Specific";
        case 0xFF: return "Vendor Specific";
        default:   return "Unknown";
    }
}

// 将端点属性中的传输类型位转换为可读字符串
const char* endpoint_type_to_string(uint8_t bmAttributes) {
    switch (bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) {
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_CONTROL:     return "Control";
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_ISOCHRONOUS: return "Isochronous";
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK:        return "Bulk";
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT:   return "Interrupt";
        default:                                         return "Unknown";
    }
}

// 将端点地址中的方向位转换为可读字符串
const char* endpoint_direction_to_string(uint8_t addr) {
    return (addr & LIBUSB_ENDPOINT_DIR_MASK) ? "IN" : "OUT";
}

// 获取端点方向描述
std::string UsbEndpointInfo::direction_str() const {
    return endpoint_direction_to_string(address);
}

// 获取端点传输类型描述
std::string UsbEndpointInfo::type_str() const {
    return endpoint_type_to_string(attributes);
}

// 获取端点编号 (地址的低 4 位)
int UsbEndpointInfo::endpoint_id() const {
    return address & LIBUSB_ENDPOINT_ADDRESS_MASK;
}

// 判断是否为 IN 端点 (设备到主机方向)
bool UsbEndpointInfo::is_in() const {
    return (address & LIBUSB_ENDPOINT_DIR_MASK) != 0;
}

// 判断是否为 OUT 端点 (主机到设备方向)
bool UsbEndpointInfo::is_out() const {
    return (address & LIBUSB_ENDPOINT_DIR_MASK) == 0;
}

// 判断配置是否为自供电 (bit 6)
bool UsbConfigInfo::self_powered() const {
    return (attributes & 0x40) != 0;
}

// 判断配置是否支持远程唤醒 (bit 5)
bool UsbConfigInfo::remote_wakeup() const {
    return (attributes & 0x20) != 0;
}

// UsbDevice 构造函数：接收原始设备指针并增加引用计数
UsbDevice::UsbDevice(libusb_device* raw_dev)
    : _dev(raw_dev) {
    libusb_ref_device(_dev);
}

// UsbDevice 析构函数：关闭设备句柄并释放设备引用
UsbDevice::~UsbDevice() {
    if (_handle) {
        libusb_close(_handle);
        _handle = nullptr;
    }
    if (_dev) {
        libusb_unref_device(_dev);
        _dev = nullptr;
    }
}

// 移动构造函数：转移设备指针所有权
UsbDevice::UsbDevice(UsbDevice&& other) noexcept
    : _dev(other._dev), _handle(other._handle), _opened(other._opened) {
    other._dev = nullptr;
    other._handle = nullptr;
    other._opened = false;
}

// 移动赋值运算符：释放当前资源并接管对方资源
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

// 延迟打开设备：仅在需要读取字符串描述符时才打开设备
// 使用 mutable 成员变量，允许在 const 方法中修改
void UsbDevice::try_open() const {
    if (_opened) return;
    int rc = libusb_open(_dev, &_handle);
    if (rc == 0) {
        _opened = true;
    }
}

// 获取厂商 ID (VID)
uint16_t UsbDevice::vid() const {
    libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(_dev, &desc) == 0) {
        return desc.idVendor;
    }
    return 0;
}

// 获取产品 ID (PID)
uint16_t UsbDevice::pid() const {
    libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(_dev, &desc) == 0) {
        return desc.idProduct;
    }
    return 0;
}

// 格式化字节数组为十六进制字符串，每行显示指定数量的字节
// 同时显示 ASCII 可打印字符，不可打印字符用 '.' 代替
std::string UsbDevice::_format_hex(const unsigned char* data, size_t len, size_t bytes_per_line) const {
    std::ostringstream oss;
    for (size_t i = 0; i < len; i += bytes_per_line) {
        // 输出十六进制值
        for (size_t j = 0; j < bytes_per_line && (i + j) < len; ++j) {
            oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i + j]) << " ";
        }
        // 填充空格对齐
        for (size_t j = i + bytes_per_line; j < i + bytes_per_line && j < len + (bytes_per_line - (len % bytes_per_line)); ++j) {
            oss << "   ";
        }
        oss << "  ";
        // 输出 ASCII 字符
        for (size_t j = 0; j < bytes_per_line && (i + j) < len; ++j) {
            char c = static_cast<char>(data[i + j]);
            oss << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
        }
        if (i + bytes_per_line < len) oss << "\n";
    }
    return oss.str();
}

// 获取描述符的十六进制转储字符串
std::string UsbDevice::_get_descriptor_hexdump(const unsigned char* data, int length) const {
    return _format_hex(data, length, 16);
}

// 获取字符串描述符内容
// index: 字符串描述符索引 (0 表示无效)
// langid: 语言 ID，默认 0x0409 (美式英语)
std::string UsbDevice::get_string_descriptor(uint8_t index, uint16_t langid) const {
    if (index == 0) return "";
    try_open();
    if (!_handle) return "";
    unsigned char buf[512];
    int len = libusb_get_string_descriptor_ascii(_handle, index, buf, sizeof(buf));
    if (len > 0) {
        return std::string(reinterpret_cast<char*>(buf), len);
    }
    return "";
}

// 获取完整的设备信息
// 包括设备描述符、配置描述符、接口描述符、端点描述符和字符串描述符
UsbDeviceInfo UsbDevice::get_info() const {
    UsbDeviceInfo info;
    libusb_device_descriptor desc;
    int rc = libusb_get_device_descriptor(_dev, &desc);
    if (rc < 0) return info;

    // 填充设备描述符字段
    info.vendor_id = desc.idVendor;
    info.product_id = desc.idProduct;
    info.bcd_device = desc.bcdDevice;
    info.bcd_usb = desc.bcdUSB;
    info.device_class = desc.bDeviceClass;
    info.device_subclass = desc.bDeviceSubClass;
    info.device_protocol = desc.bDeviceProtocol;
    info.max_packet_size0 = desc.bMaxPacketSize0;
    info.class_str = usb_class_to_string(desc.bDeviceClass);

    // 获取设备速度和总线信息
    info.device_speed = libusb_get_device_speed(_dev);
    info.speed_str = usb_speed_to_string(info.device_speed);
    info.bus_number = libusb_get_bus_number(_dev);
    info.device_address = libusb_get_device_address(_dev);
    info.port_number = libusb_get_port_number(_dev);

    // 尝试打开设备以读取字符串描述符
    try_open();
    if (_handle) {
        info.manufacturer = get_string_descriptor(desc.iManufacturer);
        info.product = get_string_descriptor(desc.iProduct);
        info.serial_number = get_string_descriptor(desc.iSerialNumber);
    }

    // 获取活动配置描述符
    libusb_config_descriptor* cfg = nullptr;
    rc = libusb_get_active_config_descriptor(_dev, &cfg);
    if (rc == 0 && cfg) {
        UsbConfigInfo ci;
        ci.value = cfg->bConfigurationValue;
        ci.num_interfaces = cfg->bNumInterfaces;
        ci.max_power = cfg->MaxPower;
        ci.attributes = cfg->bmAttributes;
        if (_handle) {
            ci.name = get_string_descriptor(cfg->iConfiguration);
        }

        // 遍历所有接口
        for (int i = 0; i < cfg->bNumInterfaces; ++i) {
            auto& iface = cfg->interface[i];
            for (int j = 0; j < iface.num_altsetting; ++j) {
                auto& alt = iface.altsetting[j];
                UsbInterfaceInfo ii;
                ii.number = alt.bInterfaceNumber;
                ii.alt_setting = alt.bAlternateSetting;
                ii.bclass = alt.bInterfaceClass;
                ii.subclass = alt.bInterfaceSubClass;
                ii.protocol = alt.bInterfaceProtocol;
                ii.i_interface = alt.iInterface;
                ii.class_str = usb_class_to_string(alt.bInterfaceClass);
                if (_handle) {
                    ii.name = get_string_descriptor(alt.iInterface);
                }

                // 遍历接口下的所有端点
                for (int k = 0; k < alt.bNumEndpoints; ++k) {
                    auto& ep = alt.endpoint[k];
                    UsbEndpointInfo ei;
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

// 获取设备摘要信息 (一行文本)
// 格式: VID:0xXXXX PID:0xXXXX Speed:xxx USB:x.xx [Product] (Manufacturer)
std::string UsbDevice::summary() const {
    auto info = get_info();
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    oss << "VID:0x" << std::setw(4) << info.vendor_id
        << " PID:0x" << std::setw(4) << info.product_id
        << " Speed:" << info.speed_str
        << " USB:" << ((info.bcd_usb >> 8) & 0xFF) << "." << (info.bcd_usb & 0xFF);
    if (!info.product.empty()) {
        oss << " [" << info.product << "]";
    }
    if (!info.manufacturer.empty()) {
        oss << " (" << info.manufacturer << ")";
    }
    if (!info.configs.empty()) {
        oss << " Configs:" << info.configs.size();
        oss << " Interfaces:" << static_cast<int>(info.configs[0].num_interfaces);
    }
    return oss.str();
}

// 获取配置详情字符串 (包含所有描述符的详细信息)
std::string UsbDevice::_get_config_details(const UsbDeviceInfo& info) const {
    std::ostringstream oss;
    oss << "\n";
    oss << "  ======================== USB Device ========================\n\n";

    // 设备基本信息
    oss << "      +++++++++++++++++ Device Information ++++++++++++++++++\n";
    oss << "Device Description       : " << info.product << "\n";
    oss << "Manufacturer             : " << info.manufacturer << "\n";
    oss << "Serial Number            : " << info.serial_number << "\n";
    oss << "Device Speed             : " << info.speed_str << "\n";
    oss << "Bus Number               : " << static_cast<int>(info.bus_number) << "\n";
    oss << "Device Address           : 0x" << std::hex << std::uppercase
        << static_cast<int>(info.device_address) << std::dec << "\n";
    oss << "Port Number              : " << static_cast<int>(info.port_number) << "\n\n";

    // 设备描述符详情
    oss << "  ---------------------- Device Descriptor ----------------------\n";
    oss << "bLength                  : 0x12 (18 bytes)\n";
    oss << "bDescriptorType          : 0x01 (Device Descriptor)\n";
    oss << std::hex << std::uppercase << std::setfill('0');
    oss << "bcdUSB                   : 0x" << std::setw(4) << info.bcd_usb
        << " (USB Version " << ((info.bcd_usb >> 8) & 0xFF) << "." << (info.bcd_usb & 0xFF) << ")\n";
    oss << "bDeviceClass             : 0x" << std::setw(2) << static_cast<int>(info.device_class)
        << " (" << info.class_str << ")\n";
    oss << "bDeviceSubClass          : 0x" << std::setw(2) << static_cast<int>(info.device_subclass) << "\n";
    oss << "bDeviceProtocol          : 0x" << std::setw(2) << static_cast<int>(info.device_protocol) << "\n";
    oss << "bMaxPacketSize0          : 0x" << std::setw(2) << static_cast<int>(info.max_packet_size0)
        << " (" << static_cast<int>(info.max_packet_size0) << " bytes)\n";
    oss << "idVendor                 : 0x" << std::setw(4) << info.vendor_id << "\n";
    oss << "idProduct                : 0x" << std::setw(4) << info.product_id << "\n";
    oss << "bcdDevice                : 0x" << std::setw(4) << info.bcd_device << "\n";
    oss << "iManufacturer            : 0x01 (\"" << info.manufacturer << "\")\n";
    oss << "iProduct                 : 0x02 (\"" << info.product << "\")\n";
    oss << "iSerialNumber            : 0x03 (\"" << info.serial_number << "\")\n";
    oss << "bNumConfigurations       : " << info.configs.size() << "\n";
    oss << std::dec;

    // 遍历所有配置
    for (const auto& cfg : info.configs) {
        oss << "\n  ------------------ Configuration Descriptor -------------------\n";
        oss << "bLength                  : 0x09 (9 bytes)\n";
        oss << "bDescriptorType          : 0x02 (Configuration Descriptor)\n";
        oss << "bNumInterfaces           : " << static_cast<int>(cfg.num_interfaces) << "\n";
        oss << "bConfigurationValue      : " << static_cast<int>(cfg.value) << "\n";
        oss << "iConfiguration           : \"" << cfg.name << "\"\n";
        oss << "bmAttributes             : 0x" << std::hex << std::setw(2)
            << static_cast<int>(cfg.attributes) << std::dec << "\n";
        oss << "  Self Powered           : " << (cfg.self_powered() ? "yes" : "no") << "\n";
        oss << "  Remote Wakeup          : " << (cfg.remote_wakeup() ? "yes" : "no") << "\n";
        oss << "MaxPower                 : " << static_cast<int>(cfg.max_power) * 2 << " mA\n";

        // 遍历配置下的所有接口
        for (const auto& iface : cfg.interfaces) {
            oss << "\n  ---------------- Interface Descriptor -----------------\n";
            oss << "bInterfaceNumber         : " << static_cast<int>(iface.number) << "\n";
            oss << "bAlternateSetting        : " << static_cast<int>(iface.alt_setting) << "\n";
            oss << "bNumEndpoints            : " << iface.endpoints.size() << "\n";
            oss << "bInterfaceClass          : 0x" << std::hex << std::setw(2)
                << static_cast<int>(iface.bclass) << std::dec
                << " (" << iface.class_str << ")\n";
            oss << "bInterfaceSubClass       : 0x" << std::hex << std::setw(2)
                << static_cast<int>(iface.subclass) << std::dec << "\n";
            oss << "bInterfaceProtocol       : 0x" << std::hex << std::setw(2)
                << static_cast<int>(iface.protocol) << std::dec << "\n";
            if (!iface.name.empty()) {
                oss << "iInterface               : \"" << iface.name << "\"\n";
            }

            // 遍历接口下的所有端点
            for (const auto& ep : iface.endpoints) {
                oss << "\n  ----------------- Endpoint Descriptor -----------------\n";
                oss << "bEndpointAddress         : 0x" << std::hex << std::setw(2)
                    << static_cast<int>(ep.address) << std::dec
                    << " (Direction=" << ep.direction_str()
                    << " EndpointID=" << ep.endpoint_id() << ")\n";
                oss << "bmAttributes             : 0x" << std::hex << std::setw(2)
                    << static_cast<int>(ep.attributes) << std::dec
                    << " (TransferType=" << ep.type_str() << ")\n";
                oss << "wMaxPacketSize           : 0x" << std::hex << std::setw(4)
                    << ep.max_packet_size << std::dec
                    << " (" << std::dec << ep.max_packet_size << " bytes)\n";
                oss << "bInterval                : 0x" << std::hex << std::setw(2)
                    << static_cast<int>(ep.interval) << std::dec << "\n";
            }
        }
    }

    return oss.str();
}

// 判断设备是否包含 HID 接口
bool UsbDevice::has_hid_interface() const {
    auto info = get_info();
    for (const auto& cfg : info.configs) {
        for (const auto& iface : cfg.interfaces) {
            if (iface.bclass == LIBUSB_CLASS_HID) {
                return true;
            }
        }
    }
    return false;
}

// 查找 HID 接口
// 返回接口编号和 IN/OUT 中断端点信息
// 如果找到 HID 接口返回 true，否则返回 false
bool UsbDevice::find_hid_interface(int& interface_number,
                                    UsbEndpointInfo& in_endpoint,
                                    UsbEndpointInfo& out_endpoint) const {
    auto info = get_info();
    for (const auto& cfg : info.configs) {
        for (const auto& iface : cfg.interfaces) {
            if (iface.bclass == LIBUSB_CLASS_HID) {
                interface_number = static_cast<int>(iface.number);
                // 查找 IN 和 OUT 中断端点
                for (const auto& ep : iface.endpoints) {
                    if (ep.is_in() && ep.type_str() == std::string("Interrupt")) {
                        in_endpoint = ep;
                    }
                    if (ep.is_out() && ep.type_str() == std::string("Interrupt")) {
                        out_endpoint = ep;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

// 获取设备详细信息 (包含摘要和所有描述符详情)
std::string UsbDevice::detail() const {
    auto info = get_info();
    std::ostringstream oss;

    // 摘要部分
    oss << "  ========================== Summary =========================\n";
    oss << std::hex << std::uppercase << std::setfill('0');
    oss << "Vendor ID                : 0x" << std::setw(4) << info.vendor_id;
    if (!info.manufacturer.empty()) oss << " (" << info.manufacturer << ")";
    oss << "\n";
    oss << "Product ID               : 0x" << std::setw(4) << info.product_id;
    if (!info.product.empty()) oss << " (" << info.product << ")";
    oss << "\n";
    oss << "USB Version              : " << ((info.bcd_usb >> 8) & 0xFF)
        << "." << (info.bcd_usb & 0xFF) << "\n";
    oss << "Device Speed             : " << info.speed_str << "\n";
    if (!info.configs.empty()) {
        oss << "Self powered             : " << (info.configs[0].self_powered() ? "yes" : "no") << "\n";
        oss << "MaxPower                 : " << static_cast<int>(info.configs[0].max_power) * 2 << " mA\n";
        int ep_count = 0;
        for (const auto& iface : info.configs[0].interfaces) {
            ep_count += static_cast<int>(iface.endpoints.size());
        }
        oss << "Used Endpoints           : " << ep_count << "\n";
    }
    oss << std::dec;

    // 配置详情部分
    oss << _get_config_details(info);

    return oss.str();
}

} // namespace usb_core
