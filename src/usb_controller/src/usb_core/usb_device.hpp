// ============================================================================
// usb_device.hpp - USB 设备信息与操作模块（头文件）
//
// 功能说明：
//   本文件定义了 USB 设备相关的数据结构（EndpointInfo、InterfaceInfo、
//   ConfigInfo、DeviceInfo）和 UsbDevice 类。
//   UsbDevice 是对 libusb_device 的 C++ 封装，提供设备信息查询、
//   描述符解析、HID 接口检测等功能。
//
// 核心数据结构层级关系：
//   DeviceInfo
//     └── ConfigInfo[]          （配置描述符数组）
//           └── InterfaceInfo[] （接口描述符数组）
//                 └── EndpointInfo[] （端点描述符数组）
//
// 依赖关系：
//   - libusb.h：底层 USB 设备操作
//   - <cstdint>：定长整数类型
//   - <string>：字符串支持
//   - <vector>：动态数组
//   - <sstream>：字符串流格式化
//   - <iomanip>：输出格式控制
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <sstream>  // std::ostringstream 字符串输出流
#include <iomanip>  // std::hex, std::setw 等格式化操纵符

// 前向声明 libusb 相关结构体，减少头文件依赖
struct libusb_device;              // USB 设备原始指针
struct libusb_device_handle;      // 已打开设备的操作句柄
struct libusb_device_descriptor;  // 设备描述符
struct libusb_config_descriptor;  // 配置描述符

namespace usb_ctrl {
namespace core {

// ============================================================================
// 工具函数：将 USB 速度常量转换为可读字符串
// ============================================================================
const char* speed_to_string(int speed);

// ============================================================================
// 工具函数：将 USB 设备类代码转换为可读字符串
// ============================================================================
const char* class_to_string(uint8_t cls);

// ============================================================================
// 工具函数：将端点传输类型转换为可读字符串
// ============================================================================
const char* ep_type_to_string(uint8_t bmAttributes);

// ============================================================================
// 工具函数：将端点地址转换为方向字符串（"IN" 或 "OUT"）
// ============================================================================
const char* ep_dir_to_string(uint8_t addr);

// ============================================================================
// EndpointInfo - USB 端点描述符信息结构体
//
// 对应 USB 规范中的端点描述符（Endpoint Descriptor）。
// 每个 USB 接口可以包含多个端点，每个端点有独立的地址、传输类型、
// 最大包大小和轮询间隔。
// ============================================================================
struct EndpointInfo {
    uint8_t address = 0;          // 端点地址（bit7=方向, bit0-3=端点号）
    uint8_t attributes = 0;       // 端点属性（bit0-1=传输类型）
    uint16_t max_packet_size = 0; // 最大数据包大小（字节）
    uint8_t interval = 0;         // 数据传输间隔（帧/微帧数）

    // 获取端点方向字符串："IN"（设备到主机）或 "OUT"（主机到设备）
    std::string direction_str() const;

    // 获取传输类型字符串："Control"/"Bulk"/"Interrupt"/"Isochronous"
    std::string type_str() const;

    // 获取端点编号（0-15），不含方向位
    int endpoint_id() const;

    // 判断是否为 IN 端点（设备→主机）
    bool is_in() const;

    // 判断是否为 OUT 端点（主机→设备）
    bool is_out() const;
};

// ============================================================================
// InterfaceInfo - USB 接口描述符信息结构体
//
// 对应 USB 规范中的接口描述符（Interface Descriptor）。
// 一个 USB 配置可以包含多个接口，每个接口代表一种设备功能。
// 例如：一个 USB 键盘可能有一个 HID 接口。
// ============================================================================
struct InterfaceInfo {
    uint8_t number = 0;       // 接口编号（从 0 开始）
    uint8_t alt_setting = 0;  // 备用设置编号（用于同一接口的不同配置）
    uint8_t bclass = 0;       // 接口类代码（如 0x03=HID, 0x08=Mass Storage）
    uint8_t subclass = 0;     // 接口子类代码
    uint8_t protocol = 0;     // 接口协议代码
    uint8_t i_interface = 0;  // 接口字符串描述符索引
    std::string name;         // 接口名称（从字符串描述符读取）
    std::string class_str;    // 接口类的可读名称（如 "HID"）
    std::vector<EndpointInfo> endpoints; // 该接口包含的所有端点
};

// ============================================================================
// ConfigInfo - USB 配置描述符信息结构体
//
// 对应 USB 规范中的配置描述符（Configuration Descriptor）。
// 每个 USB 设备至少有一个配置，配置定义了设备的供电方式和接口集合。
// ============================================================================
struct ConfigInfo {
    uint8_t value = 0;          // 配置值（用于 SetConfiguration 请求）
    uint8_t num_interfaces = 0; // 该配置包含的接口数量
    uint8_t max_power = 0;      // 最大功耗（单位：2mA，实际功耗 = max_power * 2 mA）
    uint8_t attributes = 0;     // 配置属性（bit6=自供电, bit5=远程唤醒）
    std::string name;           // 配置名称（从字符串描述符读取）

    // 判断设备是否为自供电（不使用 USB 总线供电）
    bool self_powered() const;

    // 判断设备是否支持远程唤醒
    bool remote_wakeup() const;

