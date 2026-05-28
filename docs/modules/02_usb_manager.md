<!-- 本文件用于说明 src/ui/usb_manager 模块的 USB 设备管理与页面切换流程。 -->

# usb_manager 模块逻辑说明

## 模块职责

`src/ui/usb_manager` 是 USB 功能入口，负责：

- 创建 `UsbController` 并刷新当前 USB 设备列表
- 启动 USB 热插拔监听
- 将设备信息展示到设备管理控件
- 响应用户进入设备的请求
- 创建并展示 GT-64HE 键盘控制页面

核心文件：

- `src/ui/usb_manager/USBWidget.cpp`
- `src/ui/usb_manager/USBWidgetSlots.cpp`
- `src/ui/usb_manager/USBWidget.h`
- `src/ui/usb_manager/USBDeviceManagerWidget.*`
- `src/ui/usb_manager/Ui_USBWidget.*`

## 构建依赖

```mermaid
flowchart LR
    usb_manager["usb_manager"] --> devices_ctrl["devices_ctrl"]
    usb_manager --> console["console"]
    usb_manager --> keyboards["keyboards"]
```

## 初始化流程

```mermaid
flowchart TB
    A["USBWidget 构造"] --> B["ui->setupUi(this)"]
    B --> C["注册 UsbDeviceInfo 元类型"]
    C --> D["连接 sig_dev_insert/remove"]
    D --> E["m_ctrl.refresh_devices()"]
    E --> F["dev_hotplug_init()"]
    F --> G["遍历 m_ctrl.devices()"]
    G --> H["UsbDeviceInfo::from_usb_device"]
    H --> I["ui->usbDeviceManager->haveDevice(info)"]
```

## 热插拔流程

```mermaid
sequenceDiagram
    participant L as libusb
    participant C as UsbController
    participant U as USBWidget
    participant M as USBDeviceManagerWidget

    U->>C: hotplug_start(callback)
    L-->>C: HotplugEvent Arrived / Left
    C-->>U: callback(event, device)
    U->>U: UsbDeviceInfo::from_usb_device(device)
    alt 插入
        U->>U: emit sig_dev_insert(info)
        U->>M: addDevice(info)
    else 拔出
        U->>U: emit sig_dev_remove(info)
        U->>M: delDevice(info)
    end
```

## 进入键盘页面流程

```mermaid
flowchart TB
    A["设备管理控件发出 enterRequested(info)"] --> B["USBWidget::on_usbDeviceManager_enterRequested"]
    B --> C["openUSBWidget(info)"]
    C --> D{"info.is_hid 是否为 true"}
    D -->|否| E["打印 skip non-HID 并返回"]
    D -->|是| F["gt64heWidget(info)"]
    F --> G{"m_gt64heWidget 是否为空"}
    G -->|为空| H["new GT64HeWidget(info)"]
    H --> I["ui->addSubWidget(PAGE_TY_KB, widget)"]
    I --> J["连接 exitWidget / exitWindow"]
    J --> K["ui->showSubWidget(PAGE_TY_KB)"]
    G -->|非空| L["保持当前键盘页面"]
```

## 页面切换流程

```mermaid
flowchart LR
    A["PAGE_TY_MAIN"] -->|"进入 HID 设备"| B["PAGE_TY_KB"]
    B -->|"键盘页面 exitWidget"| A
    A -->|"窗口 resize"| C["400 x 200"]
    B -->|"窗口 resize"| D["1000 x 700"]
```

## 当前状态

- 已完成设备枚举、热插拔通知和设备列表更新。
- 已能从设备列表进入键盘控制页面。
- 只支持单个 `m_gt64heWidget` 实例。
- `closeUSBWidget()` 当前为空实现。
- `openUSBWidget()` 只判断是否 HID，没有校验 GT-64HE 的 VID/PID。

## 改进建议

1. 在 `openUSBWidget()` 中增加 `GT64HeDevice::VID` 和 `GT64HeDevice::PID` 校验。
2. 补齐 `closeUSBWidget()`，拔出当前设备时应关闭对应页面并释放设备。
3. 如果后续支持多设备，应将 `m_gt64heWidget` 从单指针改为按设备 ID 管理的映射。
4. 热插拔回调来自 USB 事件线程时，应确保 UI 更新始终通过 Qt queued connection 回到主线程。
5. 设备列表应展示设备类型、VID/PID、产品名和可进入状态，避免用户误点非目标设备。
