#!/usr/bin/env python3
# ============================================================================
# usb_test_ui.py - USB 控制器集成测试 UI（Flet 现代化版本）
#
# 功能说明：
#   使用 Flet (Flutter) 构建现代化 Material Design 3 风格界面，
#   通过 ctypes 调用 C API 共享库，整合所有 USB 控制器功能。
#
# 特性：
#   - Material Design 3 风格
#   - 支持暗色/亮色主题切换
#   - 响应式布局
#   - 圆角卡片、阴影效果
#   - 平滑动画过渡
#
# 功能模块：
#   1. 设备枚举：列出/刷新/搜索设备
#   2. HID 设备：打开/关闭/信息查询
#   3. HID I/O：同步读写、特性报告
#   4. 异步传输：连续读取、轮询结果
#   5. 批量传输：指定端点读写
#   6. 热插拔：监听设备插拔事件
# ============================================================================

import flet as ft
import ctypes
import ctypes.util
import datetime
import json
import os
import sys
import platform

# ============================================================================
# 查找并加载共享库
# ============================================================================
def find_lib():
    """查找共享库（跨平台，自动搜索所有构建类型目录）"""
    # 根据平台确定库文件名
    if platform.system() == "Windows":
        lib_name = "usb_ctrl_capi.dll"
    elif platform.system() == "Darwin":
        lib_name = "libusb_ctrl_capi.dylib"
    else:
        lib_name = "libusb_ctrl_capi.so"

    # 获取脚本所在目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # 项目根目录
    project_root = os.path.abspath(os.path.join(script_dir, "..", "..", ".."))
    # 构建根目录
    build_root = os.path.join(project_root, "build")

    # 所有可能的构建类型目录名
    cfg_dirs = ["Debug", "Release", "RelWithDebInfo", "MinSizeRel"]
    # 共享库可能的子目录
    lib_subdirs = ["lib/dynamic", "lib/shared", "lib"]
    # 构建目标子目录（main 或 usb）
    target_dirs = ["usb", "main", ""]

    # 候选路径列表
    candidates = [os.path.join(script_dir, lib_name)]

    # 遍历所有组合
    for target in target_dirs:
        for cfg in cfg_dirs:
            for lib_sub in lib_subdirs:
                candidates.append(os.path.join(build_root, target, cfg, lib_sub, lib_name))
        for lib_sub in lib_subdirs:
            candidates.append(os.path.join(build_root, target, lib_sub, lib_name))

    # 搜索存在的路径
    for path in candidates:
        abs_path = os.path.abspath(path)
        if os.path.exists(abs_path):
            return abs_path
    return None


LIB_PATH = find_lib()
if not LIB_PATH:
    print("ERROR: Shared library not found!")
    print(f"  Platform: {platform.system()}")
    print("Please build the project first:")
    print("  Linux/macOS: ./build.sh usb")
    print("  Windows:     build.bat usb")
    sys.exit(1)

# Windows 下添加 DLL 搜索路径（MinGW 运行时依赖）
if platform.system() == "Windows":
    # 将 DLL 所在目录加入搜索路径
    dll_dir = os.path.dirname(LIB_PATH)
    os.add_dll_directory(dll_dir)

    # 查找 MinGW 运行时目录（libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll）
    def _find_mingw_bin():
        """从 CMakeCache.txt 中读取编译器路径，推导 MinGW bin 目录"""
        # 从 DLL 所在目录向上搜索 CMakeCache.txt
        search_dir = dll_dir
        for _ in range(5):
            cache_path = os.path.join(search_dir, "CMakeCache.txt")
            if os.path.exists(cache_path):
                try:
                    with open(cache_path, "r", encoding="utf-8", errors="ignore") as f:
                        for line in f:
                            if "CMAKE_CXX_COMPILER:FILEPATH=" in line:
                                compiler = line.split("=", 1)[1].strip()
                                if compiler.endswith("g++.exe"):
                                    return os.path.dirname(compiler)
                except Exception:
                    pass
                break
            parent = os.path.dirname(search_dir)
            if parent == search_dir:
                break
            search_dir = parent
        return None

    mingw_bin = _find_mingw_bin()
    if mingw_bin and os.path.isdir(mingw_bin):
        os.add_dll_directory(mingw_bin)

print(f"Loading library: {LIB_PATH}")
lib = ctypes.CDLL(LIB_PATH)

# ============================================================================
# C API 函数签名声明
# ============================================================================

# 创建/销毁
lib.usb_ctrl_create.restype = ctypes.c_void_p
lib.usb_ctrl_create.argtypes = []

lib.usb_ctrl_destroy.restype = None
lib.usb_ctrl_destroy.argtypes = [ctypes.c_void_p]

# 基础设置
lib.usb_ctrl_set_debug.restype = None
lib.usb_ctrl_set_debug.argtypes = [ctypes.c_void_p, ctypes.c_int]

lib.usb_ctrl_refresh.restype = None
lib.usb_ctrl_refresh.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_device_count.restype = ctypes.c_int
lib.usb_ctrl_device_count.argtypes = [ctypes.c_void_p]

