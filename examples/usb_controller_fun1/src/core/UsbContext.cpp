#include "core/UsbContext.h"
#include <iostream>

// 构造函数：初始化成员变量
UsbContext::UsbContext()
    : ctx_(nullptr)        // 上下文指针初始化为空
    , initialized_(false)  // 初始化标志设为false
{
}

// 析构函数：确保资源正确释放
UsbContext::~UsbContext()
{
    // 如果已初始化，调用exit释放资源
    if (initialized_) {
        exit();
    }
}

// 初始化libusb上下文
// 返回: true成功, false失败
bool UsbContext::init()
{
    // 调用libusb_init初始化库
    int ret = libusb_init(&ctx_);
    if (ret != LIBUSB_SUCCESS) {
        // 初始化失败，打印错误信息
        std::cerr << "libusb init failed: " << libusb_error_name(ret) << std::endl;
        return false;
    }
    // 初始化成功，设置标志
    initialized_ = true;
    return true;
}

// 退出libusb上下文，释放所有资源
void UsbContext::exit()
{
    // 如果上下文存在，调用libusb_exit释放
    if (ctx_) {
        libusb_exit(ctx_);
        ctx_ = nullptr;           // 清空指针
        initialized_ = false;     // 重置标志
    }
}

// 枚举所有USB设备
// 返回: 设备指针列表
std::vector<libusb_device*> UsbContext::enumerateDevices()
{
    libusb_device** devs = nullptr;  // 设备列表指针
    std::vector<libusb_device*> result;  // 结果向量

    // 检查是否已初始化
    if (!initialized_) {
        return result;
    }

    // 获取设备列表
    ssize_t count = libusb_get_device_list(ctx_, &devs);
    if (count < 0) {
        // 获取失败，打印错误
        std::cerr << "libusb get device list failed: " << libusb_error_name((int)count) << std::endl;
        return result;
    }

    // 遍历设备列表，添加到结果中
    for (ssize_t i = 0; i < count; ++i) {
        result.push_back(devs[i]);
    }

    // 释放设备列表（不释放设备对象）
    libusb_free_device_list(devs, 0);
    return result;
}

// 获取libusb上下文指针
// 返回: libusb_context指针
libusb_context* UsbContext::getContext() const
{
    return ctx_;
}
