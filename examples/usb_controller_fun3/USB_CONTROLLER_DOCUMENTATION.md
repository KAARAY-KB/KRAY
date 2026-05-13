# USB 控制器库 (USB Controller Library) 详细说明文档

> **版本**: v3.0 — Unified API Edition  
> **语言**: C++17  
> **依赖**: libusb-1.0  
> **平台**: Linux / Windows (MSVC / MinGW)

---

## 目录

1. [项目概述](#1-项目概述)
2. [项目结构](#2-项目结构)
3. [架构设计](#3-架构设计)
4. [模块详解](#4-模块详解)
   - [4.1 usb_core — USB 核心模块](#41-usb_core--usb-核心模块)
   - [4.2 usb_transfer — USB 传输模块](#42-usb_transfer--usb-传输模块)
   - [4.3 usb_controller — 统一接口模块](#43-usb_controller--统一接口模块)
   - [4.4 main.cpp — 演示程序](#44-maincpp--演示程序)
5. [数据流与调用链](#5-数据流与调用链)
6. [API 参考](#6-api-参考)
7. [构建说明](#7-构建说明)
8. [使用示例](#8-使用示例)

---

## 1. 项目概述

本项目是一个基于 **libusb** 的 C++ USB 设备控制库，提供统一的 API 接口用于：

- **USB 设备枚举与查询**：扫描系统所有 USB 设备，获取设备描述符、配置描述符、接口描述符、端点描述符
- **HID 设备操作**：打开/关闭 HID 设备，读写输入/输出报告，获取/发送特性报告
- **同步数据传输**：支持控制传输、批量传输、中断传输
- **异步数据传输**：支持单次异步读写、连续异步读取（自动循环）

### 设计目标

| 目标 | 实现方式 |
|------|----------|
| 易用性 | 外观模式（Facade），`UsbController` 统一接口 |
| 安全性 | RAII 资源管理，智能指针，禁止拷贝 |
| 可移植性 | 跨平台 CMake 构建，Windows/Linux 双平台支持 |
| 高性能 | 异步传输引擎，独立事件处理线程 |

---

## 2. 项目结构

```
examples/usb_controller_fun3/
├── CMakeLists.txt                    # 根构建文件（可执行文件 + libusb 配置）
├── main.cpp                          # 交互式演示程序（17 个演示功能）
├── library/libusb/                   # libusb 预编译库（Windows）
│   ├── include/                      # libusb 头文件
│   ├── VS2022/                       # MSVC 预编译库
│   └── MinGW64/                      # MinGW 预编译库
└── src/
    ├── CMakeLists.txt                # 静态库构建文件
    ├── usb_controller.hpp            # USB 控制器统一接口（头文件）
    ├── usb_controller.cpp            # USB 控制器统一接口（实现文件）
    ├── usb_core/                     # USB 核心模块
    │   ├── usb_context.hpp           # USB 上下文管理（头文件）
    │   ├── usb_context.cpp           # USB 上下文管理（实现文件）
    │   ├── usb_device.hpp            # USB 设备信息封装（头文件）
    │   ├── usb_device.cpp            # USB 设备信息封装（实现文件）
    │   ├── usb_device_manager.hpp    # USB 设备管理器（头文件）
    │   └── usb_device_manager.cpp    # USB 设备管理器（实现文件）
    └── usb_transfer/                 # USB 传输模块
        ├── usb_transfer.hpp          # 同步传输（头文件）
        ├── usb_transfer.cpp          # 同步传输（实现文件）
        ├── hid_device.hpp            # HID 设备操作（头文件）
        ├── hid_device.cpp            # HID 设备操作（实现文件）
        ├── async_transfer.hpp        # 异步传输（头文件）
        └── async_transfer.cpp        # 异步传输（实现文件）
```

---

## 3. 架构设计

### 3.1 分层架构

```
┌─────────────────────────────────────────────────┐
│                  main.cpp                        │  ← 应用层
│            (交互式演示程序)                        │
├─────────────────────────────────────────────────┤
│              UsbController                       │  ← 外观层
│           (统一对外接口)                          │
├─────────────────────┬───────────────────────────┤
│     usb_core/       │     usb_transfer/          │  ← 业务层
│  ┌───────────────┐  │  ┌──────────────────────┐ │
│  │ UsbContext    │  │  │ SyncTransfer         │ │
│  │ UsbDevice     │  │  │ HidDevice            │ │
│  │ UsbDeviceMgr  │  │  │ AsyncTransferEngine  │ │
│  └───────────────┘  │  │ AsyncHidTransfer     │ │
│                     │  └──────────────────────┘ │
├─────────────────────┴───────────────────────────┤
│                  libusb                          │  ← 驱动层
│         (跨平台 USB 底层库)                       │
└─────────────────────────────────────────────────┘
```

### 3.2 设计模式

| 模式 | 应用位置 | 说明 |
|------|----------|------|
| **外观模式 (Facade)** | `UsbController` | 封装底层复杂操作，提供简洁统一接口 |
| **RAII** | 所有资源管理类 | 构造时获取资源，析构时自动释放 |
| **单例引用** | `UsbContext` | 全局唯一的 libusb 上下文 |
| **观察者模式** | `AsyncCallback` | 异步传输完成通知 |

### 3.3 命名空间

```
usb_ctrl                          # 顶层命名空间
├── usb_ctrl::core                # USB 核心模块
│   ├── UsbException              # USB 异常类
│   ├── UsbContext                # USB 上下文
│   ├── UsbDevice                 # USB 设备
│   ├── UsbDeviceManager          # 设备管理器
│   ├── DeviceInfo                # 设备信息结构体
│   ├── ConfigInfo                # 配置信息结构体
│   ├── InterfaceInfo             # 接口信息结构体
│   └── EndpointInfo              # 端点信息结构体
└── usb_ctrl::transfer            # USB 传输模块
    ├── TransferResult            # 传输结果结构体
    ├── SyncTransfer              # 同步传输类
    ├── HidDevice                 # HID 设备类
    ├── AsyncTransferEngine       # 异步传输引擎
    ├── AsyncHidTransfer          # HID 异步传输
    ├── AsyncTransferType         # 异步传输类型枚举
    ├── AsyncTransferRequest      # 异步传输请求
    └── AsyncCallback             # 异步回调类型
```

---

## 4. 模块详解

### 4.1 usb_core — USB 核心模块

#### 4.1.1 UsbContext — USB 上下文管理

**文件**: `usb_context.hpp` / `usb_context.cpp`

对 `libusb_context` 的 RAII 封装，负责 libusb 库的初始化与销毁。

```
生命周期:
  构造 → libusb_init()     // 初始化 libusb 库
  析构 → libusb_exit()     // 清理 libusb 资源
```

**关键方法**:
| 方法 | 说明 |
|------|------|
| `handle()` | 获取底层 `libusb_context*` 指针 |
| `set_debug_level(int)` | 设置调试输出级别 (0-4) |

**设计要点**:
- 禁止拷贝和移动，确保上下文唯一性
- 初始化失败时抛出 `UsbException` 异常

---

#### 4.1.2 UsbDevice — USB 设备封装

**文件**: `usb_device.hpp` / `usb_device.cpp`

对 `libusb_device` 的 C++ 面向对象封装，管理设备引用计数。

**核心数据结构层级**:
```
DeviceInfo                          # 设备完整信息
├── vendor_id, product_id           # VID/PID
├── manufacturer, product, serial   # 字符串描述符
├── device_speed, speed_str         # 速度信息
├── bus_number, device_address      # 总线信息
└── configs[]                       # 配置数组
    └── ConfigInfo                  # 配置信息
        ├── value, max_power        # 配置值、功耗
        ├── self_powered()          # 是否自供电
        ├── remote_wakeup()         # 是否支持远程唤醒
        └── interfaces[]            # 接口数组
            └── InterfaceInfo       # 接口信息
                ├── number, bclass  # 接口编号、类代码
                ├── class_str       # 类名称（如 "HID"）
                └── endpoints[]     # 端点数组
                    └── EndpointInfo # 端点信息
                        ├── address # 端点地址
                        ├── max_packet_size # 最大包大小
                        ├── is_in() / is_out() # 方向判断
                        └── type_str() # 传输类型
```

**关键方法**:
| 方法 | 说明 |
|------|------|
| `get_info()` | 获取完整设备描述符信息（核心方法） |
| `summary()` | 单行摘要（VID/PID/速度/产品名） |
| `detail()` | 多行详细信息（所有描述符） |
| `descriptor_tree()` | 树形结构（层级缩进） |
| `has_hid_interface()` | 检查是否有 HID 接口 |
| `find_hid_interface()` | 查找 HID 接口的端点信息 |
| `vid()` / `pid()` | 快速获取 VID/PID |

**`get_info()` 方法流程**:
```
1. libusb_get_device_descriptor()    → 设备描述符
2. libusb_get_device_speed()         → 设备速度
3. libusb_get_bus_number()           → 总线编号
4. libusb_get_device_address()       → 设备地址
5. get_string_descriptor()           → 字符串描述符（制造商/产品/序列号）
6. libusb_get_active_config_descriptor() → 活动配置描述符
7. 遍历接口 → 遍历备用设置 → 遍历端点
```

---

#### 4.1.3 UsbDeviceManager — 设备管理器

**文件**: `usb_device_manager.hpp` / `usb_device_manager.cpp`

管理系统所有 USB 设备的列表，提供枚举、查询和格式化输出。

**关键方法**:
| 方法 | 说明 |
|------|------|
| `refresh()` | 重新枚举所有 USB 设备 |
| `count()` | 获取设备数量 |
| `list_summary()` | 设备简要列表 |
| `list_detail()` | 设备详细列表 |
| `list_tree()` | 设备树形结构 |
| `list_hid_summary()` | HID 设备列表 |
| `find_by_vid_pid()` | 按 VID/PID 查找 |
| `find_hid_devices()` | 查找所有 HID 设备 |
| `find_by_class()` | 按类代码查找 |

---

### 4.2 usb_transfer — USB 传输模块

#### 4.2.1 TransferResult — 传输结果结构体

```cpp
struct TransferResult {
    bool success;              // 传输是否成功
    int bytes_transferred;     // 实际传输字节数
    int error_code;            // libusb 错误码
    std::string error_message; // 错误描述
    std::vector<uint8_t> data; // 传输数据
};
```

#### 4.2.2 SyncTransfer — 同步传输类

**文件**: `usb_transfer.hpp` / `usb_transfer.cpp`

封装 libusb 的同步传输 API，所有方法阻塞调用线程直到完成。

**支持的传输类型**:
| 方法 | 传输类型 | 方向 | 用途 |
|------|----------|------|------|
| `bulk_read()` | 批量 | IN | 大容量数据读取（U盘） |
| `bulk_write()` | 批量 | OUT | 大容量数据写入 |
| `interrupt_read()` | 中断 | IN | HID 输入报告（键盘/鼠标） |
| `interrupt_write()` | 中断 | OUT | HID 输出报告（LED 控制） |
| `control_transfer()` | 控制 | OUT | 发送控制请求（Set_Report） |
| `control_read()` | 控制 | IN | 读取控制响应（Get_Report） |

**工具函数**:
| 函数 | 说明 |
|------|------|
| `bytes_to_hex()` | 字节数组 → 十六进制字符串 |
| `bytes_to_ascii()` | 字节数组 → ASCII 字符串 |
| `format_transfer_result()` | 格式化传输结果 |

---

#### 4.2.3 HidDevice — HID 设备操作类

**文件**: `hid_device.hpp` / `hid_device.cpp`

封装 HID 设备的打开、关闭和数据传输操作。

**HID 设备打开流程**:
```
1. has_hid_interface()           → 检测 HID 接口
2. find_hid_interface()          → 查找 IN/OUT 端点
3. libusb_open()                 → 打开设备
4. libusb_kernel_driver_active() → 检测内核驱动
5. libusb_detach_kernel_driver() → 分离内核驱动
6. libusb_claim_interface()      → 声明接口
7. 创建 SyncTransfer             → 准备数据传输
```

**关闭流程（逆序）**:
```
1. 释放 SyncTransfer
2. libusb_release_interface()    → 释放接口
3. libusb_attach_kernel_driver() → 重新附加内核驱动
4. libusb_close()                → 关闭设备
```

**关键方法**:
| 方法 | 说明 |
|------|------|
| `open()` | 打开 HID 设备 |
| `close()` | 关闭 HID 设备 |
| `read_report()` | 读取输入报告（中断 IN） |
| `write_report()` | 写入输出报告（中断 OUT） |
| `get_feature_report()` | 获取特性报告（控制传输 GET_REPORT） |
| `send_feature_report()` | 发送特性报告（控制传输 SET_REPORT） |

---

#### 4.2.4 AsyncTransferEngine — 异步传输引擎

**文件**: `async_transfer.hpp` / `async_transfer.cpp`

管理 libusb 异步事件处理线程。

**线程模型**:
```
主线程                          事件线程
  │                               │
  ├─ start() ─────────────────→ 创建线程
  │                               │
  ├─ submit() ────────────────→ libusb_submit_transfer()
  │                               │
  │                            libusb_handle_events_timeout()
  │                               │
  │                            _transfer_cb() ← libusb 回调
  │                               │
  │                            用户 callback()
  │                               │
  ├─ stop() ──────────────────→ _running = false
  │                            join() 等待线程退出
```

**关键方法**:
| 方法 | 说明 |
|------|------|
| `start()` | 启动事件处理线程 |
| `stop()` | 停止事件处理线程 |
| `submit()` | 提交异步传输请求 |
| `is_running()` | 检查引擎是否运行 |
| `pending_count()` | 获取待处理传输数量 |

---

#### 4.2.5 AsyncHidTransfer — HID 异步传输类

**文件**: `async_transfer.hpp` / `async_transfer.cpp`

封装 HID 设备的异步数据传输操作。

**关键方法**:
| 方法 | 说明 |
|------|------|
| `read_async()` | 单次异步读取 |
| `write_async()` | 单次异步写入 |
| `read_continuous()` | 连续异步读取（自动循环） |
| `stop_continuous()` | 停止连续读取 |

**连续读取机制**:
```
read_continuous() → read_async() → 回调 → 检查 _continuous → read_continuous() → ...
                                                              ↓ (false)
                                                           停止循环
```

---

### 4.3 usb_controller — 统一接口模块

**文件**: `usb_controller.hpp` / `usb_controller.cpp`

作为外观类，整合所有子模块，提供统一的对外 API。

**内部组件**:
```
UsbController
├── _ctx          : unique_ptr<UsbContext>           # USB 上下文
├── _manager      : unique_ptr<UsbDeviceManager>     # 设备管理器
├── _hid_device   : unique_ptr<HidDevice>            # HID 设备
├── _sync_transfer: unique_ptr<SyncTransfer>         # 同步传输
├── _async_engine : unique_ptr<AsyncTransferEngine>  # 异步引擎
└── _async_hid    : unique_ptr<AsyncHidTransfer>     # HID 异步传输
```

**API 分类**:
| 类别 | 方法 | 说明 |
|------|------|------|
| 基础设置 | `set_debug_level()`, `refresh_devices()` | 调试级别、刷新设备 |
| 设备查询 | `list_devices()`, `list_devices_detail()`, `list_devices_tree()`, `list_hid_devices()`, `device_count()`, `device_detail()`, `find_by_vid_pid()`, `find_hid_devices()` | 设备列表和查找 |
| HID 操作 | `open_hid_device()`, `open_hid_device_by_vid_pid()`, `close_hid_device()`, `is_hid_open()`, `hid_device_info()`, `hid_read()`, `hid_write()`, `hid_get_feature_report()`, `hid_send_feature_report()` | HID 设备读写 |
| 同步传输 | `bulk_read()`, `bulk_write()`, `interrupt_read()`, `interrupt_write()` | 原始端点传输 |
| 异步传输 | `async_start()`, `async_stop()`, `hid_read_async()`, `hid_write_async()`, `hid_read_continuous()`, `hid_stop_continuous()`, `async_pending_count()` | 异步传输控制 |
| 底层访问 | `hid_device()`, `sync_transfer()`, `async_engine()` | 高级用法 |

---

### 4.4 main.cpp — 演示程序

**文件**: `main.cpp`

交互式命令行演示程序，提供 17 个演示功能：

| 编号 | 功能 | 说明 |
|------|------|------|
| 1 | 列出所有 USB 设备 | 设备简要列表 |
| 2 | 设备详细信息 | 前 3 个设备的完整描述符 |
| 3 | 树形视图 | 层级缩进格式 |
| 4 | HID 设备列表 | 仅显示 HID 类设备 |
| 5 | 按索引打开 HID | 交互式选择设备 |
| 6 | 按 VID:PID 打开 HID | 十六进制输入 VID/PID |
| 7 | 设备详情 | 指定索引的完整信息 |
| 8 | 同步 HID 读取 | 中断 IN 传输 |
| 9 | 同步 HID 写入 | 中断 OUT 传输 |
| 10 | 获取特性报告 | GET_REPORT 控制传输 |
| 11 | 发送特性报告 | SET_REPORT 控制传输 |
| 12 | 单次异步读取 | 异步中断 IN |
| 13 | 连续异步读取 | 5 秒自动循环读取 |
| 14 | 原始批量读取 | 指定端点批量 IN |
| 15 | 原始批量写入 | 指定端点批量 OUT |
| 16 | 按 VID:PID 查找 | 设备搜索 |
| 17 | 自动运行所有演示 | 无人值守演示模式 |

---

## 5. 数据流与调用链

### 5.1 设备枚举流程

```
main()
  └→ UsbController 构造
       └→ UsbContext 构造 → libusb_init()
       └→ UsbDeviceManager 构造 → refresh()
            └→ libusb_get_device_list()
            └→ _raw_to_list() → 创建 UsbDevice 对象列表
            └→ libusb_free_device_list()
```

### 5.2 HID 设备打开与读取流程

```
main() → demo_hid_read()
  └→ ctrl.open_hid_device(index)
       └→ HidDevice::open()
            └→ has_hid_interface()
            └→ find_hid_interface()
            └→ libusb_open()
            └→ libusb_detach_kernel_driver()
            └→ libusb_claim_interface()
            └→ 创建 SyncTransfer
  └→ ctrl.hid_read(64)
       └→ HidDevice::read_report()
            └→ SyncTransfer::interrupt_read()
                 └→ libusb_interrupt_transfer()
```

### 5.3 异步传输流程

```
main() → demo_async_read()
  └→ ctrl.async_start()
       └→ AsyncTransferEngine::start()
            └→ 创建事件线程 → _event_loop()
  └→ ctrl.hid_read_async(64, callback)
       └→ AsyncHidTransfer::read_async()
            └→ AsyncTransferEngine::submit()
                 └→ libusb_alloc_transfer()
                 └→ libusb_fill_interrupt_transfer()
                 └→ libusb_submit_transfer()
  └→ [事件线程] libusb_handle_events_timeout()
       └→ _transfer_cb() ← libusb 回调
            └→ 解析 TransferResult
            └→ 调用用户 callback()
            └→ libusb_free_transfer()
```

---

## 6. API 参考

### 6.1 快速开始

```cpp
#include "usb_controller.hpp"

int main() {
    // 1. 创建控制器（自动初始化 libusb 并枚举设备）
    usb_ctrl::UsbController ctrl;

    // 2. 列出所有设备
    std::cout << ctrl.list_devices();

    // 3. 打开第一个 HID 设备
    if (ctrl.open_hid_device(0)) {
        // 4. 读取 HID 输入报告
        auto result = ctrl.hid_read(64, 1000);
        if (result.success) {
            std::cout << "Read " << result.bytes_transferred << " bytes\n";
        }

        // 5. 写入 HID 输出报告
        std::vector<uint8_t> data = {0x00, 0x01, 0x02};
        ctrl.hid_write(data, 1000);

        // 6. 关闭设备
        ctrl.close_hid_device();
    }

    return 0;
}
```

### 6.2 异步传输示例

```cpp
usb_ctrl::UsbController ctrl;
ctrl.open_hid_device(0);

// 启动异步引擎
ctrl.async_start();

// 单次异步读取
ctrl.hid_read_async(64, [](const TransferResult& r) {
    if (r.success) {
        std::cout << "Async read: " << r.bytes_transferred << " bytes\n";
    }
}, 2000);

// 连续异步读取（自动循环）
std::atomic<int> count{0};
ctrl.hid_read_continuous(64, [&count](const TransferResult& r) {
    if (r.success && r.bytes_transferred > 0) {
        ++count;
        std::cout << "#" << count << ": " << r.bytes_transferred << " bytes\n";
    }
}, 500);

// 5 秒后停止
std::this_thread::sleep_for(std::chrono::seconds(5));
ctrl.hid_stop_continuous();
ctrl.async_stop();
```

### 6.3 设备查找示例

```cpp
usb_ctrl::UsbController ctrl;

// 按 VID:PID 查找
auto devs = ctrl.find_by_vid_pid(0x046D, 0xC077);
for (auto& dev : devs) {
    std::cout << dev.summary() << "\n";
}

// 查找所有 HID 设备
auto hid_devs = ctrl.find_hid_devices();
std::cout << "Found " << hid_devs.size() << " HID devices\n";
```

---

## 7. 构建说明

### 7.1 Linux

```bash
# 安装依赖
sudo apt install libusb-1.0-0-dev cmake build-essential

# 构建
cd examples/usb_controller_fun3
mkdir build && cd build
cmake ..
make -j$(nproc)

# 运行（需要 root 权限或配置 udev 规则）
sudo ./bin/ex_usb_controller_fun3
```

### 7.2 Windows (MSVC)

```bash
# 使用 Visual Studio 2022
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release

# 运行（需要安装 libusb 驱动，如 Zadig）
bin\Release\ex_usb_controller_fun3.exe
```

### 7.3 udev 规则（Linux，避免 sudo）

创建 `/etc/udev/rules.d/99-usb.rules`：
```
SUBSYSTEM=="usb", ATTR{idVendor}=="046d", ATTR{idProduct}=="c077", MODE="0666"
```
然后执行 `sudo udevadm control --reload-rules && sudo udevadm trigger`。

---

## 8. 使用示例

### 8.1 读取键盘输入

```cpp
usb_ctrl::UsbController ctrl;
ctrl.open_hid_device_by_vid_pid(0x046D, 0xC534); // Logitech 键盘

while (true) {
    auto result = ctrl.hid_read(8, 1000);
    if (result.success && result.bytes_transferred > 0) {
        // 解析 HID 键盘报告
        // data[0] = 修饰键, data[2] = 按键码
        std::cout << transfer::bytes_to_hex(result.data, 8) << "\n";
    }
}
```

### 8.2 控制键盘 LED

```cpp
usb_ctrl::UsbController ctrl;
ctrl.open_hid_device_by_vid_pid(0x046D, 0xC534);

// 输出报告格式: [报告ID, LED状态]
// LED 状态: bit0=NumLock, bit1=CapsLock, bit2=ScrollLock
std::vector<uint8_t> led_on = {0x00, 0x02};  // 打开 CapsLock
ctrl.hid_write(led_on, 1000);
```

### 8.3 获取设备描述符树

```cpp
usb_ctrl::UsbController ctrl;
std::cout << ctrl.list_devices_tree();
```

输出示例：
```
USB Device Tree (3 devices):
========================================
[0] VID:0x046D PID:0xC077 Speed:Full (12Mbps) USB:2.0 [USB Optical Mouse]
  ├── Configuration 1: 100mA
  │   └── Interface 0: HID (3 endpoints)
  │       ├── Endpoint 0x81: Interrupt IN (max 8B)
  │       └── Endpoint 0x01: Interrupt OUT (max 8B)
```

---

> **文档生成日期**: 2026-05-14  
> **项目路径**: `/home/Y700Gen4/workspace/qt/KRAY/examples/usb_controller_fun3`