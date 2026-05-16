// ============================================================================
// gt64he_device.cpp - GT-64HE 键盘设备（实现文件）
//
// 功能说明：
//   实现 GT64HeDevice 的构造和析构。
//   大部分操作由基类 UsbDeviceBase 提供。
// ============================================================================

#include "gt64he_device.hpp"
#include "console.h"

// 构造函数
GT64HeDevice::GT64HeDevice(const UsbDeviceInfo& info)
    : UsbDeviceBase(info) // 调用基类构造函数
{
    Console::out() << "GT64HeDevice: created for " << info.to_string() << std::endl;
}

// 析构函数
GT64HeDevice::~GT64HeDevice() {
    Console::out() << "GT64HeDevice: destroyed" << std::endl;
}
