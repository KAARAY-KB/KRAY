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
#include "usb_core/usb_device.hpp"
#include "console.h"
#include <sstream>
#include <iomanip>

static std::string _dn = "[DevInfo]";

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
    Console::out() << _dn << " from_usb_device: start extracting device info" << std::endl;
    UsbDeviceInfo info;
    // 获取设备完整信息
    auto di = dev.get_info();
    info.vid = di.vendor_id;          // 厂商 ID
    info.pid = di.product_id;         // 产品 ID
    info.bus = di.bus_number;         // 总线编号
    info.port = di.port_number;       // 端口号
    info.addr = di.device_address;    // 设备地址
    info.mfr = di.manufacturer;       // 制造商
    info.prod = di.product;           // 产品名称
    info.sn = di.serial_number;       // 序列号
    info.type = detect_type(info.vid, info.pid); // 检测设备类型
    Console::out() << _dn << " from_usb_device: extracted -> " << info.to_string() << std::endl;
    return info;
}

// 根据 VID/PID 检测设备类型
UsbDeviceInfo::DevType UsbDeviceInfo::detect_type(uint16_t vid, uint16_t pid) {
    Console::out() << _dn << " detect_type: checking VID=0x"
                   << std::hex << vid << " PID=0x" << pid << std::dec << std::endl;
    for (size_t i = 0; i < g_known_devs_count; ++i) {
        if (g_known_devs[i].vid == vid && g_known_devs[i].pid == pid) {
            Console::out() << _dn << " detect_type: matched known device -> " << g_known_devs[i].name << std::endl;
            return g_known_devs[i].type; // 匹配到已知设备
        }
    }
    Console::out() << _dn << " detect_type: no match found, type=UNKNOWN" << std::endl;
    return DEV_UNKNOWN; // 未知设备
}

// 格式化为可读字符串
std::string UsbDeviceInfo::to_string() const {
    std::ostringstream oss;
    oss << type_name() << " ["          // 设备类型名
        << std::hex << std::setfill('0')
        << "VID:" << std::setw(4) << vid << " " // 厂商 ID
        << "PID:" << std::setw(4) << pid << " " // 产品 ID
        << std::dec
        << "Bus:" << (int)bus << " "    // 总线编号
        << "Port:" << (int)port << " "  // 端口号
        << "Addr:" << (int)addr << "] " // 设备地址
        << mfr << " / " << prod;        // 制造商 / 产品名
    if (!sn.empty()) {
        oss << " SN:" << sn;            // 序列号
    }
    return oss.str();
}

// 设备比较
bool UsbDeviceInfo::operator==(const UsbDeviceInfo& other) const {
    bool match = vid == other.vid && pid == other.pid &&
                 bus == other.bus && port == other.port && addr == other.addr;
    Console::out() << _dn << " operator==: " << (match ? "MATCH" : "MISMATCH")
                   << " this=" << to_string()
                   << " other=" << other.to_string() << std::endl;
    return match;
}

// 获取设备类型名称
const char* UsbDeviceInfo::type_name() const {
    // 在已知设备列表中查找名称
    for (size_t i = 0; i < g_known_devs_count; ++i) {
        if (g_known_devs[i].type == type) {
            return g_known_devs[i].name;
        }
    }
    return "UNKNOWN";
}
