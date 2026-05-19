// ============================================================================
// usb_device.cpp - USB 设备信息与操作模块（实现文件）
//
// 功能说明：
//   实现 UsbDevice 类的所有方法，包括：
//   - 设备引用计数管理（构造/析构/移动语义）
//   - USB 描述符解析（设备/配置/接口/端点/字符串描述符）
//   - 设备信息格式化输出（摘要/详情/树形结构）
//   - HID 接口检测与端点查找
//   - 工具函数（速度/类代码/端点类型 到字符串的转换）
// ============================================================================

#include "usb_device.hpp"
#include "libusb.h"
#include <cstring>
#include <algorithm>      // std::min 等算法

namespace usb_ctrl {
namespace core {

// ============================================================================
// speed_to_string - 将 libusb 速度常量转换为可读字符串
//
// USB 速度等级：
//   - Low Speed  : 1.5 Mbps  （USB 1.0/1.1，用于键盘鼠标等低速设备）
//   - Full Speed : 12 Mbps   （USB 1.1，通用速度）
//   - High Speed : 480 Mbps  （USB 2.0）
//   - Super Speed: 5 Gbps    （USB 3.0）
//   - Super+     : 10 Gbps   （USB 3.1）
// ============================================================================
const char* speed_to_string(int speed) {
    switch (speed) {
        case LIBUSB_SPEED_LOW:        return "Low (1.5Mbps)";   // 低速设备
        case LIBUSB_SPEED_FULL:       return "Full (12Mbps)";   // 全速设备
        case LIBUSB_SPEED_HIGH:       return "High (480Mbps)";  // 高速设备
        case LIBUSB_SPEED_SUPER:      return "Super (5Gbps)";   // 超速设备
        case LIBUSB_SPEED_SUPER_PLUS: return "Super+ (10Gbps)"; // 超速+设备
        default:                      return "Unknown";         // 未知速度
    }
}

// ============================================================================
// class_to_string - 将 USB 类代码转换为可读字符串
//
// USB 设备类代码定义（USB-IF 标准）：
//   0x00 = 复合设备（每个接口定义自己的类）
//   0x03 = HID（人机接口设备：键盘、鼠标等）
//   0x08 = Mass Storage（大容量存储：U盘、移动硬盘）
//   0x09 = Hub（USB 集线器）
//   0x0E = Video（视频设备：摄像头）
//   0xE0 = Wireless（无线控制器）
//   0xFF = Vendor Specific（厂商自定义）
// ============================================================================
const char* class_to_string(uint8_t cls) {
    switch (cls) {
        case 0x00: return "Per Interface";   // 复合设备，类定义在接口级别
        case 0x01: return "Audio";           // 音频设备
        case 0x02: return "CDC";             // 通信设备类
        case 0x03: return "HID";             // 人机接口设备
        case 0x05: return "Physical";        // 物理设备
        case 0x06: return "Image";           // 图像设备
        case 0x07: return "Printer";         // 打印机
        case 0x08: return "Mass Storage";    // 大容量存储
        case 0x09: return "Hub";             // USB 集线器
        case 0x0A: return "CDC-Data";        // 通信设备类数据
        case 0x0B: return "Smart Card";      // 智能卡
        case 0x0E: return "Video";           // 视频设备
        case 0xDC: return "Diagnostic";      // 诊断设备
        case 0xE0: return "Wireless";        // 无线控制器
        case 0xEF: return "Misc";            // 杂项设备
        case 0xFE: return "App Specific";    // 应用特定
        case 0xFF: return "Vendor Specific"; // 厂商自定义
        default:   return "Unknown";         // 未知类代码
    }
}

// ============================================================================
// ep_type_to_string - 将端点属性中的传输类型转换为可读字符串
//
// USB 端点传输类型（bmAttributes 的 bit0-1）：
//   - Control     : 控制传输（端点 0 专用，用于设备枚举和配置）
//   - Isochronous : 同步传输（用于音视频等实时数据，无错误重传）
//   - Bulk        : 批量传输（用于大容量数据，有错误重传）
//   - Interrupt   : 中断传输（用于 HID 等小数据量、定时传输）
// ============================================================================
const char* ep_type_to_string(uint8_t bmAttributes) {
    // 使用 LIBUSB_TRANSFER_TYPE_MASK 掩码提取传输类型位
    switch (bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) {
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_CONTROL:     return "Control";     // 控制传输
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_ISOCHRONOUS: return "Isochronous"; // 同步传输
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK:        return "Bulk";        // 批量传输
        case LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT:   return "Interrupt";   // 中断传输
        default:                                        return "Unknown";     // 未知类型
    }
}

// ============================================================================
// ep_dir_to_string - 将端点地址转换为方向字符串
//
// USB 端点地址 bit7 表示方向：
//   - bit7 = 1 → IN  （设备到主机）
//   - bit7 = 0 → OUT （主机到设备）
// ============================================================================
const char* ep_dir_to_string(uint8_t addr) {
    // 检查端点地址的最高位（bit7 = LIBUSB_ENDPOINT_DIR_MASK = 0x80）
    return (addr & LIBUSB_ENDPOINT_DIR_MASK) ? "IN" : "OUT";
}

// ============================================================================
// EndpointInfo 成员函数实现
// ============================================================================

// 获取端点方向字符串
std::string EndpointInfo::direction_str() const { return ep_dir_to_string(address); }

// 获取端点传输类型字符串
std::string EndpointInfo::type_str() const      { return ep_type_to_string(attributes); }

// 获取端点编号（去除方向位后的低 4 位）
int EndpointInfo::endpoint_id() const            { return address & LIBUSB_ENDPOINT_ADDRESS_MASK; }

// 判断是否为 IN 端点（bit7 = 1）
bool EndpointInfo::is_in() const                 { return (address & LIBUSB_ENDPOINT_DIR_MASK) != 0; }

// 判断是否为 OUT 端点（bit7 = 0）
bool EndpointInfo::is_out() const                { return (address & LIBUSB_ENDPOINT_DIR_MASK) == 0; }

// ============================================================================
// ConfigInfo 成员函数实现
// ============================================================================

// 判断设备是否自供电（bmAttributes bit6 = 1 表示自供电）
bool ConfigInfo::self_powered() const            { return (attributes & 0x40) != 0; }

// 判断设备是否支持远程唤醒（bmAttributes bit5 = 1 表示支持）
bool ConfigInfo::remote_wakeup() const           { return (attributes & 0x20) != 0; }

// ============================================================================
// UsbDevice 构造函数
//
// 从 libusb_device 原始指针创建 UsbDevice 对象。
// 调用 libusb_ref_device() 增加设备引用计数，防止设备在使用期间被释放。
// ============================================================================
UsbDevice::UsbDevice(libusb_device* raw_dev) : _dev(raw_dev) {
    // 增加 libusb 设备引用计数，确保设备在使用期间不被释放
    libusb_ref_device(_dev);
}

// ============================================================================
// UsbDevice 离线构造函数
//
// 从 DeviceInfo 创建 UsbDevice，设备已拔出，没有 libusb_device*。
// 用于热插拔拔出事件：设备已不在，但需要告诉回调"谁走了"。
// 此模式下 get_info() 返回保存的信息，无法打开设备或读取字符串。
// ============================================================================
UsbDevice::UsbDevice(const DeviceInfo& info) : _dev(nullptr), _saved_info(info) {}

// ============================================================================
// UsbDevice 析构函数
//
// 先关闭设备句柄（如果已打开），再减少设备引用计数。
// 当引用计数降为 0 时，libusb 会释放设备资源。
// ============================================================================
UsbDevice::~UsbDevice() {
    // 如果设备句柄已打开，先关闭它
    if (_handle) {
        libusb_close(_handle);
    }
    // 减少设备引用计数
    if (_dev) {
        libusb_unref_device(_dev);
    }
}

// ============================================================================
// UsbDevice 移动构造函数
//
// 将资源从 other 转移到当前对象，然后将 other 置为空状态。
// noexcept 保证移动操作不抛异常，使 std::vector 可以安全使用。
// ============================================================================
UsbDevice::UsbDevice(UsbDevice&& other) noexcept
    : _dev(other._dev)         // 转移设备指针
    , _handle(other._handle)   // 转移设备句柄
    , _opened(other._opened) { // 转移打开状态
    // 将源对象置为空状态，防止析构时重复释放
    other._dev = nullptr;
    other._handle = nullptr;
    other._opened = false;
}

// ============================================================================
// UsbDevice 移动赋值运算符
//
// 先释放当前对象持有的资源，再从 other 转移资源。
// 处理自赋值情况（this == &other）。
// ============================================================================
UsbDevice& UsbDevice::operator=(UsbDevice&& other) noexcept {
    // 检查自赋值
    if (this != &other) {
        // 释放当前持有的资源
        if (_handle) libusb_close(_handle);
        if (_dev) libusb_unref_device(_dev);
        // 从 other 转移资源
        _dev = other._dev;
        _handle = other._handle;
        _opened = other._opened;
        // 将源对象置为空状态
        other._dev = nullptr;
        other._handle = nullptr;
        other._opened = false;
    }
    return *this;
}

// ============================================================================
// try_open - 尝试打开设备（用于读取字符串描述符等操作）
//
// 使用 mutable 成员变量，允许在 const 方法中调用。
// 如果设备已经打开则直接返回，避免重复打开。
// 打开失败时不抛异常，仅保持 _opened = false。
// ============================================================================
void UsbDevice::try_open() const {
    // 如果已经打开，直接返回
    if (_opened) return;
    // 尝试打开设备，获取操作句柄
    int rc = libusb_open(_dev, &_handle);
    // 打开成功则标记状态
    if (rc == 0) _opened = true;
}

// ============================================================================
// vid - 快速获取厂商 ID
//
// 直接读取设备描述符中的 idVendor 字段，不读取完整设备信息。
// 读取失败时返回 0。
// ============================================================================
uint16_t UsbDevice::vid() const {
    // 设备离线：直接返回保存的信息
    if (!_dev) return _saved_info.vendor_id;
    libusb_device_descriptor desc; // 设备描述符结构体
    // 获取设备描述符，成功返回 0
    return libusb_get_device_descriptor(_dev, &desc) == LIBUSB_SUCCESS ? desc.idVendor : 0;
}

// ============================================================================
// pid - 快速获取产品 ID
//
// 直接读取设备描述符中的 idProduct 字段，不读取完整设备信息。
// 读取失败时返回 0。
// ============================================================================
uint16_t UsbDevice::pid() const {
    // 设备离线：直接返回保存的信息
    if (!_dev) return _saved_info.product_id;
    libusb_device_descriptor desc; // 设备描述符结构体
    // 获取设备描述符，成功返回 0
    return libusb_get_device_descriptor(_dev, &desc) == LIBUSB_SUCCESS ? desc.idProduct : 0;
}

// ============================================================================
// get_string_descriptor - 读取 USB 字符串描述符
//
// USB 设备可以包含多个字符串描述符（制造商、产品名、序列号等），
// 通过索引访问。需要先打开设备才能读取。
//
// @param index  字符串描述符索引（0 表示不存在）
// @param langid 语言 ID（0x0409 = 美式英语）
// @return 读取到的字符串，失败返回空字符串
// ============================================================================
std::string UsbDevice::get_string_descriptor(uint8_t index, uint16_t langid) const {
    // 索引为 0 表示该字符串描述符不存在
    if (index == 0) return "";
    // 尝试打开设备以读取字符串
    try_open();
    // 如果打开失败，返回空字符串
    if (!_handle) return "";
    unsigned char buf[512]; // 字符串缓冲区（USB 字符串最大 255 字符）
    // 调用 libusb 读取 ASCII 格式的字符串描述符
    int len = libusb_get_string_descriptor_ascii(_handle, index, buf, sizeof(buf));
    // 读取成功则构造 std::string 返回
    return len > 0 ? std::string(reinterpret_cast<char*>(buf), len) : "";
}

// ============================================================================
// _format_hex - 将二进制数据格式化为十六进制+ASCII 对照显示
//
// 输出格式示例（per_line=16）：
//   48 65 6C 6C 6F 20 57 6F 72 6C 64 21 00 00 00 00   Hello World!....
//
// @param data     原始数据指针
// @param len      数据长度
// @param per_line 每行显示的字节数
// @return 格式化后的字符串
// ============================================================================
std::string UsbDevice::_format_hex(const unsigned char* data, size_t len, size_t per_line) const {
    std::ostringstream oss; // 字符串输出流
    // 按行遍历数据
    for (size_t i = 0; i < len; i += per_line) {
        // 第一部分：十六进制显示
        for (size_t j = 0; j < per_line && (i + j) < len; ++j) {
            oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i + j]) << " "; // 每个字节两位十六进制+空格
        }
        // 补齐不足一行的空白
        for (size_t j = len - i; j < per_line && j > 0; ++j) oss << "   ";
        oss << "  "; // 分隔符
        // 第二部分：ASCII 字符显示（不可打印字符显示为 '.'）
        for (size_t j = 0; j < per_line && (i + j) < len; ++j) {
            char c = static_cast<char>(data[i + j]);
            oss << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
        }
        // 如果不是最后一行，添加换行
        if (i + per_line < len) oss << "\n";
    }
    return oss.str();
}

