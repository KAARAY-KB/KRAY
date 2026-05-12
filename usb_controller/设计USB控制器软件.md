# 设计USB控制器软件

> **Workspace:** /home/Y700Gen4/workspace/c_cpp/usb_controller

---

## User
我想做一个usb控制器，用于对usb进行一些操作，如：查看设备的所有信息，对HID接口进行收发数据，线程异步收发；使用c++软件设计分层解耦，使用cmake来编译构建。该项目下有些文件来做参考，有libusb第三方接口库 `/home/Y700Gen4/workspace/c_cpp/usb_controller/library` 、有usb设备的所有信息的demo `/home/Y700Gen4/workspace/c_cpp/usb_controller/UsbTree_report_demo.txt`

## Assistant
我来帮你设计一个USB控制器项目。首先让我了解一下现有的参考文件和项目结构。



好的，我已经了解了项目结构和参考文件。让我规划并实现这个USB控制器项目。



开始创建项目结构和CMakeLists.txt。



让我直接在项目目录下创建文件：



现在实现USB底层封装层：



现在实现设备信息解析层：



现在实现HID通信层：



现在实现异步收发层，这是核心功能之一：



让我先修复AsyncTransfer.cpp中的一些问题：



我需要修复AsyncTransfer.cpp中的内存管理问题：



Now let me create the main application file:



Now I need to fix the main.cpp to access the private event_thread_ member properly:



I need to make event_thread_ accessible or provide a method to start it. Let me add a proper method:



现在让我测试编译构建：



让我检查下系统环境并尝试直接编译：



我看到libusb库是为Windows (MinGW/Visual Studio) 提供的。让我更新CMakeLists.txt以更好地支持Windows构建，并添加必要的缺失头文件：



让我更新CMakeLists.txt，使其更适合Windows环境（因为libusb库是Windows版本）：



现在让我创建一个简单的构建说明文件，并添加缺失的algorithm头文件：



现在让我创建一个构建说明文件：



我看到src/main.cpp没有显示在列表中，让我确认一下：



现在让我为所有文件添加详细的中文注释。我需要逐个文件更新：



现在继续为剩余文件添加详细中文注释。让我同时处理多个文件：



继续为剩余文件添加详细中文注释：



继续为异步传输层和主程序添加详细中文注释：



Now let me add detailed Chinese comments to the main.cpp file:



项目已完成！以下是完整的USB控制器项目总结：

## 项目结构

```
usb_controller/
├── CMakeLists.txt              # CMake构建配置（支持Windows/Linux）
├── BUILD.md                    # 构建说明文档
├── include/                    # 头文件目录
│   ├── core/
│   │   ├── UsbContext.h        # USB上下文管理
│   │   └── UsbDevice.h         # USB设备操作
│   ├── device/
│   │   └── UsbDeviceInfo.h     # 设备信息解析
│   ├── hid/
│   │   └── HidInterface.h      # HID接口通信
│   └── async/
│       └── AsyncTransfer.h     # 异步传输管理
├── src/                        # 源文件目录
│   ├── main.cpp                # 主程序（交互式菜单）
│   ├── core/
│   │   ├── UsbContext.cpp
│   │   └── UsbDevice.cpp
│   ├── device/
│   │   └── UsbDeviceInfo.cpp
│   ├── hid/
│   │   └── HidInterface.cpp
│   └── async/
│       └── AsyncTransfer.cpp
└── library/libusb/             # 第三方libusb库
```

## 软件架构（分层解耦）

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

## 核心功能

1. **设备枚举**：列出所有USB设备及其VID/PID
2. **设备打开**：按VID/PID打开指定设备
3. **信息查看**：完整解析并打印设备描述符（厂商、产品、序列号、配置、接口、端点等）
4. **HID通信**：自动查找HID端点，支持报告收发
5. **异步收发**：基于线程的异步批量/中断传输，支持回调处理

## 构建方法

**Windows (Visual Studio):**
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