# 设备查询
lib.usb_ctrl_list_devices.restype = ctypes.c_void_p
lib.usb_ctrl_list_devices.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_list_devices_detail.restype = ctypes.c_void_p
lib.usb_ctrl_list_devices_detail.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_list_devices_tree.restype = ctypes.c_void_p
lib.usb_ctrl_list_devices_tree.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_list_hid_devices.restype = ctypes.c_void_p
lib.usb_ctrl_list_hid_devices.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_device_detail.restype = ctypes.c_void_p
lib.usb_ctrl_device_detail.argtypes = [ctypes.c_void_p, ctypes.c_int]

# HID 设备操作
lib.usb_ctrl_open_hid.restype = ctypes.c_int
lib.usb_ctrl_open_hid.argtypes = [ctypes.c_void_p, ctypes.c_int]

lib.usb_ctrl_open_hid_vid_pid.restype = ctypes.c_int
lib.usb_ctrl_open_hid_vid_pid.argtypes = [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint16]

lib.usb_ctrl_close_hid.restype = None
lib.usb_ctrl_close_hid.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_is_hid_open.restype = ctypes.c_int
lib.usb_ctrl_is_hid_open.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_hid_info.restype = ctypes.c_void_p
lib.usb_ctrl_hid_info.argtypes = [ctypes.c_void_p]

# HID 数据传输
lib.usb_ctrl_hid_read.restype = ctypes.c_void_p
lib.usb_ctrl_hid_read.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_uint]

lib.usb_ctrl_hid_read_latest.restype = ctypes.c_void_p
lib.usb_ctrl_hid_read_latest.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_uint]

lib.usb_ctrl_hid_write.restype = ctypes.c_void_p
lib.usb_ctrl_hid_write.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.c_int, ctypes.c_uint]

lib.usb_ctrl_hid_get_feature.restype = ctypes.c_void_p
lib.usb_ctrl_hid_get_feature.argtypes = [ctypes.c_void_p, ctypes.c_uint8, ctypes.c_int, ctypes.c_uint]

lib.usb_ctrl_hid_send_feature.restype = ctypes.c_void_p
lib.usb_ctrl_hid_send_feature.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.c_int, ctypes.c_uint]

# 批量传输
lib.usb_ctrl_bulk_read.restype = ctypes.c_void_p
lib.usb_ctrl_bulk_read.argtypes = [ctypes.c_void_p, ctypes.c_uint8, ctypes.c_int, ctypes.c_uint]

lib.usb_ctrl_bulk_write.restype = ctypes.c_void_p
lib.usb_ctrl_bulk_write.argtypes = [ctypes.c_void_p, ctypes.c_uint8, ctypes.POINTER(ctypes.c_uint8), ctypes.c_int, ctypes.c_uint]

# 异步传输
lib.usb_ctrl_async_start.restype = None
lib.usb_ctrl_async_start.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_async_stop.restype = None
lib.usb_ctrl_async_stop.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_async_pending.restype = ctypes.c_int
lib.usb_ctrl_async_pending.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_async_read_continuous.restype = None
lib.usb_ctrl_async_read_continuous.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_uint]

lib.usb_ctrl_async_stop_continuous.restype = None
lib.usb_ctrl_async_stop_continuous.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_async_poll.restype = ctypes.c_void_p
lib.usb_ctrl_async_poll.argtypes = [ctypes.c_void_p]

# 热插拔
lib.usb_ctrl_is_hotplug_supported.restype = ctypes.c_int
lib.usb_ctrl_is_hotplug_supported.argtypes = []

lib.usb_ctrl_hotplug_start.restype = ctypes.c_int
lib.usb_ctrl_hotplug_start.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_hotplug_stop.restype = None
lib.usb_ctrl_hotplug_stop.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_is_hotplug_listening.restype = ctypes.c_int
lib.usb_ctrl_is_hotplug_listening.argtypes = [ctypes.c_void_p]

lib.usb_ctrl_hotplug_poll.restype = ctypes.c_void_p
lib.usb_ctrl_hotplug_poll.argtypes = [ctypes.c_void_p]

# 内存管理
lib.usb_ctrl_free_str.restype = None
lib.usb_ctrl_free_str.argtypes = [ctypes.c_void_p]


# ============================================================================
# 辅助函数：调用返回字符串的 C API，自动释放内存
# ============================================================================
def call_str(func, *args):
    """调用返回 char* 的 C 函数，自动释放内存"""
    ptr = func(*args)
    if not ptr:
        return ""
    # 从 void 指针读取字符串
    result = ctypes.cast(ptr, ctypes.c_char_p).value
    s = result.decode("utf-8", errors="replace") if result else ""
    # 释放 C 端分配的内存
    lib.usb_ctrl_free_str(ptr)
    return s


def call_json(func, *args):
    """调用返回 JSON 字符串的 C 函数，解析为字典"""
    s = call_str(func, *args)
    if not s:
        return {}
    try:
        return json.loads(s)
    except json.JSONDecodeError:
        return {"raw": s}


def hex_to_bytes(hex_str):
    """十六进制字符串转字节数组"""
    hex_str = hex_str.replace(" ", "").replace("0x", "")
    data = []
    for i in range(0, len(hex_str), 2):
        if i + 1 < len(hex_str):
            data.append(int(hex_str[i:i+2], 16))
    arr = (ctypes.c_uint8 * len(data))(*data)
    return arr, len(data)