    std::vector<InterfaceInfo> interfaces; // 该配置包含的所有接口
};

// ============================================================================
// DeviceInfo - USB 设备完整信息结构体
//
// 聚合了设备描述符、配置描述符、接口描述符和端点描述符的所有信息。
// 通过 UsbDevice::get_info() 方法一次性获取并填充。
// ============================================================================
struct DeviceInfo {
    uint16_t vendor_id = 0;       // 厂商 ID（VID），由 USB-IF 分配
    uint16_t product_id = 0;      // 产品 ID（PID），由厂商自定义
    uint16_t bcd_device = 0;      // 设备版本号（BCD 格式）
    uint16_t bcd_usb = 0;         // 支持的 USB 规范版本（BCD 格式）
    uint8_t device_class = 0;     // 设备类代码（0x00=复合设备, 0x03=HID 等）
    uint8_t device_subclass = 0;  // 设备子类代码
    uint8_t device_protocol = 0;  // 设备协议代码
    uint8_t max_packet_size0 = 0; // 端点 0 的最大包大小（控制传输使用）
    int device_speed = 0;         // 设备速度（低速/全速/高速/超速）
    uint8_t bus_number = 0;       // 设备所在 USB 总线编号
    uint8_t device_address = 0;   // 设备在总线上的地址
    uint8_t port_number = 0;      // 设备连接的端口号
    std::string manufacturer;     // 制造商字符串
    std::string product;          // 产品名称字符串
    std::string serial_number;    // 序列号字符串
    std::string class_str;        // 设备类的可读名称
    std::string speed_str;        // 速度的可读名称
    std::vector<ConfigInfo> configs; // 设备的所有配置信息
};

// ============================================================================
// UsbDevice - USB 设备封装类
//
// 对 libusb_device 的 C++ 面向对象封装。
// 管理设备引用计数（通过 libusb_ref_device / libusb_unref_device），
// 支持移动语义，禁止拷贝。
// 提供设备信息查询、描述符解析、HID 接口检测等功能。
// ============================================================================
class UsbDevice {
public:
    // 构造函数：从 libusb 原始设备指针创建 UsbDevice
    // 会增加设备的引用计数
    // @param raw_dev libusb_device* 原始指针
    explicit UsbDevice(libusb_device* raw_dev);

    // 构造函数：从设备信息创建 UsbDevice（设备已拔出，无法操作）
    // 用于热插拔拔出事件：设备已不在，但需要告诉回调"谁走了"
    // @param info 设备信息记录
    explicit UsbDevice(const DeviceInfo& info);

    // 析构函数：关闭设备句柄并减少引用计数
    ~UsbDevice();

    // 禁止拷贝（设备所有权唯一）
    UsbDevice(const UsbDevice&) = delete;
    UsbDevice& operator=(const UsbDevice&) = delete;

    // 支持移动构造和移动赋值（用于 std::vector 等容器）
    UsbDevice(UsbDevice&& other) noexcept;
    UsbDevice& operator=(UsbDevice&& other) noexcept;

    // 获取底层 libusb_device 原始指针
    // 设备离线时（已拔出）返回 nullptr
    libusb_device* handle() const { return _dev; }

    // 检查设备是否离线（已拔出，只剩信息记录，无法再操作设备）
    bool is_offline() const { return _dev == nullptr; }

    // 获取设备的完整信息（包括所有描述符）
    // 该方法会尝试打开设备以读取字符串描述符
    // @return DeviceInfo 结构体，包含所有设备信息
    DeviceInfo get_info() const;

    // 读取 USB 字符串描述符
    // @param index  字符串描述符索引（0 表示无字符串）
    // @param langid 语言 ID，默认为 0x0409（美式英语）
    // @return 字符串内容，失败返回空字符串
    std::string get_string_descriptor(uint8_t index, uint16_t langid = 0x0409) const;

    // 检查设备是否包含 HID 接口
    // @return true 如果设备至少有一个 HID 类接口
    bool has_hid_interface() const;

    // 查找设备的 HID 接口及其端点信息
    // @param[out] iface_num 找到的 HID 接口编号
    // @param[out] in_ep     找到的 IN 中断端点信息
    // @param[out] out_ep    找到的 OUT 中断端点信息
    // @return true 如果找到 HID 接口
    bool find_hid_interface(int& iface_num, EndpointInfo& in_ep, EndpointInfo& out_ep) const;

    // 获取设备简要摘要信息（单行格式）
    // @return 包含 VID/PID/速度/产品名 的摘要字符串
    std::string summary() const;

    // 获取设备详细信息（多行格式，包含所有描述符）
    // @return 格式化的设备详细信息字符串
    std::string detail() const;

    // 获取设备描述符树形结构（层级缩进格式）
    // @return 树形结构的设备描述符字符串
    std::string descriptor_tree() const;

    // 快速获取厂商 ID（不读取完整描述符）
    // @return 厂商 ID
    uint16_t vid() const;

    // 快速获取产品 ID（不读取完整描述符）
    // @return 产品 ID
    uint16_t pid() const;

private:
    // 尝试打开设备（用于读取字符串描述符）
    // 如果已打开则直接返回，避免重复打开
    void try_open() const;

    libusb_device* _dev = nullptr;               // libusb 设备原始指针
    mutable libusb_device_handle* _handle = nullptr; // 设备操作句柄（mutable 允许在 const 方法中修改）
    mutable bool _opened = false;                // 标记设备是否已打开
    DeviceInfo _saved_info;                       // 保存的设备信息（设备离线时使用）

    // 将二进制数据格式化为十六进制+ASCII 对照显示
    // @param data     原始数据指针
    // @param len      数据长度
    // @param per_line 每行显示的字节数，默认 16
    // @return 格式化后的字符串
    std::string _format_hex(const unsigned char* data, size_t len, size_t per_line = 16) const;

    // 生成配置描述符的详细格式化输出
    // @param info 设备信息结构体
    // @return 格式化的配置详情字符串
    std::string _config_details(const DeviceInfo& info) const;
};

} // namespace core
} // namespace usb_ctrl
