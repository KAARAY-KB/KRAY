#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <libusb.h>
#include <string>
#include <vector>

// USB设备操作封装类
// 提供设备打开/关闭、接口管理、数据传输等功能
class UsbDevice {
public:
    // 构造函数：初始化成员变量
    UsbDevice();
    
    // 析构函数：确保设备关闭
    ~UsbDevice();

    // 打开USB设备
    // 参数: dev - 设备指针
    // 返回: true成功, false失败
    bool open(libusb_device* dev);
    
    // 设置设备配置
    // 参数: configuration - 配置值
    // 返回: true成功, false失败
    bool setConfiguration(int configuration);
    
    // 关闭USB设备，释放所有接口
    void close();

    // 声明接口（使用设备前必须声明）
    // 参数: interface_num - 接口编号
    // 返回: true成功, false失败
    bool claimInterface(int interface_num);
    
    // 分离内核驱动（Linux下需要）
    // 参数: interface_num - 接口编号
    // 返回: true成功, false失败
    bool detachKernelDriver(int interface_num);
    
    // 释放接口
    // 参数: interface_num - 接口编号
    void releaseInterface(int interface_num);

    // 控制传输（端点0）
    // 参数: bmRequestType - 请求类型, bRequest - 请求码
    //      wValue - 值, wIndex - 索引
    //      data - 数据缓冲区, wLength - 数据长度
    //      timeout - 超时时间(ms)
    // 返回: 实际传输字节数或错误码
    int controlTransfer(uint8_t bmRequestType, uint8_t bRequest,
                        uint16_t wValue, uint16_t wIndex,
                        unsigned char* data, uint16_t wLength,
                        unsigned int timeout);

    // 批量传输
    // 参数: endpoint - 端点地址, data - 数据缓冲区
    //      length - 数据长度, transferred - 实际传输长度指针
    //      timeout - 超时时间(ms)
    // 返回: LIBUSB_SUCCESS成功或错误码
    int bulkTransfer(uint8_t endpoint, unsigned char* data,
                     int length, int* transferred,
                     unsigned int timeout);

    // 中断传输
    // 参数: endpoint - 端点地址, data - 数据缓冲区
    //      length - 数据长度, transferred - 实际传输长度指针
    //      timeout - 超时时间(ms)
    // 返回: LIBUSB_SUCCESS成功或错误码
    int interruptTransfer(uint8_t endpoint, unsigned char* data,
                          int length, int* transferred,
                          unsigned int timeout);

    // 获取字符串描述符
    // 参数: desc_index - 描述符索引, str - 输出字符串
    // 返回: 字符串长度或错误码
    int getStringDescriptor(uint8_t desc_index, std::string& str);
    
    // 获取设备描述符
    // 参数: desc - 设备描述符结构
    // 返回: LIBUSB_SUCCESS成功或错误码
    int getDeviceDescriptor(libusb_device_descriptor& desc);
    
    // 获取配置描述符
    // 参数: config_index - 配置索引, config - 配置描述符指针
    // 返回: LIBUSB_SUCCESS成功或错误码
    int getConfigDescriptor(uint8_t config_index, libusb_config_descriptor*& config);

    // 释放配置描述符
    // 参数: config - 配置描述符指针
    void freeConfigDescriptor(libusb_config_descriptor* config);

    // 检查设备是否打开
    // 返回: true已打开, false未打开
    bool isOpen() const;
    
    // 获取设备句柄
    // 返回: libusb_device_handle指针
    libusb_device_handle* getHandle() const;

private:
    libusb_device_handle* handle_;       // 设备句柄
    std::vector<int> claimed_interfaces_; // 已声明的接口列表
};

#endif