def format_result(result):
    """格式化传输结果为可读字符串"""
    success = result.get("success", False)
    bytes_count = result.get("bytes", 0)
    hex_data = result.get("data_hex", "")
    ascii_data = result.get("data_ascii", "")
    error_msg = result.get("error_msg", "")
    status = "OK" if success else "FAIL"
    lines = [f"[{status}] {bytes_count} bytes"]
    if hex_data:
        lines.append(f"  HEX  : {hex_data}")
    if ascii_data:
        lines.append(f"  ASCII: {ascii_data}")
    if error_msg:
        lines.append(f"  Error: {error_msg}")
    return "\n".join(lines)


# ============================================================================
# 主应用类
# ============================================================================
class UsbTestApp(ft.Column):
    """USB 控制器集成测试 UI"""

    def __init__(self):
        super().__init__(expand=True)
        
        # 创建控制器
        self.handle = lib.usb_ctrl_create()
        if not self.handle:
            raise RuntimeError("Failed to create USB controller")

        # 异步轮询状态
        self.async_running = False
        self.hotplug_running = False
        self.async_count = 0
        
        # 主题模式
        self.theme_mode = ft.ThemeMode.LIGHT
        
        # 构建 UI
        self._build_ui()

    def _build_ui(self):
        """构建主界面"""
        # 预创建状态栏文本控件（不能在容器内赋值）
        self.device_count_text = ft.Text("0", size=28, weight=ft.FontWeight.BOLD, color=ft.Colors.BLUE)
        self.hid_status_text = ft.Text("Closed", size=28, weight=ft.FontWeight.BOLD, color=ft.Colors.RED)

        # 顶部应用栏
        self.appbar = ft.AppBar(
            title=ft.Text("USB Controller Test", size=34, weight=ft.FontWeight.BOLD),
            bgcolor=ft.Colors.SURFACE_CONTAINER_HIGH,
            actions=[
                ft.IconButton(
                    icon=ft.Icons.DARK_MODE if self.theme_mode == ft.ThemeMode.LIGHT else ft.Icons.LIGHT_MODE,
                    tooltip="Toggle Theme",
                    on_click=self._toggle_theme,
                ),
                ft.PopupMenuButton(
                    items=[
                        ft.PopupMenuItem(content="About", on_click=lambda _: self._show_snack("USB Controller Test v1.0")),
                    ],
                ),
            ],
        )

        # 状态栏
        self.status_bar = ft.Container(
            content=ft.Row([
                ft.Icon(ft.Icons.USB, color=ft.Colors.BLUE),
                ft.Text("Devices:", size=28),
                self.device_count_text,
                ft.Container(width=20),
                ft.Icon(ft.Icons.LOCK_OUTLINE, color=ft.Colors.RED),
                ft.Text("HID:", size=28),
                self.hid_status_text,
            ], spacing=5),
            padding=ft.Padding(15, 8, 15, 8),
            bgcolor=ft.Colors.SURFACE_CONTAINER_LOWEST,
            border_radius=10,
        )

        # 输出日志区域（ListView 智能滚动：底部时自动滚动，上翻时不滚动）
        self._log_at_bottom = True  # 跟踪用户是否在底部
        self.output_list = ft.ListView(
            expand=True,
            spacing=2,
            auto_scroll=True,
            on_scroll=self._on_log_scroll,
        )
        self.output_container = ft.Container(
            content=self.output_list,
            expand=True,
            bgcolor=ft.Colors.SURFACE_CONTAINER_LOWEST,
            border=ft.Border(
                left=ft.BorderSide(1, ft.Colors.OUTLINE_VARIANT),
                top=ft.BorderSide(1, ft.Colors.OUTLINE_VARIANT),
                right=ft.BorderSide(1, ft.Colors.OUTLINE_VARIANT),
                bottom=ft.BorderSide(1, ft.Colors.OUTLINE_VARIANT),
            ),
            border_radius=6,
            padding=8,
        )

        # 左侧导航栏（Tab 切换）
        self.navigation_rail = ft.NavigationRail(
            selected_index=0,
            label_type=ft.NavigationRailLabelType.ALL,
            min_width=160,
            min_extended_width=220,
            destinations=[
                ft.NavigationRailDestination(icon=ft.Icons.USB, label="Device", selected_icon=ft.Icons.USB),
                ft.NavigationRailDestination(icon=ft.Icons.LOCK_OUTLINE, label="HID", selected_icon=ft.Icons.LOCK_OPEN),
                ft.NavigationRailDestination(icon=ft.Icons.SYNC_ALT, label="I/O", selected_icon=ft.Icons.SYNC_ALT),
                ft.NavigationRailDestination(icon=ft.Icons.TIMELAPSE, label="Async", selected_icon=ft.Icons.TIMELAPSE),
                ft.NavigationRailDestination(icon=ft.Icons.CABLE, label="Hotplug", selected_icon=ft.Icons.CABLE),
            ],
            on_change=self._on_tab_change,
        )

        # 内容区域容器
        self.content_area = ft.Container(
            content=self._build_device_tab(),
            expand=True,
        )

        # 主布局
        self.controls = [
            self.appbar,
            ft.Divider(height=1),
            self.status_bar,
            ft.Divider(height=1),
            ft.Row(
                [
                    self.navigation_rail,
                    ft.VerticalDivider(width=1),
                    ft.Column(
                        [
                            ft.Card(
                                content=ft.Container(
                                    content=self.content_area,
                                    padding=16,
                                ),
                                elevation=1,
                                expand=3,
                            ),
                            ft.Divider(height=1),
                            ft.Card(
                                content=ft.Container(
                                    content=self.output_container,
                                    padding=8,
                                ),
                                elevation=1,
                                expand=2,
                            ),
                        ],
                        expand=True,
                    ),
                ],
                expand=True,
            ),
        ]

    # ---- Tab 切换 ----
    def _on_tab_change(self, e):
        """处理 Tab 切换事件"""
        index = e.control.selected_index
        tabs = [
            self._build_device_tab(),
            self._build_hid_tab(),
            self._build_io_tab(),
            self._build_async_tab(),
            self._build_hotplug_tab(),
        ]
        self.content_area.content = tabs[index]
        self.update()

    # ---- 主题切换 ----
    def _toggle_theme(self, e):
        """切换暗色/亮色主题"""
        self.theme_mode = ft.ThemeMode.DARK if self.theme_mode == ft.ThemeMode.LIGHT else ft.ThemeMode.LIGHT
        self.page.theme_mode = self.theme_mode
        self.appbar.actions[0].icon = ft.Icons.DARK_MODE if self.theme_mode == ft.ThemeMode.LIGHT else ft.Icons.LIGHT_MODE
        self.page.update()

    # ---- 设备枚举页 ----
    def _build_device_tab(self):
        """构建设备枚举页"""
        # 设备索引输入框
        self.dev_idx_field = ft.TextField(label="Device Index", width=200, value="0")
        # VID/PID 输入框
        self.vid_field = ft.TextField(label="VID", width=150, value="0x")
        self.pid_field = ft.TextField(label="PID", width=150, value="0x")

        return ft.Column([
            # 刷新按钮行
            ft.Row([
                ft.Button(
                    "Refresh",
                    icon=ft.Icons.REFRESH,
                    style=ft.ButtonStyle(bgcolor=ft.Colors.PRIMARY_CONTAINER),
                    on_click=self._refresh_devices,
                ),
                ft.Button(
                    "List All",
                    icon=ft.Icons.LIST,
                    style=ft.ButtonStyle(bgcolor=ft.Colors.SECONDARY_CONTAINER),
                    on_click=self._list_devices,
                ),
            ], spacing=10),

            ft.Divider(),

            # 设备操作按钮组
            ft.Text("Device Operations", size=32, weight=ft.FontWeight.W_600, color=ft.Colors.ON_SURFACE_VARIANT),
            ft.Row([
                ft.OutlinedButton("List Detail", icon=ft.Icons.INFO_OUTLINE, on_click=self._list_detail),
                ft.OutlinedButton("Device Tree", icon=ft.Icons.ACCOUNT_TREE_OUTLINED, on_click=self._list_tree),
                ft.OutlinedButton("List HID", icon=ft.Icons.USB, on_click=self._list_hid),
            ], wrap=True, spacing=8),

            ft.Divider(),

            # 单设备查询
            ft.Text("Query Single Device", size=32, weight=ft.FontWeight.W_600, color=ft.Colors.ON_SURFACE_VARIANT),
            ft.Row([self.dev_idx_field], spacing=10),
            ft.FilledButton("Get Device Detail", icon=ft.Icons.SEARCH, on_click=self._device_detail),

            ft.Divider(),

            # VID/PID 查找
            ft.Text("Find by VID:PID", size=32, weight=ft.FontWeight.W_600, color=ft.Colors.ON_SURFACE_VARIANT),
            ft.Row([self.vid_field, self.pid_field], spacing=10),
        ], spacing=8)

    # ---- HID 设备页 ----
    def _build_hid_tab(self):
        """构建 HID 设备页"""
        # 打开索引输入框
        self.hid_idx_field = ft.TextField(label="Index", width=180, value="0")
        # VID/PID 输入框
        self.hid_vid_field = ft.TextField(label="VID", width=150, value="0x")
        self.hid_pid_field = ft.TextField(label="PID", width=150, value="0x")

        return ft.Column([
            ft.Text("Open by Index", size=32, weight=ft.FontWeight.W_600, color=ft.Colors.ON_SURFACE_VARIANT),
            ft.Row([self.hid_idx_field, ft.FilledButton("Open", icon=ft.Icons.DOOR_FRONT_DOOR, on_click=self._open_hid_idx)], spacing=10),

            ft.Divider(),

            ft.Text("Open by VID:PID", size=32, weight=ft.FontWeight.W_600, color=ft.Colors.ON_SURFACE_VARIANT),
            ft.Row([self.hid_vid_field, self.hid_pid_field, ft.FilledButton("Open", icon=ft.Icons.DOOR_FRONT_DOOR, on_click=self._open_hid_vid_pid)], spacing=10),

            ft.Divider(),

            ft.Text("HID Control", size=32, weight=ft.FontWeight.W_600, color=ft.Colors.ON_SURFACE_VARIANT),
            ft.Row([
                ft.OutlinedButton("HID Info", icon=ft.Icons.INFO_OUTLINE, on_click=self._hid_info),
                ft.OutlinedButton("Close HID", icon=ft.Icons.DOOR_BACK_DOOR, on_click=self._close_hid, style=ft.ButtonStyle(color=ft.Colors.ERROR)),
            ], wrap=True, spacing=8),
        ], spacing=8)

    # ---- HID I/O 页 ----
    def _build_io_tab(self):
        """构建 HID I/O 页"""
        # 读取参数
        self.read_len_field = ft.TextField(label="Read Length", width=180, value="64")
        self.read_timeout_field = ft.TextField(label="Timeout (ms)", width=180, value="1000")
        # 写入数据
        self.write_data_field = ft.TextField(label="Write Data (hex)", value="00010203")
        # 特性报告
        self.report_id_field = ft.TextField(label="Report ID", width=120, value="0")
        self.feature_data_field = ft.TextField(label="Feature Data (hex)", value="000102")
        # 批量传输
        self.bulk_ep_field = ft.TextField(label="Endpoint (hex)", width=150, value="0x81")

        return ft.Column([
            # 读取区域卡片
            ft.Card(
                content=ft.Container(
                    content=ft.Column([
                        ft.Row([ft.Icon(ft.Icons.DOWNLOAD, color=ft.Colors.BLUE), ft.Text("Read Operations", size=30, weight=ft.FontWeight.W_600)], spacing=8),
                        ft.Row([self.read_len_field, self.read_timeout_field], spacing=10),
                        ft.Row([
                            ft.FilledButton("HID Read", icon=ft.Icons.DOWNLOAD, on_click=self._hid_read),
                            ft.FilledButton("Read Latest", icon=ft.Icons.UPDATE, on_click=self._hid_read_latest),
                        ], wrap=True, spacing=8),
                    ], spacing=8),
                    padding=12,
                ),
                elevation=0,
            ),

            # 写入区域卡片
            ft.Card(
                content=ft.Container(
                    content=ft.Column([
                        ft.Row([ft.Icon(ft.Icons.UPLOAD, color=ft.Colors.GREEN), ft.Text("Write Operation", size=30, weight=ft.FontWeight.W_600)], spacing=8),
                        self.write_data_field,
                        ft.FilledButton("HID Write", icon=ft.Icons.UPLOAD, on_click=self._hid_write),
                    ], spacing=8),
                    padding=12,
                ),
                elevation=0,
            ),

            # 特性报告卡片
            ft.Card(
                content=ft.Container(
                    content=ft.Column([
                        ft.Row([ft.Icon(ft.Icons.SETTINGS, color=ft.Colors.ORANGE), ft.Text("Feature Report", size=30, weight=ft.FontWeight.W_600)], spacing=8),
                        ft.Row([self.report_id_field], spacing=10),
                        ft.Row([
                            ft.FilledButton("Get Feature", icon=ft.Icons.DOWNLOAD, on_click=self._hid_get_feature),
                        ]),
                        self.feature_data_field,
                        ft.FilledButton("Send Feature", icon=ft.Icons.UPLOAD, on_click=self._hid_send_feature),
                    ], spacing=8),
                    padding=12,
                ),
                elevation=0,
            ),

            # 批量传输卡片
            ft.Card(
                content=ft.Container(
                    content=ft.Column([
                        ft.Row([ft.Icon(ft.Icons.STORAGE, color=ft.Colors.PURPLE), ft.Text("Bulk Transfer", size=30, weight=ft.FontWeight.W_600)], spacing=8),
                        ft.Row([self.bulk_ep_field], spacing=10),
                        ft.Row([
                            ft.FilledButton("Bulk Read", icon=ft.Icons.DOWNLOAD, on_click=self._bulk_read),
                            ft.FilledButton("Bulk Write", icon=ft.Icons.UPLOAD, on_click=self._bulk_write),
                        ], wrap=True, spacing=8),
                    ], spacing=8),
                    padding=12,
                ),
                elevation=0,
            ),
        ], spacing=12, scroll=ft.ScrollMode.AUTO)

    # ---- 异步传输页 ----
    def _build_async_tab(self):
        """构建异步传输页"""
        # 异步参数
        self.async_len_field = ft.TextField(label="Read Length", width=180, value="64")
        self.async_timeout_field = ft.TextField(label="Timeout (ms)", width=180, value="2000")

        # 状态显示
        self.async_status_text = ft.Text("Status: Idle", size=28, color=ft.Colors.GREY)
        self.async_count_text = ft.Text("Reads: 0", size=28)
        self.async_pending_text = ft.Text("Pending: 0", size=28)

        # 按钮
        self.btn_async_start = ft.FilledButton(
            "Start Continuous Read",
            icon=ft.Icons.PLAY_ARROW,
            style=ft.ButtonStyle(bgcolor=ft.Colors.GREEN),
            on_click=self._start_async,
        )
        self.btn_async_stop = ft.OutlinedButton(
            "Stop",
            icon=ft.Icons.STOP,
            style=ft.ButtonStyle(color=ft.Colors.ERROR),
            on_click=self._stop_async,
            disabled=True,
        )

        return ft.Column([
            ft.Card(
                content=ft.Container(
                    content=ft.Column([
                        ft.Row([ft.Icon(ft.Icons.TIMELAPSE, color=ft.Colors.CYAN), ft.Text("Async Settings", size=30, weight=ft.FontWeight.W_600)], spacing=8),
                        ft.Row([self.async_len_field, self.async_timeout_field], spacing=10),
                        ft.Row([self.btn_async_start, self.btn_async_stop], spacing=10),
                        ft.Divider(),
                        ft.Row([self.async_status_text, ft.Container(width=20), self.async_count_text, ft.Container(width=20), self.async_pending_text], alignment=ft.MainAxisAlignment.START, spacing=10),
                    ], spacing=8),
                    padding=12,
                ),
                elevation=0,
            ),
        ], spacing=8)

    # ---- 热插拔页 ----
    def _build_hotplug_tab(self):
        """构建热插拔页"""
        supported = bool(lib.usb_ctrl_is_hotplug_supported())
        
        # 支持状态标签
        support_color = ft.Colors.GREEN if supported else ft.Colors.RED
        support_text = "Supported" if supported else "Not Supported"

        # 热插拔事件日志
        self.hotplug_log = ft.TextField(
            multiline=True,
            min_lines=6,
            max_lines=10,
            read_only=True,
            value="",
            text_size=26,
            text_style=ft.TextStyle(font_family="Consolas"),
            hint_text="Hotplug events...",
        )

        # 按钮
        self.btn_hotplug_start = ft.FilledButton(
            "Start Monitoring",
            icon=ft.Icons.RADAR,
            on_click=self._start_hotplug,
        )
        self.btn_hotplug_stop = ft.OutlinedButton(
            "Stop Monitoring",
            icon=ft.Icons.STOP,
            on_click=self._stop_hotplug,
            disabled=True,
        )

        # 状态文本
        self.hotplug_status_text = ft.Text("Status: Idle", size=28, color=ft.Colors.GREY)

        return ft.Column([
            ft.Card(
                content=ft.Container(
                    content=ft.Column([
                        ft.Row([ft.Icon(ft.Icons.CABLE, color=support_color), ft.Text(support_text, size=30, weight=ft.FontWeight.W_600, color=support_color)], spacing=8),
                        ft.Divider(),
                        ft.Row([self.btn_hotplug_start, self.btn_hotplug_stop], spacing=10),
                        self.hotplug_status_text,
                    ], spacing=8),
                    padding=12,
                ),
                elevation=0,
            ),
            ft.Divider(),
            ft.Text("Event Log:", size=30, weight=ft.FontWeight.W_600, color=ft.Colors.ON_SURFACE_VARIANT),
            self.hotplug_log,
        ], spacing=8)

    # ========================================================================
    # 输出辅助
    # ========================================================================
    def _on_log_scroll(self, e):
        """监听日志区域滚动，动态切换 auto_scroll"""
        # 滚动位置接近底部（容差 50px）时开启自动滚动，否则关闭
        at_bottom = e.pixels >= e.max_scroll_extent - 50
        self.output_list.auto_scroll = at_bottom
        self._log_at_bottom = at_bottom

    def _log(self, text):
        """向输出区域追加文本（智能滚动：底部时自动滚动，上翻时不滚动）"""
        timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        line = ft.Text(
            f"[{timestamp}] {text}",
            size=26,
            font_family="Consolas",
            selectable=True,
        )
        self.output_list.controls.append(line)
        # 限制最大行数，避免内存问题
        if len(self.output_list.controls) > 2000:
            self.output_list.controls = self.output_list.controls[-1500:]
        self.update()

    def _show_snack(self, msg):
        """显示 SnackBar 提示"""
        snack = ft.SnackBar(content=ft.Text(msg), duration=2000)
        self.page.overlay.append(snack)
        snack.open = True
        self.page.update()

    def _update_status(self):
        """更新状态标签"""
        count = lib.usb_ctrl_device_count(self.handle)
        self.device_count_text.value = str(count)
        is_open = bool(lib.usb_ctrl_is_hid_open(self.handle))
        self.hid_status_text.value = "Opened" if is_open else "Closed"
        self.hid_status_text.color = ft.Colors.GREEN if is_open else ft.Colors.RED
        self.update()

    # ========================================================================
    # 设备枚举操作
    # ========================================================================
    def _refresh_devices(self, e):
        """刷新设备列表"""
        lib.usb_ctrl_refresh(self.handle)
        self._update_status()
        self._log("Devices refreshed")

    def _list_devices(self, e):
        """列出所有设备"""
        s = call_str(lib.usb_ctrl_list_devices, self.handle)
        self._log(s if s else "No devices found")

    def _list_detail(self, e):
        """列出设备详细信息"""
        s = call_str(lib.usb_ctrl_list_devices_detail, self.handle)
        self._log(s if s else "No devices found")

    def _list_tree(self, e):
        """列出设备树"""
        s = call_str(lib.usb_ctrl_list_devices_tree, self.handle)
        self._log(s if s else "No devices found")

    def _list_hid(self, e):
        """列出 HID 设备"""
        s = call_str(lib.usb_ctrl_list_hid_devices, self.handle)
        self._log(s if s else "No HID devices found")

    def _device_detail(self, e):
        """获取单个设备详情"""
        try:
            idx = int(self.dev_idx_field.value)
        except ValueError:
            self._log("ERROR: Invalid device index")
            return
        s = call_str(lib.usb_ctrl_device_detail, self.handle, idx)
        self._log(s if s else f"No device at index {idx}")

    # ========================================================================
    # HID 设备操作
    # ========================================================================
    def _open_hid_idx(self, e):
        """按索引打开 HID 设备"""
        try:
            idx = int(self.hid_idx_field.value)
        except ValueError:
            self._log("ERROR: Invalid index")
            return
        ok = lib.usb_ctrl_open_hid(self.handle, idx)
        if ok:
            info = call_str(lib.usb_ctrl_hid_info, self.handle)
            self._log(f"HID opened: {info}" if info else "HID opened")
        else:
            self._log("Failed to open HID device")
        self._update_status()

    def _open_hid_vid_pid(self, e):
        """按 VID:PID 打开 HID 设备"""
        try:
            vid = int(self.hid_vid_field.value, 16)
            pid = int(self.hid_pid_field.value, 16)
        except ValueError:
            self._log("ERROR: Invalid VID/PID")
            return
        ok = lib.usb_ctrl_open_hid_vid_pid(self.handle, vid, pid)
        if ok:
            info = call_str(lib.usb_ctrl_hid_info, self.handle)
            self._log(f"HID opened: {info}" if info else "HID opened")
        else:
            self._log("Failed to open HID device by VID:PID")
        self._update_status()

    def _close_hid(self, e):
        """关闭 HID 设备"""
        self._stop_async(None)
        lib.usb_ctrl_close_hid(self.handle)
        self._log("HID device closed")
        self._update_status()

    def _hid_info(self, e):
        """获取 HID 信息"""
        info = call_str(lib.usb_ctrl_hid_info, self.handle)
        self._log(f"HID Info: {info}" if info else "No HID device opened")

    # ========================================================================
    # HID I/O 操作
    # ========================================================================
    def _hid_read(self, e):
        """同步 HID 读取"""
        try:
            length = int(self.read_len_field.value)
            timeout = int(self.read_timeout_field.value)
        except ValueError:
            self._log("ERROR: Invalid length/timeout")
            return
        result = call_json(lib.usb_ctrl_hid_read, self.handle, length, timeout)
        self._log(f"HID Read:\n{format_result(result)}")

    def _hid_read_latest(self, e):
        """读取最新 HID 报告"""
        try:
            length = int(self.read_len_field.value)
        except ValueError:
            self._log("ERROR: Invalid length")
            return
        result = call_json(lib.usb_ctrl_hid_read_latest, self.handle, length, 10)
        self._log(f"HID ReadLatest:\n{format_result(result)}")

    def _hid_write(self, e):
        """同步 HID 写入"""
        hex_str = self.write_data_field.value.strip()
        if not hex_str:
            self._log("ERROR: No data")
            return
        try:
            arr, length = hex_to_bytes(hex_str)
        except ValueError:
            self._log("ERROR: Invalid hex data")
            return
        try:
            timeout = int(self.read_timeout_field.value)
        except ValueError:
            timeout = 1000
        result = call_json(lib.usb_ctrl_hid_write, self.handle, arr, length, timeout)
        self._log(f"HID Write:\n{format_result(result)}")

    def _hid_get_feature(self, e):
        """获取特性报告"""
        try:
            report_id = int(self.report_id_field.value, 16)
            length = int(self.read_len_field.value)
        except ValueError:
            self._log("ERROR: Invalid report_id/length")
            return
        result = call_json(lib.usb_ctrl_hid_get_feature, self.handle, report_id, length, 1000)
        self._log(f"GetFeature:\n{format_result(result)}")

    def _hid_send_feature(self, e):
        """发送特性报告"""
        hex_str = self.feature_data_field.value.strip()
        if not hex_str:
            self._log("ERROR: No data")
            return
        try:
            arr, length = hex_to_bytes(hex_str)
        except ValueError:
            self._log("ERROR: Invalid hex data")
            return
        result = call_json(lib.usb_ctrl_hid_send_feature, self.handle, arr, length, 1000)
        self._log(f"SendFeature:\n{format_result(result)}")

    # ========================================================================
    # 批量传输
    # ========================================================================
    def _bulk_read(self, e):
        """批量读取"""
        try:
            ep = int(self.bulk_ep_field.value, 16)
            length = int(self.read_len_field.value)
        except ValueError:
            self._log("ERROR: Invalid endpoint/length")
            return
        result = call_json(lib.usb_ctrl_bulk_read, self.handle, ep, length, 2000)
        self._log(f"BulkRead:\n{format_result(result)}")

    def _bulk_write(self, e):
        """批量写入"""
        try:
            ep = int(self.bulk_ep_field.value, 16)
        except ValueError:
            self._log("ERROR: Invalid endpoint")
            return
        hex_str = self.write_data_field.value.strip()
        if not hex_str:
            self._log("ERROR: No data")
            return
        try:
            arr, length = hex_to_bytes(hex_str)
        except ValueError:
            self._log("ERROR: Invalid hex data")
            return
        result = call_json(lib.usb_ctrl_bulk_write, self.handle, ep, arr, length, 2000)
        self._log(f"BulkWrite:\n{format_result(result)}")

    # ========================================================================
    # 异步传输
    # ========================================================================
    def _start_async(self, e):
        """启动连续异步读取"""
        if not lib.usb_ctrl_is_hid_open(self.handle):
            self._log("ERROR: No HID device opened")
            return
        try:
            length = int(self.async_len_field.value)
            timeout = int(self.async_timeout_field.value)
        except ValueError:
            self._log("ERROR: Invalid length/timeout")
            return

        lib.usb_ctrl_async_start(self.handle)
        lib.usb_ctrl_async_read_continuous(self.handle, length, timeout)
        self.async_running = True
        self.async_count = 0

        self.btn_async_start.disabled = True
        self.btn_async_stop.disabled = False
        self.async_status_text.value = "Status: Running"
        self.async_status_text.color = ft.Colors.GREEN
        self.update()

        self._poll_async()

    def _stop_async(self, e):
        """停止连续异步读取"""
        if not self.async_running:
            return
        self.async_running = False
        lib.usb_ctrl_async_stop_continuous(self.handle)
        lib.usb_ctrl_async_stop(self.handle)

        self.btn_async_start.disabled = False
        self.btn_async_stop.disabled = True
        self.async_status_text.value = "Status: Idle"
        self.async_status_text.color = ft.Colors.GREY
        self.update()

    def _poll_async(self):
        """轮询异步读取结果"""
        if not self.async_running:
            return

        while True:
            ptr = lib.usb_ctrl_async_poll(self.handle)
            if not ptr:
                break
            result = ctypes.cast(ptr, ctypes.c_char_p).value
            s = result.decode("utf-8", errors="replace") if result else ""
            lib.usb_ctrl_free_str(ptr)
            self.async_count += 1
            self.async_count_text.value = f"Reads: {self.async_count}"
            try:
                data = json.loads(s)
                hex_data = data.get("data_hex", "")
                ascii_data = data.get("data_ascii", "")
                bytes_count = data.get("bytes", 0)
                self._log(f"[Async #{self.async_count}] {bytes_count} bytes")
                self._log(f"  HEX  : {hex_data}")
                self._log(f"  ASCII: {ascii_data}")
            except json.JSONDecodeError:
                self._log(f"[Async] {s}")

        pending = lib.usb_ctrl_async_pending(self.handle)
        self.async_pending_text.value = f"Pending: {pending}"
        self.update()

        if self.async_running:
            self.page.run_task(self._async_poll_task)

    async def _async_poll_task(self):
        """异步轮询任务"""
        import asyncio
        await asyncio.sleep(0.1)  # 100ms 间隔
        self._poll_async()

    # ========================================================================
    # 热插拔
    # ========================================================================
    def _start_hotplug(self, e):
        """启动热插拔监听"""
        if not lib.usb_ctrl_is_hotplug_supported():
            self._log("Hotplug not supported on this platform")
            return
        ok = lib.usb_ctrl_hotplug_start(self.handle)
        if not ok:
            self._log("Failed to start hotplug monitoring")
            return
        self.hotplug_running = True
        self.btn_hotplug_start.disabled = True
        self.btn_hotplug_stop.disabled = False
        self.hotplug_status_text.value = "Status: Monitoring"
        self.hotplug_status_text.color = ft.Colors.GREEN
        self._log("Hotplug monitoring started")
        self.update()
        self._poll_hotplug()

    def _stop_hotplug(self, e):
        """停止热插拔监听"""
        if not self.hotplug_running:
            return
        self.hotplug_running = False
        lib.usb_ctrl_hotplug_stop(self.handle)
        self.btn_hotplug_start.disabled = False
        self.btn_hotplug_stop.disabled = True
        self.hotplug_status_text.value = "Status: Idle"
        self.hotplug_status_text.color = ft.Colors.GREY
        self._log("Hotplug monitoring stopped")
        self.update()

    def _poll_hotplug(self):
        """轮询热插拔事件"""
        if not self.hotplug_running:
            return

        while True:
            ptr = lib.usb_ctrl_hotplug_poll(self.handle)
            if not ptr:
                break
            result = ctypes.cast(ptr, ctypes.c_char_p).value
            s = result.decode("utf-8", errors="replace") if result else ""
            lib.usb_ctrl_free_str(ptr)
            try:
                data = json.loads(s)
                event = data.get("event", "unknown")
                summary = data.get("summary", "")
                symbol = "[+]" if event == "arrived" else "[-]"
                line = f"{symbol} {event}: {summary}\n"
                current = self.hotplug_log.value or ""
                self.hotplug_log.value = current + line
                self._log(f"Hotplug {symbol} {summary}")
            except json.JSONDecodeError:
                self._log(f"Hotplug: {s}")

        self.update()

        if self.hotplug_running:
            self.page.run_task(self._hotplug_poll_task)

    async def _hotplug_poll_task(self):
        """热插拔轮询任务"""
        import asyncio
        await asyncio.sleep(0.2)  # 200ms 间隔
        self._poll_hotplug()


# ============================================================================
# 入口
# ============================================================================
def main(page: ft.Page):
    """主入口函数"""
    page.title = "USB Controller Test"
    page.window.width = 1400
    page.window.height = 900
    page.window.min_width = 1100
    page.window.min_height = 700
    page.padding = 0
    page.bgcolor = ft.Colors.SURFACE
    # 全局默认字体大小（按钮、输入框、菜单等未指定 size 的控件）
    page.text_size = 26
    
    app = UsbTestApp()
    
    page.add(app)
    
    # 添加到 page 后再刷新设备列表
    app._refresh_devices(None)


if __name__ == "__main__":
    ft.run(main)
