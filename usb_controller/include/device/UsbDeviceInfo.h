#ifndef USB_DEVICE_INFO_H
#define USB_DEVICE_INFO_H

#include <libusb.h>
#include <string>
#include <vector>
#include <iostream>

// 端点信息结构体
// 存储USB端点的详细配置信息
struct EndpointInfo {
    uint8_t address;           // 端点地址（包含方向位）
    uint8_t transfer_type;     // 传输类型（控制/批量/中断/等时）
    uint16_t max_packet_size;  // 最大包大小
    uint8_t interval;          // 轮询间隔
};

// 接口信息结构体
// 存储USB接口的配置和端点列表
struct InterfaceInfo {
    uint8_t interface_num;         // 接口编号
    uint8_t alt_setting;           // 备用设置编号
    uint8_t interface_class;       // 接口类代码
    uint8_t interface_subclass;    // 接口子类代码
    uint8_t interface_protocol;    // 接口协议
    std::vector<EndpointInfo> endpoints;  // 端点列表
};

// 配置信息结构体
// 存储USB配置的完整信息
struct ConfigInfo {
    uint8_t config_value;          // 配置值
    uint16_t total_length;         // 配置描述符总长度
    uint8_t num_interfaces;        // 接口数量
    uint8_t attributes;            // 配置属性
    uint8_t max_power;             // 最大功耗
    std::vector<InterfaceInfo> interfaces;  // 接口列表
};

// 设备信息结构体
// 存储USB设备的完整描述信息
struct DeviceInfo {
    uint16_t vendor_id;            // 厂商ID
    uint16_t product_id;           // 产品ID
    uint16_t usb_version;          // USB版本
    uint8_t device_class;          // 设备类代码
    uint8_t device_subclass;       // 设备子类代码
    uint8_t device_protocol;       // 设备协议
    uint8_t device_version;        // 设备版本号
    std::string manufacturer;      // 制造商字符串
    std::string product;           // 产品字符串
    std::string serial_number;     // 序列号字符串
    uint8_t bus_number;            // 总线编号
    uint8_t device_address;        // 设备地址
    std::vector<ConfigInfo> configs;  // 配置列表
};

// USB设备信息解析类
// 负责解析和打印USB设备的完整信息
class UsbDeviceInfo {
public:
    // 解析设备信息
    // 参数: dev - 设备指针, handle - 设备句柄
    // 返回: DeviceInfo结构体
    static DeviceInfo parse(libusb_device* dev, libusb_device_handle* handle);
    
    // 打印设备信息到控制台
    // 参数: info - 设备信息结构体
    static void print(const DeviceInfo& info);
    
    // 获取速度名称
    // 参数: speed - 速度代码
    // 返回: 速度名称字符串
    static std::string getSpeedName(uint8_t speed);
    
    // 获取类名称
    // 参数: class_code - 类代码
    // 返回: 类名称字符串
    static std::string getClassName(uint8_t class_code);
    
    // 获取传输类型名称
    // 参数: type - 传输类型代码
    // 返回: 传输类型名称字符串
    static std::string getTransferTypeName(uint8_t type);
};

#endif