// ============================================================================
// get_info - 获取设备的完整描述符信息
//
// 这是 UsbDevice 最核心的方法，一次性读取并解析所有 USB 描述符：
//   1. 设备描述符（Device Descriptor）
//   2. 字符串描述符（制造商、产品名、序列号）
//   3. 配置描述符（Configuration Descriptor）
//   4. 接口描述符（Interface Descriptor）
//   5. 端点描述符（Endpoint Descriptor）
//
// @return 填充完整的 DeviceInfo 结构体
// ============================================================================
DeviceInfo UsbDevice::get_info() const {
    if (!_dev) return _saved_info;

    DeviceInfo info;

    // ---- 第 1 步：读取设备描述符 ----
    libusb_device_descriptor desc;
    int rc = libusb_get_device_descriptor(_dev, &desc);
    if (rc < 0) return info; // 读取失败返回空信息

    // 填充设备描述符基本信息
    info.vendor_id = desc.idVendor;             // 厂商 ID
    info.product_id = desc.idProduct;           // 产品 ID
    info.bcd_device = desc.bcdDevice;           // 设备版本号
    info.bcd_usb = desc.bcdUSB;                 // USB 规范版本
    info.device_class = desc.bDeviceClass;      // 设备类代码
    info.device_subclass = desc.bDeviceSubClass; // 设备子类代码
    info.device_protocol = desc.bDeviceProtocol; // 设备协议代码
    info.max_packet_size0 = desc.bMaxPacketSize0; // 端点 0 最大包大小
    info.class_str = class_to_string(desc.bDeviceClass); // 设备类名称

    // 获取设备速度、总线号、地址、端口号
    info.device_speed = libusb_get_device_speed(_dev);
    info.speed_str = speed_to_string(info.device_speed);
    info.bus_number = libusb_get_bus_number(_dev);
    info.device_address = libusb_get_device_address(_dev);
    info.port_number = libusb_get_port_number(_dev);

    // ---- 第 2 步：读取字符串描述符（需要打开设备） ----
    try_open(); // 尝试打开设备
    if (_handle) {
        // 读取制造商、产品名、序列号字符串
        info.manufacturer = get_string_descriptor(desc.iManufacturer);
        info.product = get_string_descriptor(desc.iProduct);
        info.serial_number = get_string_descriptor(desc.iSerialNumber);
    }

    // ---- 第 3 步：读取活动配置描述符 ----
    libusb_config_descriptor* cfg = nullptr;
    rc = libusb_get_active_config_descriptor(_dev, &cfg);
    if (rc == 0 && cfg) {
        ConfigInfo ci; // 创建配置信息结构体
        ci.value = cfg->bConfigurationValue;   // 配置值
        ci.num_interfaces = cfg->bNumInterfaces; // 接口数量
        ci.max_power = cfg->MaxPower;           // 最大功耗
        ci.attributes = cfg->bmAttributes;      // 配置属性
        if (_handle) ci.name = get_string_descriptor(cfg->iConfiguration); // 配置名称

        // ---- 第 4 步：遍历所有接口 ----
        for (int i = 0; i < cfg->bNumInterfaces; ++i) {
            auto& iface = cfg->interface[i]; // 获取第 i 个接口
            // 遍历接口的所有备用设置
            for (int j = 0; j < iface.num_altsetting; ++j) {
                auto& alt = iface.altsetting[j]; // 获取第 j 个备用设置
                InterfaceInfo ii; // 创建接口信息结构体
                ii.number = alt.bInterfaceNumber;       // 接口编号
                ii.alt_setting = alt.bAlternateSetting; // 备用设置编号
                ii.bclass = alt.bInterfaceClass;        // 接口类代码
                ii.subclass = alt.bInterfaceSubClass;   // 接口子类代码
                ii.protocol = alt.bInterfaceProtocol;   // 接口协议代码
                ii.i_interface = alt.iInterface;        // 接口字符串索引
                ii.class_str = class_to_string(alt.bInterfaceClass); // 接口类名称
                if (_handle) ii.name = get_string_descriptor(alt.iInterface); // 接口名称

                // ---- 第 5 步：遍历接口的所有端点 ----
                for (int k = 0; k < alt.bNumEndpoints; ++k) {
                    auto& ep = alt.endpoint[k]; // 获取第 k 个端点
                    EndpointInfo ei; // 创建端点信息结构体
                    ei.address = ep.bEndpointAddress;       // 端点地址
                    ei.attributes = ep.bmAttributes;        // 端点属性
                    ei.max_packet_size = ep.wMaxPacketSize; // 最大包大小
                    ei.interval = ep.bInterval;             // 轮询间隔
                    ii.endpoints.push_back(ei); // 添加到接口的端点列表
                }
                ci.interfaces.push_back(std::move(ii)); // 添加到配置的接口列表
            }
        }
        info.configs.push_back(std::move(ci)); // 添加到设备的配置列表
        libusb_free_config_descriptor(cfg); // 释放配置描述符内存
    }
    return info;
}

