#include "device/UsbDeviceInfo.h"
#include <sstream>
#include <iomanip>

// 解析设备信息
// 参数: dev - 设备指针, handle - 设备句柄
// 返回: DeviceInfo结构体
DeviceInfo UsbDeviceInfo::parse(libusb_device* dev, libusb_device_handle* handle)
{
    DeviceInfo info;                     // 存储解析结果
    libusb_device_descriptor desc;       // 设备描述符

    // 获取设备描述符
    int ret = libusb_get_device_descriptor(dev, &desc);
    if (ret != LIBUSB_SUCCESS) {
        return info;  // 失败返回空信息
    }

    // 填充基本信息
    info.vendor_id = desc.idVendor;
    info.product_id = desc.idProduct;
    info.usb_version = desc.bcdUSB;
    info.device_class = desc.bDeviceClass;
    info.device_subclass = desc.bDeviceSubClass;
    info.device_protocol = desc.bDeviceProtocol;
    info.device_version = desc.bcdDevice;
    info.bus_number = libusb_get_bus_number(dev);
    info.device_address = libusb_get_device_address(dev);

    // 如果设备已打开，获取字符串描述符
    if (handle) {
        // 获取制造商字符串
        if (desc.iManufacturer > 0) {
            unsigned char buf[256];
            ret = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, buf, sizeof(buf));
            if (ret > 0) {
                info.manufacturer = std::string(reinterpret_cast<char*>(buf));
            }
        }

        // 获取产品字符串
        if (desc.iProduct > 0) {
            unsigned char buf[256];
            ret = libusb_get_string_descriptor_ascii(handle, desc.iProduct, buf, sizeof(buf));
            if (ret > 0) {
                info.product = std::string(reinterpret_cast<char*>(buf));
            }
        }

        // 获取序列号字符串
        if (desc.iSerialNumber > 0) {
            unsigned char buf[256];
            ret = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, buf, sizeof(buf));
            if (ret > 0) {
                info.serial_number = std::string(reinterpret_cast<char*>(buf));
            }
        }
    }

    // 解析所有配置描述符
    uint8_t num_configs = desc.bNumConfigurations;
    for (uint8_t i = 0; i < num_configs; ++i) {
        libusb_config_descriptor* config = nullptr;
        ret = libusb_get_config_descriptor(dev, i, &config);
        if (ret != LIBUSB_SUCCESS) {
            continue;  // 跳过失败的配置
        }

        ConfigInfo ci;  // 配置信息
        ci.config_value = config->bConfigurationValue;
        ci.total_length = config->wTotalLength;
        ci.num_interfaces = config->bNumInterfaces;
        ci.attributes = config->bmAttributes;
        ci.max_power = config->MaxPower;

        // 遍历所有接口
        for (uint8_t j = 0; j < config->bNumInterfaces; ++j) {
            const libusb_interface* iface = &config->interface[j];
            // 遍历所有备用设置
            for (int k = 0; k < iface->num_altsetting; ++k) {
                const libusb_interface_descriptor* if_desc = &iface->altsetting[k];

                InterfaceInfo ii;  // 接口信息
                ii.interface_num = if_desc->bInterfaceNumber;
                ii.alt_setting = if_desc->bAlternateSetting;
                ii.interface_class = if_desc->bInterfaceClass;
                ii.interface_subclass = if_desc->bInterfaceSubClass;
                ii.interface_protocol = if_desc->bInterfaceProtocol;

                // 遍历所有端点
                for (uint8_t ep = 0; ep < if_desc->bNumEndpoints; ++ep) {
                    const libusb_endpoint_descriptor* ep_desc = &if_desc->endpoint[ep];

                    EndpointInfo ei;  // 端点信息
                    ei.address = ep_desc->bEndpointAddress;
                    ei.transfer_type = ep_desc->bmAttributes & 0x03;  // 只取低2位
                    ei.max_packet_size = ep_desc->wMaxPacketSize;
                    ei.interval = ep_desc->bInterval;

                    ii.endpoints.push_back(ei);
                }

                ci.interfaces.push_back(ii);
            }
        }

        info.configs.push_back(ci);
        libusb_free_config_descriptor(config);  // 释放配置描述符
    }

    return info;
}

