<!-- 本文件用于说明 src/usb_controller 模块的 libusb 封装、设备管理和传输流程。 -->

# usb_controller 模块逻辑说明

## 模块职责

`src/usb_controller` 是项目的 USB 底层控制库，负责屏蔽 libusb 的复杂细节，对上层提供统一接口。

主要能力：

- 初始化和释放 libusb 上下文
- 枚举 USB 设备
- 查找 HID 设备
- 打开和关闭 HID 设备
- 执行同步读写
- 执行异步读写和连续读取
- 监听设备热插拔

核心文件：

- `src/usb_controller/src/usb_controller.hpp`
- `src/usb_controller/src/usb_controller.cpp`
- `src/usb_controller/src/usb_core/*`
- `src/usb_controller/src/usb_io/*`
- `src/usb_controller/doc.md`

## 内部结构

```mermaid
flowchart TB
    A["UsbController<br/>统一外观 Facade"] --> B["UsbContext<br/>libusb 上下文"]
    A --> C["UsbDeviceManager<br/>设备枚举与查询"]
    A --> D["HidDevice<br/>HID 打开与报告读写"]
    A --> E["SyncTransfer<br/>同步传输"]
    A --> F["AsyncTransfer<br/>异步传输"]
    A --> G["UsbEventThread<br/>事件线程"]
    A --> H["UsbHotplugBase<br/>热插拔抽象"]
    B --> I["libusb"]
    C --> I
    D --> I
    E --> I
    F --> I
    G --> I
    H --> I
```

## 设备枚举流程

```mermaid
flowchart TB
    A["UsbController::refresh_devices()"] --> B["UsbDeviceManager 清空旧列表"]
    B --> C["libusb_get_device_list"]
    C --> D["遍历 libusb_device"]
    D --> E["读取设备描述符"]
    E --> F["读取配置、接口、端点信息"]
    F --> G["构造 UsbDevice"]
    G --> H["保存到 devices()"]
```

## HID 打开流程

```mermaid
flowchart TB
    A["open_hid_device_by_vid_pid(vid, pid)"] --> B["find_by_vid_pid"]
    B --> C{"是否找到设备"}
    C -->|否| D["返回 false"]
    C -->|是| E["查找 HID 接口"]
    E --> F{"是否存在 HID 接口"}
    F -->|否| D
    F -->|是| G["创建 HidDevice"]
    G --> H["打开设备句柄"]
    H --> I["必要时分离内核驱动"]
    I --> J["声明接口"]
    J --> K["绑定 SyncTransfer"]
    K --> L["返回 true"]
```

## 异步连续读取流程

```mermaid
sequenceDiagram
    participant U as 上层模块
    participant C as UsbController
    participant A as AsyncTransfer
    participant T as UsbEventThread
    participant L as libusb

    U->>C: async_start()
    C->>T: 启动事件线程
    U->>C: hid_read_continuous(length, callback)
    C->>A: 创建并提交读取 transfer
    A->>L: libusb_submit_transfer
    L-->>T: transfer 完成事件
    T-->>A: 处理回调
    A-->>U: callback(TransferResult)
    A->>L: 再次提交下一次读取
```

## 热插拔流程

```mermaid
sequenceDiagram
    participant U as USBWidget
    participant C as UsbController
    participant H as UsbHotplugBase
    participant T as UsbEventThread
    participant L as libusb

    U->>C: hotplug_start(callback)
    C->>H: 注册热插拔回调
    C->>T: 启动事件线程
    L-->>T: 设备插入/拔出事件
    T-->>H: libusb hotplug callback
    H-->>C: HotplugEvent + UsbDevice
    C-->>U: 上层 callback
```

## 对外 API 分类

| 分类 | 代表 API | 说明 |
| --- | --- | --- |
| 基础设置 | `set_debug_level()`、`refresh_devices()` | 初始化和刷新 |
| 设备查询 | `devices()`、`find_by_vid_pid()`、`find_hid_devices()` | 获取设备信息 |
| HID 操作 | `open_hid_device_by_vid_pid()`、`hid_read()`、`hid_write()` | HID 报告读写 |
| 同步传输 | `bulk_read()`、`interrupt_write()` | 通用传输 |
| 异步传输 | `hid_read_continuous()`、`hid_write_async()` | 后台读写 |
| 热插拔 | `hotplug_start()`、`hotplug_stop()` | 插拔监听 |

## 当前状态

- 底层 USB 封装相对完整，并且已有独立 `doc.md`。
- 上层主要通过 HID API 使用该模块。
- 热插拔能力已接入 `USBWidget`。
- 异步连续读取已接入键盘调试页面。

## 改进建议

1. 为 `TransferResult` 增加更结构化的错误码，减少仅依赖字符串判断。
2. 明确异步引擎与热插拔事件线程的关系，避免重复启动或关闭顺序问题。
3. 为 HID 接口选择提供更细控制，例如按接口号或端点能力选择。
4. 增加 mock 或 fake backend，方便无设备环境下测试 UI 和协议逻辑。
5. 将 `src/usb_controller/doc.md` 的成熟内容整合到项目总文档中。
