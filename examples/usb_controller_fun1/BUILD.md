# USB Controller - 构建说明

## 项目结构
```
usb_controller/
├── CMakeLists.txt              # CMake构建配置
├── include/                    # 头文件目录
│   ├── core/                   # 核心USB封装
│   │   ├── UsbContext.h        # USB上下文管理
│   │   └── UsbDevice.h         # USB设备操作
│   ├── device/                 # 设备信息解析
│   │   └── UsbDeviceInfo.h     # 设备信息解析与打印
│   ├── hid/                    # HID通信
│   │   └── HidInterface.h      # HID接口操作
│   └── async/                  # 异步传输
│       └── AsyncTransfer.h     # 异步收发管理
├── src/                        # 源文件目录
│   ├── main.cpp                # 主程序入口
│   ├── core/                   # 核心层实现
│   ├── device/                 # 设备信息层实现
│   ├── hid/                    # HID层实现
│   └── async/                  # 异步层实现
└── library/                    # 第三方库
    └── libusb/                 # libusb库（Windows）
```

## 构建方法

### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### Windows (MinGW)
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

### Linux
```bash
mkdir build
cd build
cmake ..
make
```

## 软件架构

### 分层设计
```
┌─────────────────────────────────────────────┐
│            应用层 (main.cpp)                │
│         用户交互、菜单、命令处理              │
├─────────────────────────────────────────────┤
│          异步传输层 (AsyncTransfer)          │
│       线程异步收发、事件循环管理              │
├─────────────────────────────────────────────┤
│          HID通信层 (HidInterface)            │
│       HID端点查找、报告收发                   │
├─────────────────────────────────────────────┤
│        设备信息层 (UsbDeviceInfo)            │
│       设备描述符解析、信息打印                │
├─────────────────────────────────────────────┤
│          核心封装层 (UsbContext/Device)      │
│       libusb API封装、资源管理                │
├─────────────────────────────────────────────┤
│              libusb库                        │
└─────────────────────────────────────────────┘
```

### 核心类说明

1. **UsbContext**: USB上下文管理，负责libusb初始化和设备枚举
2. **UsbDevice**: USB设备操作封装，提供打开/关闭、端点传输等接口
3. **UsbDeviceInfo**: 设备信息解析，解析并打印设备描述符信息
4. **HidInterface**: HID接口通信，自动查找端点并支持报告收发
5. **AsyncTransfer**: 异步传输管理，支持多线程异步批量/中断传输

## 功能特性

- 枚举所有USB设备
- 按VID/PID打开指定设备
- 查看设备完整信息（描述符、配置、端点等）
- HID接口数据收发
- 异步批量/中断传输（线程池模式）
- 交互式命令行界面