// 打印设备信息到控制台
// 参数: info - 设备信息结构体
void UsbDeviceInfo::print(const DeviceInfo& info)
{
    std::cout << "==================== USB Device Info ====================" << std::endl;
    std::cout << "Vendor ID        : 0x" << std::hex << std::setw(4) << std::setfill('0')
              << info.vendor_id << std::dec << std::endl;
    std::cout << "Product ID       : 0x" << std::hex << std::setw(4) << std::setfill('0')
              << info.product_id << std::dec << std::endl;
    std::cout << "USB Version      : " << std::hex << (info.usb_version >> 12) << "."
              << ((info.usb_version >> 8) & 0x0F) << std::dec << std::endl;
    std::cout << "Device Class     : 0x" << std::hex << static_cast<int>(info.device_class)
              << " (" << getClassName(info.device_class) << ")" << std::dec << std::endl;
    std::cout << "Manufacturer     : " << info.manufacturer << std::endl;
    std::cout << "Product          : " << info.product << std::endl;
    std::cout << "Serial Number    : " << info.serial_number << std::endl;
    std::cout << "Bus Number       : " << static_cast<int>(info.bus_number) << std::endl;
    std::cout << "Device Address   : " << static_cast<int>(info.device_address) << std::endl;
    std::cout << std::endl;

    // 打印所有配置信息
    for (size_t i = 0; i < info.configs.size(); ++i) {
        const ConfigInfo& cfg = info.configs[i];
        std::cout << "--- Configuration " << (i + 1) << " ---" << std::endl;
        std::cout << "  Config Value   : " << static_cast<int>(cfg.config_value) << std::endl;
        std::cout << "  Interfaces     : " << static_cast<int>(cfg.num_interfaces) << std::endl;
        std::cout << "  Max Power      : " << static_cast<int>(cfg.max_power) * 2 << " mA" << std::endl;
        std::cout << std::endl;

        // 打印接口信息
        for (const auto& iface : cfg.interfaces) {
            std::cout << "  Interface " << static_cast<int>(iface.interface_num)
                      << " (Alt " << static_cast<int>(iface.alt_setting) << ")" << std::endl;
            std::cout << "    Class        : 0x" << std::hex << static_cast<int>(iface.interface_class)
                      << " (" << getClassName(iface.interface_class) << ")" << std::dec << std::endl;
            std::cout << "    Endpoints    : " << iface.endpoints.size() << std::endl;

            // 打印端点信息
            for (const auto& ep : iface.endpoints) {
                std::cout << "      Endpoint 0x" << std::hex << static_cast<int>(ep.address)
                          << std::dec << std::endl;
                std::cout << "        Type         : " << getTransferTypeName(ep.transfer_type) << std::endl;
                std::cout << "        Max Packet   : " << ep.max_packet_size << " bytes" << std::endl;
                std::cout << "        Interval     : " << static_cast<int>(ep.interval) << std::endl;
            }
            std::cout << std::endl;
        }
    }
    std::cout << "=========================================================" << std::endl;
}

// 获取速度名称
// 参数: speed - 速度代码
// 返回: 速度名称字符串
std::string UsbDeviceInfo::getSpeedName(uint8_t speed)
{
    switch (speed) {
        case LIBUSB_SPEED_LOW:
            return "Low-Speed";
        case LIBUSB_SPEED_FULL:
            return "Full-Speed";
        case LIBUSB_SPEED_HIGH:
            return "High-Speed";
        case LIBUSB_SPEED_SUPER:
            return "SuperSpeed";
        case LIBUSB_SPEED_SUPER_PLUS:
            return "SuperSpeedPlus";
        default:
            return "Unknown";
    }
}

// 获取类名称
// 参数: class_code - 类代码
// 返回: 类名称字符串
std::string UsbDeviceInfo::getClassName(uint8_t class_code)
{
    switch (class_code) {
        case LIBUSB_CLASS_PER_INTERFACE:
            return "Per Interface";
        case LIBUSB_CLASS_AUDIO:
            return "Audio";
        case LIBUSB_CLASS_COMM:
            return "Communications";
        case LIBUSB_CLASS_HID:
            return "HID";
        case LIBUSB_CLASS_IMAGE:
            return "Image";
        case LIBUSB_CLASS_PRINTER:
            return "Printer";
        case LIBUSB_CLASS_MASS_STORAGE:
            return "Mass Storage";
        case LIBUSB_CLASS_HUB:
            return "Hub";
        case LIBUSB_CLASS_DATA:
            return "CDC-Data";
        case LIBUSB_CLASS_WIRELESS:
            return "Wireless";
        case LIBUSB_CLASS_MISCELLANEOUS:
            return "Miscellaneous";
        case LIBUSB_CLASS_VENDOR_SPEC:
            return "Vendor Specific";
        default:
            return "Unknown";
    }
}

// 获取传输类型名称
// 参数: type - 传输类型代码
// 返回: 传输类型名称字符串
std::string UsbDeviceInfo::getTransferTypeName(uint8_t type)
{
    switch (type) {
        case LIBUSB_TRANSFER_TYPE_CONTROL:
            return "Control";
        case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
            return "Isochronous";
        case LIBUSB_TRANSFER_TYPE_BULK:
            return "Bulk";
        case LIBUSB_TRANSFER_TYPE_INTERRUPT:
            return "Interrupt";
        default:
            return "Unknown";
    }
}
