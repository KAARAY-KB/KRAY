<!-- 本文件用于说明 src/core 模块的职责、窗口调度逻辑和启动流程。 -->

# core 模块逻辑说明

## 模块职责

`src/core` 是应用的主壳模块，负责：

- 定义应用入口 `main.cpp`
- 创建 Qt 应用对象、加载资源和翻译
- 创建并显示 `Kray` 主窗口
- 通过主窗口按钮懒加载各功能窗口
- 管理部分子窗口的关闭和释放

核心文件：

- `src/core/main.cpp`
- `src/core/kray.h`
- `src/core/kray.cpp`
- `src/core/kray_msg.cpp`
- `src/core/t1.cpp`
- `src/core/t2.cpp`

## 构建依赖

```mermaid
flowchart LR
    core["core 静态库"] --> qcustomplot["qcustomplot"]
    core --> usb_manager["usb_manager"]
    core --> music_rhythm["music_rhythm"]
    core --> console["console"]
    core --> qt["Qt Widgets/Core/Gui"]
```

## 启动流程

```mermaid
flowchart TB
    A["main(argc, argv)"] --> B["Q_INIT_RESOURCE(resources)"]
    B --> C["创建 QApplication"]
    C --> D["遍历系统语言列表"]
    D --> E{"是否找到 KRAY_xx 翻译资源"}
    E -->|是| F["installTranslator"]
    E -->|否| G["使用默认语言"]
    F --> H["设置应用字体"]
    G --> H
    H --> I["创建 Kray 主窗口"]
    I --> J["w.show()"]
    J --> K["a.exec() 进入 Qt 事件循环"]
```

## 主窗口逻辑

`Kray` 构造函数主要做三件事：

1. 初始化 UI、标题和图标。
2. 创建全局控制台窗口 `_consoleWin` 并注册日志 sink。
3. 创建系统托盘图标，并绑定“显示”和“退出”动作。

```mermaid
flowchart TB
    A["Kray 构造"] --> B["ui->setupUi(this)"]
    B --> C["创建 ConsoleWidget"]
    C --> D["Console::registerSink"]
    D --> E["创建 MSystemTrayIcon"]
    E --> F["绑定托盘 actionTriggered"]
    F --> G["绑定 closeToQuitToggled"]
```

## 子窗口懒加载流程

```mermaid
flowchart TB
    A["用户点击主窗口按钮"] --> B{"目标窗口指针是否为空"}
    B -->|为空| C["new 对应 QWidget"]
    C --> D["连接 exitWindow 信号"]
    D --> E["保存成员指针"]
    B -->|非空| F["复用已有窗口"]
    E --> G["show_top() 或 show()"]
    F --> G
```

按钮与窗口关系：

| 按钮槽函数 | 创建对象 | 说明 |
| --- | --- | --- |
| `on_btn_console_clicked()` | `_consoleWin` | 显示日志控制台 |
| `on_btn_usb_clicked()` | `USBWidget` | USB 设备管理 |
| `on_btn_music_clicked()` | `MusicRhythmWidget` | 音乐律动窗口 |
| `on_btn_t1_clicked()` | `T1` | libusb 和图表测试页 |
| `on_btn_t2_clicked()` | `T2` | 占位测试页 |
| `on_btn_audio_clicked()` | 无 | 当前为空实现 |

## 关闭流程

```mermaid
flowchart TB
    A["Kray::closeEvent"] --> B["关闭并删除 t1"]
    B --> C["关闭并删除 t2"]
    C --> D["删除系统托盘"]
    D --> E["关闭并删除 USBWidget"]
    E --> F["关闭并删除 MusicRhythmWidget"]
    F --> G["保留 ConsoleWidget"]
    G --> H{"m_close_to_quit 是否为 true"}
    H -->|是| I["qApp->quit()"]
    H -->|否| J["仅关闭主窗口"]
```

## 当前状态

- 主窗口已经承担应用总调度职责。
- 子窗口采用懒加载，避免启动时创建所有功能窗口。
- `main.cpp` 被编入 `core` 静态库，最终可执行文件通过链接 `core` 获得入口。
- 关闭逻辑中存在多处手动释放对象。

## 改进建议

1. 将 `main.cpp` 从 `core` 静态库中拆回可执行目标，构建结构会更直观。
2. 提取 `cleanupChildWindows()`，避免 `closeEvent` 和析构函数重复释放同一批成员。
3. 优先使用 Qt 父子对象所有权或 `deleteLater()`，减少手写 `delete`。
4. `on_btn_audio_clicked()` 当前为空，应删除按钮、隐藏入口或补齐功能。
5. `_consoleWin` 当前是文件级静态变量，后续可改为 `Kray` 成员或日志服务对象。