// ============================================================================
// _config_details - 生成配置描述符的详细格式化输出
//
// 输出完整的 USB 设备描述符树，包括：
//   - 设备信息摘要
//   - 设备描述符各字段
//   - 配置描述符各字段
//   - 接口描述符各字段
//   - 端点描述符各字段
//
// @param info 已填充的设备信息结构体
// @return 格式化的多行字符串
// ============================================================================
std::string UsbDevice::_config_details(const DeviceInfo& info) const {
    std::ostringstream oss; // 字符串输出流

    // ---- 设备信息标题 ----
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

    // ---- 设备描述符详情 ----
    oss << "  ---------------------- Device Descriptor ----------------------\n";
    oss << std::hex << std::uppercase << std::setfill('0'); // 设置十六进制大写格式
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

    // ---- 遍历每个配置描述符 ----
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

        // ---- 遍历每个接口描述符 ----
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

            // ---- 遍历每个端点描述符 ----
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

// ============================================================================
// summary - 获取设备简要摘要信息（单行格式）
//
// 输出格式示例：
//   VID:0x046D PID:0xC077 Speed:Full (12Mbps) USB:2.0 [USB Optical Mouse] (Logitech) Ifaces:1
// ============================================================================
std::string UsbDevice::summary() const {
    // 设备离线：用保存的信息生成摘要
    if (!_dev) {
        std::ostringstream oss;
        oss << std::uppercase << std::hex << std::setfill('0');
        oss << "VID:0x" << std::setw(4) << _saved_info.vendor_id
            << " PID:0x" << std::setw(4) << _saved_info.product_id
            << " [disconnected]";
        return oss.str();
    }
    auto info = get_info(); // 获取设备完整信息
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0'); // 设置十六进制大写格式
    oss << "VID:0x" << std::setw(4) << info.vendor_id       // 厂商 ID
        << " PID:0x" << std::setw(4) << info.product_id      // 产品 ID
        << " Speed:" << info.speed_str                       // 设备速度
        << " USB:" << ((info.bcd_usb >> 8) & 0xFF) << "." << (info.bcd_usb & 0xFF); // USB 版本
    if (!info.product.empty()) oss << " [" << info.product << "]";       // 产品名称
    if (!info.manufacturer.empty()) oss << " (" << info.manufacturer << ")"; // 制造商
    if (!info.configs.empty())
        oss << " Ifaces:" << static_cast<int>(info.configs[0].num_interfaces); // 接口数量
    return oss.str();
}

// ============================================================================
// detail - 获取设备详细信息（多行格式）
//
// 包含设备摘要和完整的描述符树信息。
// ============================================================================
std::string UsbDevice::detail() const {
    auto info = get_info(); // 获取设备完整信息
    std::ostringstream oss;

    // ---- 设备摘要部分 ----
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
        // 统计所有端点数量
        int ep_count = 0;
        for (auto& iface : info.configs[0].interfaces) ep_count += static_cast<int>(iface.endpoints.size());
        oss << "Used Endpoints           : " << ep_count << "\n";
    }
    oss << std::dec; // 恢复十进制格式

    // ---- 详细描述符部分 ----
    oss << _config_details(info);
    return oss.str();
}

