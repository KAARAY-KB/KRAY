#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <sstream>

// 前向声明 libusb 相关结构体
struct libusb_device;
struct libusb_device_handle;
struct libusb_device_descriptor;
struct libusb_config_descriptor;
struct libusb_interface_descriptor;
struct libusb_endpoint_descriptor;
struct libusb_interface_association_descriptor;

namespace usb_core {

// USB 速度描述字符串转换函数
const char* usb_speed_to_string(int speed);
// USB 设备类描述字符串转换函数
const char* usb_class_to_string(uint8_t cls);
// 端点传输类型描述字符串转换函数
const char* endpoint_type_to_string(uint8_t bmAttributes);
// 端点方向描述字符串转换函数
const char* endpoint_direction_to_string(uint8_t addr);

// USB 端点信息结构体
// 封装单个端点的描述符字段
struct UsbEndpointInfo {
    uint8_t address = 0;          // 端点地址 (包含方向和编号)
    uint8_t attributes = 0;       // 端点属性 (传输类型等)
    uint16_t max_packet_size = 0; // 最大包大小
    uint8_t interval = 0;         // 轮询间隔

    std::string direction_str() const; // 获取方向描述 ("IN" 或 "OUT")
    std::string type_str() const;      // 获取传输类型描述 ("Bulk", "Interrupt" 等)
    int endpoint_id() const;           // 获取端点编号 (0-15)
    bool is_in() const;                // 判断是否为 IN 端点 (设备到主机)
    bool is_out() const;               // 判断是否为 OUT 端点 (主机到设备)
};

// USB 接口信息结构体
// 封装单个接口及其所有端点的信息
struct UsbInterfaceInfo {
    uint8_t number = 0;         // 接口编号
    uint8_t alt_setting = 0;    // 备用设置编号
    uint8_t bclass = 0;         // 接口类代码
    uint8_t subclass = 0;       // 接口子类代码
    uint8_t protocol = 0;       // 接口协议代码
    uint8_t i_interface = 0;    // 接口描述字符串索引
    std::string name;           // 接口描述字符串内容
    std::string class_str;      // 接口类描述字符串
    std::vector<UsbEndpointInfo> endpoints; // 该接口下的所有端点
};

// USB 配置信息结构体
// 封装单个配置及其所有接口的信息
struct UsbConfigInfo {
    uint8_t value = 0;              // 配置值
    uint8_t num_interfaces = 0;     // 接口数量
    uint8_t max_power = 0;          // 最大功耗 (单位: 2mA)
    uint8_t attributes = 0;         // 配置属性
    std::string name;               // 配置描述字符串
    bool self_powered() const;      // 判断是否为自供电设备
    bool remote_wakeup() const;     // 判断是否支持远程唤醒
    std::vector<UsbInterfaceInfo> interfaces; // 该配置下的所有接口
    std::vector<std::string> iad_list;        // 接口关联描述符列表
};

// USB 设备信息结构体
// 封装完整的设备描述符信息，包括所有配置和接口
struct UsbDeviceInfo {
    uint16_t vendor_id = 0;         // 厂商 ID (VID)
    uint16_t product_id = 0;        // 产品 ID (PID)
    uint16_t bcd_device = 0;        // 设备版本号 (BCD 格式)
    uint16_t bcd_usb = 0;           // USB 规范版本号 (BCD 格式)
    uint8_t device_class = 0;       // 设备类代码
    uint8_t device_subclass = 0;    // 设备子类代码
    uint8_t device_protocol = 0;    // 设备协议代码
    uint8_t max_packet_size0 = 0;   // 端点 0 最大包大小
    int device_speed = 0;           // 设备速度 (libusb_speed 枚举值)
    uint8_t bus_number = 0;         // 总线编号
    uint8_t device_address = 0;     // 设备地址
    uint8_t port_number = 0;        // 端口编号
    std::string manufacturer;       // 厂商描述字符串
    std::string product;            // 产品描述字符串
    std::string serial_number;      // 序列号字符串
    std::string class_str;          // 设备类描述字符串
    std::string speed_str;          // 设备速度描述字符串
    std::vector<UsbConfigInfo> configs; // 所有配置信息
};

// USB 设备包装类
// 封装 libusb_device 指针，提供描述符访问和信息格式化功能
// 使用引用计数管理设备生命周期
class UsbDevice {
public:
    explicit UsbDevice(libusb_device* raw_dev);
    ~UsbDevice();

    // 禁止拷贝，支持移动语义
    UsbDevice(const UsbDevice&) = delete;
    UsbDevice& operator=(const UsbDevice&) = delete;

    UsbDevice(UsbDevice&& other) noexcept;
    UsbDevice& operator=(UsbDevice&& other) noexcept;

    // 获取底层 libusb_device 指针
    libusb_device* handle() const { return _dev; }

    // 获取完整的设备信息 (包括所有配置和接口)
    UsbDeviceInfo get_info() const;

    // 获取字符串描述符内容 (厂商名、产品名、序列号等)
    std::string get_string_descriptor(uint8_t index, uint16_t langid = 0x0409) const;

    // 判断设备是否包含 HID 接口
    bool has_hid_interface() const;

    // 查找 HID 接口，返回接口编号和 IN/OUT 端点信息
    bool find_hid_interface(int& interface_number,
                            UsbEndpointInfo& in_endpoint,
                            UsbEndpointInfo& out_endpoint) const;

    // 获取设备摘要信息 (一行文本，适合列表显示)
    std::string summary() const;

    // 获取设备详细信息 (多行文本，包含所有描述符)
    std::string detail() const;

    // 获取厂商 ID
    uint16_t vid() const;

    // 获取产品 ID
    uint16_t pid() const;

private:
    // 尝试打开设备 (如果尚未打开)
    void try_open() const;

    libusb_device* _dev = nullptr;
    mutable libusb_device_handle* _handle = nullptr; // 可变的设备句柄 (延迟打开)
    mutable bool _opened = false;

    // 内部辅助函数：格式化配置详情
    std::string _get_config_details(const UsbDeviceInfo& info) const;
    // 内部辅助函数：格式化描述符十六进制转储
    std::string _get_descriptor_hexdump(const unsigned char* data, int length) const;
    // 内部辅助函数：格式化字节数组为十六进制字符串
    std::string _format_hex(const unsigned char* data, size_t len, size_t bytes_per_line = 16) const;
};

} // namespace usb_core