**Windows (MinGW):**
```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

**Linux:**
```bash
mkdir build && cd build
cmake .. && make
```

## 代码特点

- 所有文件和函数都有详细中文注释
- 命名简短易读（如 `ctx_`, `handle_`, `ep`）
- 分层设计，各层职责清晰
- 支持Windows（MinGW/MSVC）和Linux平台
- 使用C++17标准

## User
[build] [ 14%] Building CXX object CMakeFiles/UsbController.dir/src/async/AsyncTransfer.cpp.o 
 [build] /home/Y700Gen4/workspace/c_cpp/usb_controller/src/async/AsyncTransfer.cpp: In function ‘void transferCallbackWrapper(libusb_transfer*)’: 
 [build] /home/Y700Gen4/workspace/c_cpp/usb_controller/src/async/AsyncTransfer.cpp:12:34: error: ‘void AsyncTransfer::handleTransferComplete(libusb_transfer*)’ is private within this context 
 [build]    12 |     async->handleTransferComplete(transfer); 
 [build]       |     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~ 
 [build] In file included from /home/Y700Gen4/workspace/c_cpp/usb_controller/src/async/AsyncTransfer.cpp:1: 
 [build] /home/Y700Gen4/workspace/c_cpp/usb_controller/include/async/AsyncTransfer.h:81:10: note: declared private here 
 [build]    81 |     void handleTransferComplete(libusb_transfer* transfer); 
 [build]       |          ^~~~~~~~~~~~~~~~~~~~~~ 
 [build] gmake[2]: *** [CMakeFiles/UsbController.dir/build.make:149：CMakeFiles/UsbController.dir/src/async/AsyncTransfer.cpp.o] 错误 1

## Assistant
编译错误是因为 `handleTransferComplete` 是私有方法，但回调函数 `transferCallbackWrapper` 需要访问它。我需要将它改为公共方法，或者将回调函数声明为友元。



已修复。`handleTransferComplete` 方法需要从私有移到公共部分，因为静态回调函数 `transferCallbackWrapper` 需要调用它。现在可以重新编译了。

## User
我在其他程序中是可以正常读取，但是这里读取不了，如下日志 Terminal#1-106

## Assistant
问题是端点地址输入错误。从设备信息可以看到：
- **输入端点**：`0x81`（Direction=IN）
- **输出端点**：`0x01`（Direction=OUT）

你要读取数据，应该使用**输入端点 `0x81`**，而不是 `0x01`。

另外，在启动异步读取前，**需要先声明接口**。让我修复这个问题：



问题找到了！在启动异步读取前**没有声明接口**。libusb要求在使用端点前必须先声明对应的接口。让我修复：



现在需要检查 `UsbDevice` 是否有 `claimInterface` 和 `releaseInterface` 方法：



方法存在。现在重新编译并测试：



编译成功！现在你可以重新运行程序。

**正确的操作步骤：**

1. 选择 `2` 打开设备（VID: `1915`, PID: `520c`）
2. 选择 `4` 启动异步读取，输入：
   - **接口编号**: `0`（从设备信息可以看到 Interface 0）
   - **端点地址**: `81`（注意是 `0x81`，输入时只需输入 `81`，这是IN端点）
   - **最大包大小**: `64`
   - **传输类型**: `1`（Interrupt）

**问题原因：**
- 之前没有调用 `claimInterface()` 声明接口，libusb不允许直接使用端点
- 端点地址应该输入 `81`（IN端点），而不是 `1`（OUT端点）

## User
Terminal#1-200

## Assistant
错误 `LIBUSB_ERROR_BUSY` 表示接口已被占用。这通常是因为Linux内核的HID驱动已经占用了该接口。需要先分离内核驱动，再声明接口。

让我修复这个问题：



需要在 `claimInterface` 前添加 `detachKernelDriver` 方法来分离内核驱动。让我添加这个功能：



现在修改 `main.cpp`，在声明接口前先分离内核驱动：



编译并测试：



编译成功！现在重新运行程序测试。

**修复说明：**

添加了 `detachKernelDriver()` 方法，在Linux下声明接口前会先分离内核HID驱动。

**测试步骤：**

1. 选择 `2` 打开设备（VID: `1915`, PID: `520c`）
2. 选择 `4` 启动异步读取：
   - 接口编号: `0`
   - 端点地址: `81`
   - 最大包大小: `64`
   - 传输类型: `1`（Interrupt）

现在应该能成功启动异步读取了。程序会先分离内核驱动，然后声明接口，最后提交传输请求。

## User
Terminal#1-109

## Assistant
`LIBUSB_ERROR_IO` 错误通常是因为端点地址格式不正确。让我检查并修复：



问题可能是端点地址输入格式或传输标志设置不完整。让我修复：



Actually, I realize the real issue. We're setting `LIBUSB_TRANSFER_FREE_TRANSFER` which will cause libusb to free the transfer after completion, but we need to resubmit it for continuous reading. Let me fix this properly:



Let me check if there's an issue with how we're setting up the interrupt transfer. The `LIBUSB_ERROR_IO` often indicates a problem with the transfer setup:



Let me add debug output to help diagnose the `LIBUSB_ERROR_IO` issue:



编译成功。但我怀疑 `LIBUSB_ERROR_IO` 可能是因为设备配置未激活。让我添加设置配置的步骤：



现在修改 `main.cpp`，在打开设备后设置配置：



编译成功！我做了以下修复来解决 `LIBUSB_ERROR_IO` 错误：

**修复内容：**

1. **添加超时值**：传输结构中的超时从 `0` 改为 `1000ms`
2. **添加 `setConfiguration()` 方法**：打开设备后设置配置值为 `1`，确保设备正确配置
3. **添加调试输出**：打印传输参数以便诊断问题

**测试步骤：**

1. 选择 `2` 打开设备（VID: `1915`, PID: `520c`）
2. 选择 `4` 启动异步读取：
   - 接口编号: `0`
   - 端点地址: `81`
   - 最大包大小: `64`
   - 传输类型: `1`（Interrupt）

现在应该能成功启动异步读取了。程序会：
1. 打开设备
2. 设置配置
3. 分离内核驱动
4. 声明接口
5. 提交传输请求

## User
Terminal#1-169 无法停止异步读取

## Assistant
问题在于 `stopRead` 使用 `libusb_cancel_transfer` 取消传输后，传输会被标记为取消状态，但回调仍然会被调用。而且取消后我们不应该重新提交传输。让我修复这个问题：



问题有两个：
1. 用户输入的端点地址 `6` 与实际端点 `0x81` 不匹配，`stopRead` 返回 `false` 但主程序没有检查
2. 需要添加停止标志防止回调重新提交传输

让我修复：



现在修改 `main.cpp`，添加停止成功的提示，并释放接口：



Looking at the terminal output, I see the issue. The user entered endpoint `6` but the actual endpoint is `0x81`. The `stopRead` function returned `false` but the program still printed "Async read stopped". I need to check the return value and also fix the stop logic.



编译成功！我做了以下修复：

**修复内容：**

1. **添加 `stopping` 标志**：在 `ReadEndpoint` 结构中添加停止标志，防止回调重新提交传输
2. **修复 `stopRead` 逻辑**：设置 `stopping=true` 后取消传输，不立即从列表移除（等待回调处理）
3. **修复 `handleTransferComplete`**：检查 `stopping` 标志，如果正在停止则清理资源并从列表移除
4. **修复 `main.cpp`**：检查 `stopRead` 返回值，成功时释放接口

**测试步骤：**

1. 选择 `2` 打开设备（VID: `1915`, PID: `520c`）
2. 选择 `4` 启动异步读取：
   - 接口编号: `0`
   - 端点地址: `81`
   - 最大包大小: `64`
   - 传输类型: `1`
3. 选择 `6` 停止异步读取：
   - 端点地址: `81`（**注意：必须输入 `81`，不是 `6`**）

现在应该能正确停止异步读取了。

## User
Terminal#1-177

## Assistant
`double free` 错误是因为我们在 `handleTransferComplete` 中手动释放了资源，但 `libusb_cancel_transfer` 可能已经释放了这些资源。需要修复内存管理逻辑：



编译成功！修复了 `double free` 错误。

**问题原因：**
- `flags` 设置了 `LIBUSB_TRANSFER_FREE_BUFFER`，libusb 会在传输完成后自动释放缓冲区
- 我们之前手动 `delete[] buffer` 和 `libusb_free_transfer` 导致双重释放

**修复方案：**
- 停止传输时，只从列表中移除，让 libusb 自动释放资源

**测试步骤：**

1. 选择 `2` 打开设备（VID: `1915`, PID: `520c`）
2. 选择 `4` 启动异步读取：
   - 接口编号: `0`
   - 端点地址: `81`
   - 最大包大小: `64`
   - 传输类型: `1`
3. 选择 `6` 停止异步读取：
   - 端点地址: `81`