// ============================================================================
// descriptor_tree - 获取设备描述符树形结构（层级缩进格式）
//
// 输出格式示例：
//   Device: USB Optical Mouse (0x046D:0xC077)
//     Config 1
//       Interface 0 [HID]
//         EP 0x81 Interrupt IN (8B)
// ============================================================================
std::string UsbDevice::descriptor_tree() const {
    auto info = get_info(); // 获取设备完整信息
    std::ostringstream oss;

    // 设备根节点
    oss << "Device: " << info.product << " (";
    oss << std::hex << std::uppercase << "0x" << info.vendor_id << ":0x" << info.product_id << std::dec << ")\n";

    // 遍历配置
    for (const auto& cfg : info.configs) {
        oss << "  Config " << static_cast<int>(cfg.value) << "\n"; // 缩进 2 空格
        // 遍历接口
        for (const auto& iface : cfg.interfaces) {
            oss << "    Interface " << static_cast<int>(iface.number)
                << " [" << iface.class_str << "]"; // 缩进 4 空格
            if (!iface.name.empty()) oss << " \"" << iface.name << "\"";
            oss << "\n";
            // 遍历端点
            for (const auto& ep : iface.endpoints) {
                oss << "      EP 0x" << std::hex << static_cast<int>(ep.address) << std::dec
                    << " " << ep.type_str() << " " << ep.direction_str()
                    << " (" << ep.max_packet_size << "B)\n"; // 缩进 6 空格
            }
        }
    }
    return oss.str();
}

