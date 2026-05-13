#include "core/UsbDevice.h"
#include <iostream>
#include <algorithm>

// 构造函数：初始化设备句柄为空
UsbDevice::UsbDevice()
    : handle_(nullptr)
{
}

// 析构函数：如果设备打开则关闭
UsbDevice::~UsbDevice()
{
    if (isOpen()) {
        close();
    }
}

// 打开USB设备
// 参数: dev - 设备指针
// 返回: true成功, false失败
bool UsbDevice::open(libusb_device* dev)
{
    // 调用libusb_open打开设备
    int ret = libusb_open(dev, &handle_);
    if (ret != LIBUSB_SUCCESS) {
        // 打开失败，打印错误
        std::cerr << "libusb open failed: " << libusb_error_name(ret) << std::endl;
        handle_ = nullptr;
        return false;
    }
    return true;
}

// 设置设备配置
// 参数: configuration - 配置值
// 返回: true成功, false失败
bool UsbDevice::setConfiguration(int configuration)
{
    int ret = libusb_set_configuration(handle_, configuration);
    if (ret != LIBUSB_SUCCESS) {
        std::cerr << "libusb set configuration failed: " << libusb_error_name(ret) << std::endl;
        return false;
    }
    return true;
}

// 关闭USB设备，释放所有已声明的接口
void UsbDevice::close()
{
    if (handle_) {
        // 释放所有已声明的接口
        for (int iface : claimed_interfaces_) {
            libusb_release_interface(handle_, iface);
        }
        claimed_interfaces_.clear();  // 清空接口列表
        libusb_close(handle_);        // 关闭设备
        handle_ = nullptr;            // 清空句柄
    }
}

// 声明USB接口（使用设备前必须声明）
// 参数: interface_num - 接口编号
// 返回: true成功, false失败
bool UsbDevice::claimInterface(int interface_num)
{
    // 调用libusb_claim_interface声明接口
    int ret = libusb_claim_interface(handle_, interface_num);
    if (ret != LIBUSB_SUCCESS) {
        // 声明失败，打印错误
        std::cerr << "libusb claim interface " << interface_num
                  << " failed: " << libusb_error_name(ret) << std::endl;
        return false;
    }
    // 成功，添加到已声明列表
    claimed_interfaces_.push_back(interface_num);
    return true;
}

// 分离内核驱动（Linux下需要）
// 参数: interface_num - 接口编号
// 返回: true成功, false失败
bool UsbDevice::detachKernelDriver(int interface_num)
{
    // 检查是否支持内核驱动分离
    if (libusb_kernel_driver_active(handle_, interface_num)) {
        // 分离内核驱动
        int ret = libusb_detach_kernel_driver(handle_, interface_num);
        if (ret != LIBUSB_SUCCESS) {
            std::cerr << "libusb detach kernel driver " << interface_num
                      << " failed: " << libusb_error_name(ret) << std::endl;
            return false;
        }
        std::cout << "Kernel driver detached from interface " << interface_num << std::endl;
    }
    return true;
}

// 释放USB接口
// 参数: interface_num - 接口编号
void UsbDevice::releaseInterface(int interface_num)
{
    // 调用libusb_release_interface释放接口
    libusb_release_interface(handle_, interface_num);
    // 从已声明列表中移除
    claimed_interfaces_.erase(
        std::remove(claimed_interfaces_.begin(), claimed_interfaces_.end(), interface_num),
        claimed_interfaces_.end()
    );
}

// 控制传输（端点0，用于设备配置和状态查询）
// 参数: bmRequestType - 请求类型, bRequest - 请求码
//      wValue - 值, wIndex - 索引
//      data - 数据缓冲区, wLength - 数据长度
//      timeout - 超时时间(ms)
// 返回: 实际传输字节数或错误码
int UsbDevice::controlTransfer(uint8_t bmRequestType, uint8_t bRequest,
                                uint16_t wValue, uint16_t wIndex,
                                unsigned char* data, uint16_t wLength,
                                unsigned int timeout)
{
    return libusb_control_transfer(handle_, bmRequestType, bRequest,
                                    wValue, wIndex, data, wLength, timeout);
}

// 批量传输（用于大量数据传输）
// 参数: endpoint - 端点地址, data - 数据缓冲区
//      length - 数据长度, transferred - 实际传输长度指针
//      timeout - 超时时间(ms)
// 返回: LIBUSB_SUCCESS成功或错误码
int UsbDevice::bulkTransfer(uint8_t endpoint, unsigned char* data,
                             int length, int* transferred,
                             unsigned int timeout)
{
    return libusb_bulk_transfer(handle_, endpoint, data, length,
                                 transferred, timeout);
}

// 中断传输（用于HID设备等）
// 参数: endpoint - 端点地址, data - 数据缓冲区
//      length - 数据长度, transferred - 实际传输长度指针
//      timeout - 超时时间(ms)
// 返回: LIBUSB_SUCCESS成功或错误码
int UsbDevice::interruptTransfer(uint8_t endpoint, unsigned char* data,
                                  int length, int* transferred,
                                  unsigned int timeout)
{
    return libusb_interrupt_transfer(handle_, endpoint, data, length,
                                      transferred, timeout);
}

// 获取字符串描述符（ASCII格式）
// 参数: desc_index - 描述符索引, str - 输出字符串
// 返回: 字符串长度或错误码
int UsbDevice::getStringDescriptor(uint8_t desc_index, std::string& str)
{
    unsigned char buf[256];  // 缓冲区
    // 调用libusb_get_string_descriptor_ascii获取字符串
    int ret = libusb_get_string_descriptor_ascii(handle_, desc_index, buf, sizeof(buf));
    if (ret < 0) {
        // 获取失败，打印错误
        std::cerr << "libusb get string descriptor failed: "
                  << libusb_error_name(ret) << std::endl;
        return ret;
    }
    // 转换为std::string
    str = std::string(reinterpret_cast<char*>(buf));
    return ret;
}

// 获取设备描述符
// 参数: desc - 设备描述符结构
// 返回: LIBUSB_SUCCESS成功或错误码
int UsbDevice::getDeviceDescriptor(libusb_device_descriptor& desc)
{
    // 获取设备指针
    libusb_device* dev = libusb_get_device(handle_);
    // 调用libusb_get_device_descriptor获取描述符
    return libusb_get_device_descriptor(dev, &desc);
}

// 获取配置描述符
// 参数: config_index - 配置索引, config - 配置描述符指针
// 返回: LIBUSB_SUCCESS成功或错误码
int UsbDevice::getConfigDescriptor(uint8_t config_index, libusb_config_descriptor*& config)
{
    return libusb_get_config_descriptor(libusb_get_device(handle_), config_index, &config);
}

// 释放配置描述符内存
// 参数: config - 配置描述符指针
void UsbDevice::freeConfigDescriptor(libusb_config_descriptor* config)
{
    libusb_free_config_descriptor(config);
}

// 检查设备是否打开
// 返回: true已打开, false未打开
bool UsbDevice::isOpen() const
{
    return handle_ != nullptr;
}

// 获取设备句柄
// 返回: libusb_device_handle指针
libusb_device_handle* UsbDevice::getHandle() const
{
    return handle_;
}
