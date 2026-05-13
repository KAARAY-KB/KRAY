#ifndef USB_CONTEXT_H
#define USB_CONTEXT_H

#include <libusb.h>
#include <vector>
#include <memory>
#include <string>

// USB上下文管理类
// 负责libusb库的初始化、退出和设备枚举
class UsbContext {
public:
    // 构造函数：初始化成员变量
    UsbContext();
    
    // 析构函数：确保资源释放
    ~UsbContext();

    // 初始化libusb上下文
    // 返回: true成功, false失败
    bool init();
    
    // 退出libusb上下文，释放资源
    void exit();

    // 枚举所有USB设备
    // 返回: 设备列表
    std::vector<libusb_device*> enumerateDevices();
    
    // 获取libusb上下文指针
    // 返回: libusb_context指针
    libusb_context* getContext() const;

private:
    libusb_context* ctx_;        // libusb上下文指针
    bool initialized_;           // 初始化状态标志
};

#endif