// ============================================================================
// has_hid_interface - 检查设备是否包含 HID 接口
//
// 遍历所有配置的所有接口，查找类代码为 LIBUSB_CLASS_HID (0x03) 的接口。
// @return true 如果找到至少一个 HID 接口
// ============================================================================
bool UsbDevice::has_hid_interface() const {
    auto info = get_info(); // 获取设备完整信息
    // 遍历所有配置
    for (const auto& cfg : info.configs)
        // 遍历所有接口
        for (const auto& iface : cfg.interfaces)
            // 检查接口类代码是否为 HID (0x03)
            if (iface.bclass == LIBUSB_CLASS_HID) return true;
    return false;
}

// ============================================================================
// find_hid_interface - 查找设备的 HID 接口及其端点信息
//
// 找到第一个 HID 接口后，提取其中的 IN 和 OUT 中断端点信息。
// HID 设备通常使用中断端点进行数据传输。
//
// @param[out] iface_num 找到的 HID 接口编号
// @param[out] in_ep     找到的 IN 中断端点信息
// @param[out] out_ep    找到的 OUT 中断端点信息
// @return true 如果找到 HID 接口
// ============================================================================
bool UsbDevice::find_hid_interface(int& iface_num, EndpointInfo& in_ep, EndpointInfo& out_ep) const {
    auto info = get_info(); // 获取设备完整信息
    // 遍历所有配置
    for (const auto& cfg : info.configs) {
        // 遍历所有接口
        for (const auto& iface : cfg.interfaces) {
            // 找到 HID 类接口
            if (iface.bclass == LIBUSB_CLASS_HID) {
                iface_num = static_cast<int>(iface.number); // 记录接口编号
                // 遍历该接口的所有端点
                for (const auto& ep : iface.endpoints) {
                    // 查找 IN 方向的中断端点
                    if (ep.is_in() && ep.type_str() == std::string("Interrupt"))
                        in_ep = ep;
                    // 查找 OUT 方向的中断端点
                    if (ep.is_out() && ep.type_str() == std::string("Interrupt"))
                        out_ep = ep;
                }
                return true; // 找到第一个 HID 接口后立即返回
            }
        }
    }
    return false; // 未找到 HID 接口
}

} // namespace core
} // namespace usb_ctrl
