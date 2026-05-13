#include "hid/HidInterface.h"
#include <iostream>

// 构造函数：初始化成员变量
// 参数: device - USB设备引用, interface_num - 接口编号
HidInterface::HidInterface(UsbDevice& device, int interface_num)
    : device_(device)           // 保存设备引用
    , interface_num_(interface_num)  // 保存接口编号
    , in_endpoint_(0)           // 输入端点初始化为0
    , out_endpoint_(0)          // 输出端点初始化为0
    , in_max_packet_(0)         // 输入包大小初始化为0
    , out_max_packet_(0)        // 输出包大小初始化为0
    , initialized_(false)       // 初始化标志设为false
{
}

// 析构函数：如果已初始化则清理
HidInterface::~HidInterface()
{
    if (initialized_) {
        cleanup();
    }
}

// 初始化HID接口
// 返回: true成功, false失败
bool HidInterface::init()
{
    // 声明接口
    if (!device_.claimInterface(interface_num_)) {
        return false;
    }

    // 查找端点
    if (!findEndpoints()) {
        std::cerr << "Failed to find HID endpoints" << std::endl;
        return false;
    }

    initialized_ = true;  // 设置初始化标志
    return true;
}

// 清理HID接口（释放接口）
void HidInterface::cleanup()
{
    if (initialized_) {
        device_.releaseInterface(interface_num_);  // 释放接口
        initialized_ = false;  // 重置标志
    }
}

// 查找HID端点（遍历配置描述符找到HID类接口的端点）
// 返回: 0成功, -1失败
int HidInterface::findEndpoints()
{
    libusb_config_descriptor* config = nullptr;
    // 获取配置描述符
    int ret = device_.getConfigDescriptor(0, config);
    if (ret != LIBUSB_SUCCESS) {
        return ret;
    }

    // 遍历所有接口
    for (uint8_t i = 0; i < config->bNumInterfaces; ++i) {
        const libusb_interface* iface = &config->interface[i];
        // 遍历所有备用设置
        for (int j = 0; j < iface->num_altsetting; ++j) {
            const libusb_interface_descriptor* if_desc = &iface->altsetting[j];

            // 检查是否是目标接口且是HID类
            if (if_desc->bInterfaceNumber == static_cast<uint8_t>(interface_num_) &&
                if_desc->bInterfaceClass == LIBUSB_CLASS_HID) {

                // 遍历端点，区分输入输出
                for (uint8_t ep = 0; ep < if_desc->bNumEndpoints; ++ep) {
                    const libusb_endpoint_descriptor* ep_desc = &if_desc->endpoint[ep];
                    uint8_t ep_addr = ep_desc->bEndpointAddress;

                    // 根据方向位判断输入/输出端点
                    if (ep_addr & LIBUSB_ENDPOINT_IN) {
                        in_endpoint_ = ep_addr;           // 保存输入端点地址
                        in_max_packet_ = ep_desc->wMaxPacketSize;  // 保存包大小
                    } else {
                        out_endpoint_ = ep_addr;          // 保存输出端点地址
                        out_max_packet_ = ep_desc->wMaxPacketSize;  // 保存包大小
                    }
                }

                device_.freeConfigDescriptor(config);  // 释放配置描述符
                return 0;  // 成功
            }
        }
    }

    device_.freeConfigDescriptor(config);  // 释放配置描述符
    return -1;  // 未找到HID端点
}

// 发送HID报告（使用中断传输）
// 参数: data - 数据向量, timeout - 超时时间(ms)
// 返回: 实际发送字节数或错误码
int HidInterface::sendReport(const std::vector<uint8_t>& data, unsigned int timeout)
{
    // 检查是否已初始化且有输出端点
    if (!initialized_ || out_endpoint_ == 0) {
        return -1;
    }

    int transferred = 0;
    std::vector<uint8_t> buf = data;  // 复制数据

    // 调用中断传输
    int ret = device_.interruptTransfer(out_endpoint_, buf.data(),
                                         static_cast<int>(buf.size()),
                                         &transferred, timeout);

    if (ret != LIBUSB_SUCCESS) {
        std::cerr << "HID send failed: " << libusb_error_name(ret) << std::endl;
        return ret;
    }

    return transferred;  // 返回实际发送字节数
}

// 接收HID报告（使用中断传输）
// 参数: data - 输出数据向量, timeout - 超时时间(ms)
// 返回: 实际接收字节数或错误码
int HidInterface::receiveReport(std::vector<uint8_t>& data, unsigned int timeout)
{
    // 检查是否已初始化且有输入端点
    if (!initialized_ || in_endpoint_ == 0) {
        return -1;
    }

    // 创建接收缓冲区
    std::vector<uint8_t> buf(in_max_packet_);
    int transferred = 0;

    // 调用中断传输
    int ret = device_.interruptTransfer(in_endpoint_, buf.data(),
                                         static_cast<int>(buf.size()),
                                         &transferred, timeout);

    if (ret != LIBUSB_SUCCESS) {
        std::cerr << "HID receive failed: " << libusb_error_name(ret) << std::endl;
        return ret;
    }

    // 将接收到的数据复制到输出向量
    data.assign(buf.begin(), buf.begin() + transferred);
    return transferred;  // 返回实际接收字节数
}

// 发送控制报告（使用控制传输）
// 参数: request_type - 请求类型, request - 请求码
//      value - 值, index - 索引
//      data - 数据向量, timeout - 超时时间(ms)
// 返回: 实际传输字节数或错误码
int HidInterface::sendControlReport(uint8_t request_type, uint8_t request,
                                     uint16_t value, uint16_t index,
                                     const std::vector<uint8_t>& data,
                                     unsigned int timeout)
{
    // 调用控制传输
    int ret = device_.controlTransfer(request_type, request, value, index,
                                       const_cast<unsigned char*>(data.data()),
                                       static_cast<uint16_t>(data.size()),
                                       timeout);
    if (ret < 0) {
        std::cerr << "HID control transfer failed: " << libusb_error_name(ret) << std::endl;
        return ret;
    }
    return ret;  // 返回实际传输字节数
}

// 获取输入端点地址
// 返回: 输入端点地址
uint8_t HidInterface::getInEndpoint() const
{
    return in_endpoint_;
}

// 获取输出端点地址
// 返回: 输出端点地址
uint8_t HidInterface::getOutEndpoint() const
{
    return out_endpoint_;
}
