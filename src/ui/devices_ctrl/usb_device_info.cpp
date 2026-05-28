// ============================================================================
// usb_device_info.cpp - USB 设备信息结构体（实现文件）
//
// 功能说明：
//   实现 UsbDeviceInfo 的方法：
//   - from_usb_device：从 UsbDevice 提取信息
//   - detect_type：根据 VID/PID 判断设备类型
//   - to_string：格式化输出
//   - operator==：设备比较
// ============================================================================

#include "usb_device_info.hpp"
#include "console.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

static const char *_dn = {"UsbDeviceInfoInfo"};

// 已知设备的 VID/PID 表
struct DevIdEntry {
    const char* name;                    // 设备名称
    UsbDeviceInfo::DevType type;         // 设备类型
    uint16_t vid;                        // 厂商 ID
    uint16_t pid;                        // 产品 ID
};

// 已知设备列表
static const DevIdEntry g_known_devs[] = {
    { "DEBUG",           UsbDeviceInfo::DEV_DBG,             0x5741, 0x4E47 },
    { "GT-64HE",         UsbDeviceInfo::DEV_KB_GT64HE,       0x9013, 0x2601 },
    { "MADE68PRO",       UsbDeviceInfo::DEV_KB_MADE68PRO,    0x0416, 0x0110 },
    { "Wooting-60HE",    UsbDeviceInfo::DEV_KB_WOOTING_60HE, 0x31E3, 0x1220 },
    { "Wooting-TwoHe",   UsbDeviceInfo::DEV_KB_WOOTING_TWOHE,0x31E3, 0x1232 },
    { "G102",            UsbDeviceInfo::DEV_MS_G102,         0x046d, 0xc092 },
};
static constexpr size_t g_known_devs_count = sizeof(g_known_devs) / sizeof(g_known_devs[0]);

// 从 UsbDevice 创建 UsbDeviceInfo
UsbDeviceInfo UsbDeviceInfo::from_usb_device(const usb_ctrl::core::UsbDevice& dev) {
    Console::info(_dn) << " from_usb_device: 开始提取设备信息" << std::endl;
    UsbDeviceInfo info;

    if (dev.is_offline()) {
        info.di = dev.get_info();
        info.type = detect_type(info.di.vendor_id, info.di.product_id);
        info.is_hid = false;
        for (const auto& cfg : info.di.configs) {
            for (const auto& iface : cfg.interfaces) {
                if (iface.bclass == 0x03) {
                    info.is_hid = true;
                    break;
                }
            }
            if (info.is_hid) break;
        }
        Console::info(_dn) << " from_usb_device: 离线设备 is_hid=" << info.is_hid << std::endl;
        Console::info(_dn) << " from_usb_device: 提取 -> " << info.to_string() << std::endl;
        return info;
    }

    info.di = dev.get_info();
    info.type = detect_type(info.di.vendor_id, info.di.product_id);

    info.is_hid = false;
    for (const auto& cfg : info.di.configs) {
        for (const auto& iface : cfg.interfaces) {
            if (iface.bclass == 0x03) {
                info.is_hid = true;
                break;
            }
        }
        if (info.is_hid) break;
    }
    Console::info(_dn) << " from_usb_device: is_hid=" << info.is_hid << std::endl;

    if (info.is_hid) {
        int hid_count = 0;
        for (const auto& cfg : info.di.configs) {
            for (const auto& iface : cfg.interfaces) {
                if (iface.bclass == 0x03) {
                    hid_count++;
                    Console::info(_dn) << " from_usb_device: HID iface="
                                   << (int)iface.number;
                    for (const auto& ep : iface.endpoints) {
                        Console::info(_dn) << " EP=0x" << std::hex << (int)ep.address
                                       << "(" << ep.max_packet_size << "B)"
                                       << ep.direction_str();
                    }
                    Console::info(_dn) << std::dec << std::endl;
                }
            }
        }
        Console::info(_dn) << " from_usb_device: total HID interfaces=" << hid_count << std::endl;
    }

    Console::info(_dn) << " from_usb_device: 提取 -> " << info.to_string() << std::endl;
    return info;
}

// 根据 VID/PID 检测设备类型
UsbDeviceInfo::DevType UsbDeviceInfo::detect_type(uint16_t vid, uint16_t pid) {
    Console::info(_dn) << " 检测类型: checking " << std::hex 
                    << std::setfill('0') << std::setw(4)
                    << "VID:0x" << vid 
                    << " PID:0x" << pid 
                    << std::dec << std::endl;
    DevIdEntry entry = { "DEV_UNKNOWN", UsbDeviceInfo::DEV_UNKNOWN, 0x5741, 0x4E47 };
    for (size_t i = 0; i < g_known_devs_count; ++i) {
        if (g_known_devs[i].vid == vid && g_known_devs[i].pid == pid) {
            entry = g_known_devs[i];
            break;
        }
    }
    Console::info(_dn) << " 检测类型: matched known device -> " << entry.name << std::endl;
    return entry.type;
}

// 格式化为可读字符串
std::string UsbDeviceInfo::to_string() const {
    std::ostringstream oss;
    oss << type_name() << " ["          // 设备类型名
        << std::hex << std::setfill('0')
        << "VID:" << std::setw(4) << di.vendor_id << " " // 厂商 ID
        << "PID:" << std::setw(4) << di.product_id << " " // 产品 ID
        << std::dec
        << "Bus:" << (int)di.bus_number << " "    // 总线编号
        << "Port:" << (int)di.port_number << " "  // 端口号
        << "Addr:" << (int)di.device_address << "] " // 设备地址
        << di.speed_str << " "                    // 速度
        << di.manufacturer << " / " << di.product;        // 制造商 / 产品名
    if (!di.serial_number.empty()) {
        oss << " SN:" << di.serial_number;            // 序列号
    }
    if (is_hid) {
        // 统计 HID 接口数量
        int hid_count = 0;
        for (const auto& cfg : di.configs) {
            for (const auto& iface : cfg.interfaces) {
                if (iface.bclass == 0x03) hid_count++;
            }
        }
        oss << " [HID x" << hid_count << "]";
    }
    return oss.str();
}

// 设备比较
bool UsbDeviceInfo::operator==(const UsbDeviceInfo& other) const {
    if (di.vendor_id != other.di.vendor_id || di.product_id != other.di.product_id)
        return false;
    if (!di.serial_number.empty() && !other.di.serial_number.empty()) {
        bool sn_match = (di.serial_number == other.di.serial_number);
        // 序列号不相等，尝试转换为小写比较
        if (!sn_match) {
            std::string sn1 = di.serial_number, sn2 = other.di.serial_number;
            std::transform(sn1.begin(), sn1.end(), sn1.begin(), ::tolower);
            std::transform(sn2.begin(), sn2.end(), sn2.begin(), ::tolower);
            sn_match = (sn1 == sn2);
        }
        return sn_match;
    }
    return di.bus_number == other.di.bus_number &&
           di.port_number == other.di.port_number &&
           di.device_address == other.di.device_address;
}

// 获取设备类型名称(已知设备列表)
const char* UsbDeviceInfo::type_name() const {
    for (size_t i = 0; i < g_known_devs_count; ++i) {
        if (g_known_devs[i].type == type) {
            return g_known_devs[i].name;
        }
    }
    return "UNKNOWN";
}
