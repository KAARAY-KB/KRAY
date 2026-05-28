<!-- 本文件用于说明 src/ui/devices_ctrl 模块的 Qt 设备封装和 USB 操作模板流程。 -->

# devices_ctrl 模块逻辑说明

## 模块职责

`src/ui/devices_ctrl` 位于 UI 与底层 USB 控制库之间，负责：

- 将底层 `usb_ctrl::core::UsbDevice` 转换为 Qt 层可传递的 `UsbDeviceInfo`
- 封装通用 USB 设备打开、关闭、读写流程
- 为 GT-64HE 定义设备常量、接口编号和灯位映射

核心文件：

- `src/ui/devices_ctrl/usb_device_info.hpp`
- `src/ui/devices_ctrl/usb_device_base.hpp`
- `src/ui/devices_ctrl/usb_device_base.cpp`
- `src/ui/devices_ctrl/gt64he_device.hpp`
- `src/ui/devices_ctrl/gt64he_device.cpp`

## 构建依赖

```mermaid
flowchart LR
    devices_ctrl["devices_ctrl"] --> usb_controller["usb_controller"]
    devices_ctrl --> console["console"]
    devices_ctrl --> qtcore["Qt Core"]
```

## 设计角色

```mermaid
classDiagram
    class UsbDeviceInfo {
        +DeviceInfo di
        +bool is_hid
        +from_usb_device()
        +to_string()
    }

    class UsbDeviceBase {
        -UsbDeviceInfo m_info
        -unique_ptr~UsbController~ m_ctrl
        -DataCallback m_read_cb
        +open()
        +close()
        +write()
        +read()
        +start_read()
        +stop_read()
        +write_async()
        #on_opened()
        #on_closing()
    }

    class GT64HeDevice {
        +VID = 0x9013
        +PID = 0x2601
        +EP_SIZE = 64
        +light_pos[63]
    }

    UsbDeviceBase <|-- GT64HeDevice
    UsbDeviceInfo --> UsbDeviceBase
```

## 打开设备流程

```mermaid
flowchart TB
    A["UsbDeviceBase::open()"] --> B{"m_ctrl 是否存在"}
    B -->|否| C["返回 false"]
    B -->|是| D["m_ctrl->refresh_devices()"]
    D --> E["open_hid_device_by_vid_pid(vid, pid)"]
    E --> F{"打开是否成功"}
    F -->|否| G["记录失败日志并返回 false"]
    F -->|是| H["on_opened()"]
    H --> I{"子类初始化是否成功"}
    I -->|否| J["close_hid_device() 并返回 false"]
    I -->|是| K["返回 true"]
```

## 异步读取流程

```mermaid
sequenceDiagram
    participant W as GT64HeWidget
    participant B as UsbDeviceBase
    participant C as UsbController
    participant L as libusb

    W->>B: start_read(64, callback)
    B->>B: 保存 m_read_cb
    B->>C: async_start()
    B->>C: hid_read_continuous(64, internalCallback)
    C->>L: 提交异步 HID 读取
    L-->>C: TransferResult
    C-->>B: internalCallback(result)
    B-->>W: m_read_cb(result.data)
```

## 写入流程

```mermaid
flowchart TB
    A["write(data) / write_async(data)"] --> B{"设备是否 is_open()"}
    B -->|否| C["记录 device not open"]
    B -->|是| D{"同步还是异步"}
    D -->|同步| E["m_ctrl->hid_write(data)"]
    D -->|异步| F["m_ctrl->hid_write_async(data, callback)"]
    E --> G["返回 TransferResult.success"]
    F --> H["回调中记录成功或失败"]
```

## GT-64HE 设备信息

| 常量 | 值 | 说明 |
| --- | --- | --- |
| `VID` | `0x9013` | 厂商 ID |
| `PID` | `0x2601` | 产品 ID |
| `EP_SIZE` | `64` | HID 报告长度 |
| `INTERFACE_KEYBOARD` | `0` | 键盘接口 |
| `INTERFACE_LAMP` | 枚举值 | 灯效接口，当前尚未真正使用 |

## 当前状态

- `UsbDeviceBase` 已经形成模板方法结构。
- `GT64HeDevice` 目前主要提供常量和灯位表，没有复杂设备命令封装。
- 读写接口仍直接传输原始字节，没有和 HPHPT 协议层强绑定。

## 改进建议

1. 在 `GT64HeDevice` 中增加明确的业务方法，例如 `sendLightFrame()`、`readDeviceAttr()`。
2. 将协议封包逻辑放在设备类或专门的 protocol adapter 中，避免 UI 层直接拼字节。
3. `light_pos` 建议改为 `static constexpr std::array`，避免每个实例都持有一份固定表。
4. 增加接口选择能力，当前 `open_hid_device_by_vid_pid()` 只按 VID/PID 找第一个 HID。
5. 对异步回调对象生命周期做防护，避免窗口销毁后底层回调访问悬空 `this`。
