<!-- 本文件用于说明 src/ui/keyboards/ray 模块的 GT-64HE 键盘页面、调试收发和灯效联动流程。 -->

# keyboards/ray 模块逻辑说明

## 模块职责

`src/ui/keyboards/ray` 是 GT-64HE 键盘的专属 UI 与业务模块，负责：

- 创建和管理键盘可视化页面
- 打开 `GT64HeDevice`
- 提供 USB 调试发送和连续接收
- 响应键盘按键 UI 点击
- 管理参数面板和颜色调试
- 接收音乐律动 LED 网格并映射到按键颜色
- 调用 `LedEffect` 生成每一帧灯效颜色

核心文件：

- `src/ui/keyboards/ray/GT64HeWidget.h`
- `src/ui/keyboards/ray/GT64HeWidget.cpp`
- `src/ui/keyboards/ray/GT64HeWidgetSlots.cpp`
- `src/ui/keyboards/ray/LedEffect.h`
- `src/ui/keyboards/ray/LedEffect.cpp`

## 构建依赖

```mermaid
flowchart LR
    keyboards["keyboards"] --> elements["elements"]
    keyboards --> usb_controller["usb_controller"]
    keyboards --> devices_ctrl["devices_ctrl"]
    keyboards --> protocol["protocol"]
    keyboards --> music_rhythm["music_rhythm"]
```

## 页面构造流程

```mermaid
flowchart TB
    A["GT64HeWidget 构造"] --> B["ui->setupUi(this)"]
    B --> C["new GT64HeDevice(info)"]
    C --> D["device->open()"]
    D --> E{"设备是否打开成功"}
    E -->|是| F["记录 device opened"]
    E -->|否| G["记录 open failed"]
    F --> H["连接 activeWindowChanged"]
    G --> H
    H --> I["创建窗口活跃状态定时器"]
    I --> J["连接 keyboard keyClicked"]
    J --> K["创建律动按钮和模式下拉框"]
    K --> L["缓存所有按键默认背景色"]
    L --> M["初始化 LedEffect"]
    M --> N["启动 activeWindowTimer"]
```

## USB 调试发送流程

```mermaid
flowchart TB
    A["点击 USB Tx 按钮"] --> B{"device 是否存在且打开"}
    B -->|否| C["直接返回"]
    B -->|是| D["创建 64 字节测试包"]
    D --> E["填充计数值和边界标记"]
    E --> F["device->write_async(buf)"]
    F --> G["UsbDeviceBase 转发到 UsbController"]
```

## USB 连续读取流程

```mermaid
sequenceDiagram
    participant W as GT64HeWidget
    participant D as GT64HeDevice
    participant B as UsbDeviceBase
    participant C as UsbController

    W->>D: start_read(EP_SIZE, callback)
    D->>B: 继承基类读取逻辑
    B->>C: async_start()
    B->>C: hid_read_continuous(64, callback)
    C-->>B: TransferResult
    B-->>W: on_read_done(data)
    W->>W: 打印前 10 字节
```

## 音乐律动联动流程

```mermaid
flowchart TB
    A["点击律动按钮"] --> B{"按钮是否 checked"}
    B -->|开启| C["遍历 QApplication::topLevelWidgets()"]
    C --> D{"是否找到 MusicRhythmWidget"}
    D -->|否| E["取消 checked 并关闭律动"]
    D -->|是| F["保存 m_music_ref"]
    F --> G["连接 sig_led_grid 到 on_led_grid"]
    B -->|关闭| H["断开 sig_led_grid"]
    H --> I["恢复按键原始背景色"]
```

## LED 网格映射流程

```mermaid
flowchart TB
    A["on_led_grid(QVector<float> data)"] --> B{"m_rhythm_on 是否为 true"}
    B -->|否| C["返回"]
    B -->|是| D["确保 m_key_prev_color 大小正确"]
    D --> E["遍历全部 MKeyboardKey"]
    E --> F["读取 key->getId()"]
    F --> G["通过 device->light_pos[id] 获取灯位"]
    G --> H["转换为 16x5 grid_idx"]
    H --> I["生成每个按键 brightness"]
    I --> J["m_led_effect.next_frame()"]
    J --> K["仅在颜色变化时更新 key 样式"]
```

## LedEffect 模式

| 模式 | 说明 |
| --- | --- |
| `LED_FX_COLOR_CYCLE` | 所有灯统一色相随时间循环 |
| `LED_FX_RAINBOW` | 色相随时间和按键位置旋转 |
| `LED_FX_BREATHING` | 基础色按正弦亮度呼吸 |
| `LED_FX_WAVE` | 基础色按位置形成波浪 |
| `LED_FX_SPARKLE` | 随机位置星火闪烁 |
| `LED_FX_STATIC` | 固定基础色 |

## 当前状态

- 键盘 UI 和颜色调试能力较完整。
- USB 收发仍是调试包和原始字节打印。
- `hphpt.h` 已被包含，但没有真正调用 `hpt_decode()` 或 `hpt_encode()`。
- 音乐律动只影响 UI 按键背景色，没有下发到物理键盘。
- `device->light_pos[id]` 假设 key id 总是合法，缺少边界检查。

## 改进建议

1. 将 USB Tx/Rx 从调试字节流切换为 HPHPT 协议帧。
2. 在 `on_read_done()` 中调用 `hpt_decode()`，并根据命令回调更新 UI 状态。
3. 在 `on_led_grid()` 中校验 key id 是否小于 `light_pos` 长度。
4. 将 UI 灯效结果转换为灯效协议包，通过 `INTERFACE_LAMP` 下发到物理键盘。
5. 将律动窗口依赖改为显式注入，避免顶层窗口遍历带来的不确定性。
6. 将大量颜色调试槽函数抽出公共方法，减少重复代码。
