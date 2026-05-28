// ============================================================================
// gt64he_device.cpp - GT-64HE 键盘设备（实现文件）
//
// 功能说明：
//   实现 GT64HeDevice 的构造、析构及设备特定操作。
//   大部分操作由基类 UsbDeviceBase 提供。
//   添加详细日志记录设备生命周期和操作流程。
// ============================================================================

#include "gt64he_device.hpp"
#include "console.h"

static std::string _dn = "";
// 构造函数
GT64HeDevice::GT64HeDevice(const UsbDeviceInfo& info)
    : UsbDeviceBase(info) // 调用基类构造函数
{
    _dn = info.di.product + ":";
    Console::info("GT64HeDevice") << _dn << " construct: device=" << info.to_string() << std::endl;
    Console::info("GT64HeDevice") << _dn << " construct: VID=0x" << std::hex << VID
                   << " PID=0x" << PID << std::dec
                   << " EP_SIZE=" << EP_SIZE << std::endl;
}

// 析构函数
GT64HeDevice::~GT64HeDevice() {
    Console::info("GT64HeDevice") << _dn << " destruct: closing device..." << std::endl;
    close();
    Console::info("GT64HeDevice") << _dn << " destruct: device destroyed" << std::endl;
}
