#!/usr/bin/env python3
# ============================================================================
# usb_test_ui.py - USB 控制器集成测试 UI
#
# 功能说明：
#   使用 tkinter 构建图形界面，通过 ctypes 调用 C API 共享库，
#   整合所有 USB 控制器功能，提供可视化操作界面。
#
# 功能模块：
#   1. 设备枚举：列出/刷新/搜索设备
#   2. HID 设备：打开/关闭/信息查询
#   3. HID I/O：同步读写、特性报告
#   4. 异步传输：连续读取、轮询结果
#   5. 批量传输：指定端点读写
#   6. 热插拔：监听设备插拔事件
# ============================================================================

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import ctypes
import ctypes.util
import json
import os
import sys
import platform

# ============================================================================
# 查找并加载共享库
# ============================================================================
def find_lib():
    """查找共享库（跨平台，自动搜索所有构建类型目录）"""
    # 根据平台确定库文件名（与 CMakeLists.txt 中 PREFIX "lib" 一致）
    if platform.system() == "Windows":
        lib_name = "libusb_ctrl_capi.dll"  # CMake 设置了 PREFIX "lib"
    elif platform.system() == "Darwin":
        lib_name = "libusb_ctrl_capi.dylib"
    else:
        lib_name = "libusb_ctrl_capi.so"

    # 获取脚本所在目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # 项目根目录（test/ -> usb_controller_fun3/ -> examples/ -> KRAY/）
    project_root = os.path.abspath(os.path.join(script_dir, "..", "..", ".."))
    # 构建根目录
    build_root = os.path.join(project_root, "build")

    # 构建类型目录名（单配置生成器如 MinGW）
    cfg_dirs = ["Debug", "Release", "RelWithDebInfo", "MinSizeRel", ""]
    # 共享库子目录（与 CMakeLists.txt 中 CMAKE_LIBRARY_OUTPUT_DIRECTORY 对应）
    lib_subdirs = ["lib/dynamic", "lib/shared", "lib", "bin", ""]
    # 构建目标子目录（build.bat usb 输出到 build/usb/）
    target_dirs = ["usb", "main", ""]

    # 候选路径：脚本同目录优先
    candidates = [os.path.join(script_dir, lib_name)]

    # 遍历所有组合：build/{target}/{cfg}/{lib_sub}/lib_name
    for target in target_dirs:
        for cfg in cfg_dirs:
            for lib_sub in lib_subdirs:
                candidates.append(os.path.join(build_root, target, cfg, lib_sub, lib_name))

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
# 主窗口类
# ============================================================================
class UsbTestApp:
    """USB 控制器集成测试 UI"""

    def __init__(self, root):
        self.root = root
        self.root.title("USB Controller - Integrated Test UI")

        # 获取屏幕大小
        screen_width = self.root.winfo_screenwidth()
        screen_height = self.root.winfo_screenheight()

        w = 1600
        h = 800
        # 初始窗口大小，居中显示
        self.root.geometry(f"{w}x{h}+{screen_width//2-w//2}+{screen_height//2-h//2}")
        # 最小窗口大小
        self.root.minsize(w, h) 

        # 创建控制器
        self.handle = lib.usb_ctrl_create()
        if not self.handle:
            messagebox.showerror("Error", "Failed to create USB controller")
            sys.exit(1)

        # 异步轮询状态
        self.async_running = False
        self.hotplug_running = False

        # 构建 UI
        self._build_ui()

        # 初始刷新
        self._refresh_devices()

        # 关闭窗口时清理
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    def _on_close(self):
        """窗口关闭时清理资源"""
        self._stop_async()
        self._stop_hotplug()
        if self.handle:
            lib.usb_ctrl_destroy(self.handle)
            self.handle = None
        self.root.destroy()

    # ========================================================================
    # UI 构建
    # ========================================================================
    def _build_ui(self):
        """构建主界面"""
        # 顶部工具栏
        toolbar = ttk.Frame(self.root)
        toolbar.pack(fill=tk.X, padx=5, pady=3)

        ttk.Button(toolbar, text="Refresh", command=self._refresh_devices).pack(side=tk.LEFT, padx=2)
        ttk.Label(toolbar, text="  Device Count:").pack(side=tk.LEFT)
        self.lbl_count = ttk.Label(toolbar, text="0")
        self.lbl_count.pack(side=tk.LEFT)
        ttk.Label(toolbar, text="  HID:").pack(side=tk.LEFT)
        self.lbl_hid = ttk.Label(toolbar, text="Closed", foreground="red")
        self.lbl_hid.pack(side=tk.LEFT)

        # 主区域：左侧控制面板 + 右侧输出
        main = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main.pack(fill=tk.BOTH, expand=True, padx=5, pady=3)

        # 左侧：功能面板（使用 Notebook 分页）
        left = ttk.Frame(main, width=320)
        main.add(left, weight=0)

        self.notebook = ttk.Notebook(left)
        self.notebook.pack(fill=tk.BOTH, expand=True)

        self._build_device_tab()
        self._build_hid_tab()
        self._build_io_tab()
        self._build_async_tab()
        self._build_hotplug_tab()

        # 右侧：输出区域
        right = ttk.Frame(main)
        main.add(right, weight=1)

        ttk.Label(right, text="Output:").pack(anchor=tk.W)
        self.output = scrolledtext.ScrolledText(right, wrap=tk.WORD, font=("Monospace", 9))
        self.output.pack(fill=tk.BOTH, expand=True)
        self.output.config(state=tk.DISABLED)

    # ---- 设备枚举页 ----
    def _build_device_tab(self):
        """设备枚举页"""
        frame = ttk.Frame(self.notebook, padding=5)
        self.notebook.add(frame, text="Device")

        ttk.Button(frame, text="List All Devices", command=self._list_devices).pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="List Devices Detail", command=self._list_detail).pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="Device Tree", command=self._list_tree).pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="List HID Devices", command=self._list_hid).pack(fill=tk.X, pady=2)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        ttk.Label(frame, text="Device Index:").pack(anchor=tk.W)
        self.ent_dev_idx = ttk.Entry(frame, width=10)
        self.ent_dev_idx.insert(0, "0")
        self.ent_dev_idx.pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="Device Detail", command=self._device_detail).pack(fill=tk.X, pady=2)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        ttk.Label(frame, text="Find by VID:PID").pack(anchor=tk.W)
        row = ttk.Frame(frame)
        row.pack(fill=tk.X, pady=2)
        ttk.Label(row, text="VID:").pack(side=tk.LEFT)
        self.ent_vid = ttk.Entry(row, width=6)
        self.ent_vid.insert(0, "0x")
        self.ent_vid.pack(side=tk.LEFT, padx=2)
        ttk.Label(row, text="PID:").pack(side=tk.LEFT)
        self.ent_pid = ttk.Entry(row, width=6)
        self.ent_pid.insert(0, "0x")
        self.ent_pid.pack(side=tk.LEFT, padx=2)

    # ---- HID 设备页 ----
    def _build_hid_tab(self):
        """HID 设备页"""
        frame = ttk.Frame(self.notebook, padding=5)
        self.notebook.add(frame, text="HID")

        ttk.Label(frame, text="Open by Index:").pack(anchor=tk.W)
        row1 = ttk.Frame(frame)
        row1.pack(fill=tk.X, pady=2)
        self.ent_hid_idx = ttk.Entry(row1, width=10)
        self.ent_hid_idx.insert(0, "0")
        self.ent_hid_idx.pack(side=tk.LEFT, padx=2)
        ttk.Button(row1, text="Open", command=self._open_hid_idx).pack(side=tk.LEFT, padx=2)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        ttk.Label(frame, text="Open by VID:PID:").pack(anchor=tk.W)
        row2 = ttk.Frame(frame)
        row2.pack(fill=tk.X, pady=2)
        ttk.Label(row2, text="VID:").pack(side=tk.LEFT)
        self.ent_hid_vid = ttk.Entry(row2, width=6)
        self.ent_hid_vid.insert(0, "0x")
        self.ent_hid_vid.pack(side=tk.LEFT, padx=2)
        ttk.Label(row2, text="PID:").pack(side=tk.LEFT)
        self.ent_hid_pid = ttk.Entry(row2, width=6)
        self.ent_hid_pid.insert(0, "0x")
        self.ent_hid_pid.pack(side=tk.LEFT, padx=2)
        ttk.Button(row2, text="Open", command=self._open_hid_vid_pid).pack(side=tk.LEFT, padx=2)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        ttk.Button(frame, text="HID Info", command=self._hid_info).pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="Close HID", command=self._close_hid).pack(fill=tk.X, pady=2)

    # ---- HID I/O 页 ----
    def _build_io_tab(self):
        """HID I/O 页"""
        frame = ttk.Frame(self.notebook, padding=5)
        self.notebook.add(frame, text="I/O")

        # 读取
        ttk.Label(frame, text="Read Length:").pack(anchor=tk.W)
        self.ent_read_len = ttk.Entry(frame, width=10)
        self.ent_read_len.insert(0, "64")
        self.ent_read_len.pack(fill=tk.X, pady=2)

        ttk.Label(frame, text="Timeout (ms):").pack(anchor=tk.W)
        self.ent_read_timeout = ttk.Entry(frame, width=10)
        self.ent_read_timeout.insert(0, "1000")
        self.ent_read_timeout.pack(fill=tk.X, pady=2)

        ttk.Button(frame, text="HID Read", command=self._hid_read).pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="HID Read Latest", command=self._hid_read_latest).pack(fill=tk.X, pady=2)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        # 写入
        ttk.Label(frame, text="Write Data (hex):").pack(anchor=tk.W)
        self.ent_write_data = ttk.Entry(frame)
        self.ent_write_data.insert(0, "00010203")
        self.ent_write_data.pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="HID Write", command=self._hid_write).pack(fill=tk.X, pady=2)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        # 特性报告
        ttk.Label(frame, text="Feature Report ID (hex):").pack(anchor=tk.W)
        self.ent_report_id = ttk.Entry(frame, width=6)
        self.ent_report_id.insert(0, "0")
        self.ent_report_id.pack(fill=tk.X, pady=2)

        ttk.Button(frame, text="Get Feature", command=self._hid_get_feature).pack(fill=tk.X, pady=2)

        ttk.Label(frame, text="Send Feature Data (hex):").pack(anchor=tk.W)
        self.ent_feature_data = ttk.Entry(frame)
        self.ent_feature_data.insert(0, "000102")
        self.ent_feature_data.pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="Send Feature", command=self._hid_send_feature).pack(fill=tk.X, pady=2)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        # 批量传输
        ttk.Label(frame, text="Bulk Endpoint (hex):").pack(anchor=tk.W)
        self.ent_bulk_ep = ttk.Entry(frame, width=6)
        self.ent_bulk_ep.insert(0, "0x81")
        self.ent_bulk_ep.pack(fill=tk.X, pady=2)

        ttk.Button(frame, text="Bulk Read", command=self._bulk_read).pack(fill=tk.X, pady=2)
        ttk.Button(frame, text="Bulk Write", command=self._bulk_write).pack(fill=tk.X, pady=2)

    # ---- 异步传输页 ----
    def _build_async_tab(self):
        """异步传输页"""
        frame = ttk.Frame(self.notebook, padding=5)
        self.notebook.add(frame, text="Async")

        ttk.Label(frame, text="Read Length:").pack(anchor=tk.W)
        self.ent_async_len = ttk.Entry(frame, width=10)
        self.ent_async_len.insert(0, "64")
        self.ent_async_len.pack(fill=tk.X, pady=2)

        ttk.Label(frame, text="Timeout (ms):").pack(anchor=tk.W)
        self.ent_async_timeout = ttk.Entry(frame, width=10)
        self.ent_async_timeout.insert(0, "2000")
        self.ent_async_timeout.pack(fill=tk.X, pady=2)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        self.btn_async_start = ttk.Button(frame, text="Start Continuous Read", command=self._start_async)
        self.btn_async_start.pack(fill=tk.X, pady=2)

        self.btn_async_stop = ttk.Button(frame, text="Stop Continuous Read", command=self._stop_async, state=tk.DISABLED)
        self.btn_async_stop.pack(fill=tk.X, pady=2)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        self.lbl_async_status = ttk.Label(frame, text="Status: Idle")
        self.lbl_async_status.pack(anchor=tk.W)
        self.lbl_async_count = ttk.Label(frame, text="Reads: 0")
        self.lbl_async_count.pack(anchor=tk.W)
        self.lbl_async_pending = ttk.Label(frame, text="Pending: 0")
        self.lbl_async_pending.pack(anchor=tk.W)

    # ---- 热插拔页 ----
    def _build_hotplug_tab(self):
        """热插拔页"""
        frame = ttk.Frame(self.notebook, padding=5)
        self.notebook.add(frame, text="Hotplug")

        supported = lib.usb_ctrl_is_hotplug_supported()
        self.lbl_hotplug_support = ttk.Label(
            frame,
            text=f"Supported: {'Yes' if supported else 'No'}",
            foreground="green" if supported else "red"
        )
        self.lbl_hotplug_support.pack(anchor=tk.W, pady=5)

        self.btn_hotplug_start = ttk.Button(frame, text="Start Monitoring", command=self._start_hotplug)
        self.btn_hotplug_start.pack(fill=tk.X, pady=2)

        self.btn_hotplug_stop = ttk.Button(frame, text="Stop Monitoring", command=self._stop_hotplug, state=tk.DISABLED)
        self.btn_hotplug_stop.pack(fill=tk.X, pady=2)

        self.lbl_hotplug_status = ttk.Label(frame, text="Status: Idle")
        self.lbl_hotplug_status.pack(anchor=tk.W, pady=5)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)

        ttk.Label(frame, text="Hotplug Events:").pack(anchor=tk.W)
        self.hotplug_log = scrolledtext.ScrolledText(frame, height=8, font=("Monospace", 9))
        self.hotplug_log.pack(fill=tk.BOTH, expand=True, pady=2)

    # ========================================================================
    # 输出辅助
    # ========================================================================
    def _log(self, text):
        """向输出区域追加文本"""
        self.output.config(state=tk.NORMAL)
        self.output.insert(tk.END, text + "\n")
        self.output.see(tk.END)
        self.output.config(state=tk.DISABLED)

    def _update_status(self):
        """更新状态标签"""
        count = lib.usb_ctrl_device_count(self.handle)
        self.lbl_count.config(text=str(count))
        is_open = lib.usb_ctrl_is_hid_open(self.handle)
        self.lbl_hid.config(
            text="Opened" if is_open else "Closed",
            foreground="green" if is_open else "red"
        )

    # ========================================================================
    # 设备枚举操作
    # ========================================================================
    def _refresh_devices(self):
        """刷新设备列表"""
        lib.usb_ctrl_refresh(self.handle)
        self._update_status()
        self._log("Devices refreshed")

    def _list_devices(self):
        """列出所有设备"""
        s = call_str(lib.usb_ctrl_list_devices, self.handle)
        self._log(s if s else "(empty)")

    def _list_detail(self):
        """列出设备详情"""
        s = call_str(lib.usb_ctrl_list_devices_detail, self.handle)
        self._log(s if s else "(empty)")

    def _list_tree(self):
        """列出设备树"""
        s = call_str(lib.usb_ctrl_list_devices_tree, self.handle)
        self._log(s if s else "(empty)")

    def _list_hid(self):
        """列出 HID 设备"""
        s = call_str(lib.usb_ctrl_list_hid_devices, self.handle)
        self._log(s if s else "(no HID devices)")

    def _device_detail(self):
        """获取设备详情"""
        try:
            idx = int(self.ent_dev_idx.get())
        except ValueError:
            self._log("ERROR: Invalid index")
            return
        s = call_str(lib.usb_ctrl_device_detail, self.handle, idx)
        self._log(s if s else "(not found)")

    # ========================================================================
    # HID 设备操作
    # ========================================================================
    def _open_hid_idx(self):
        """按索引打开 HID"""
        try:
            idx = int(self.ent_hid_idx.get())
        except ValueError:
            self._log("ERROR: Invalid index")
            return
        ok = lib.usb_ctrl_open_hid(self.handle, idx)
        if ok:
            info = call_str(lib.usb_ctrl_hid_info, self.handle)
            self._log(f"HID opened: {info}")
        else:
            self._log("Failed to open HID device")
        self._update_status()

    def _open_hid_vid_pid(self):
        """按 VID:PID 打开 HID"""
        try:
            vid = int(self.ent_hid_vid.get(), 16)
            pid = int(self.ent_hid_pid.get(), 16)
        except ValueError:
            self._log("ERROR: Invalid VID/PID")
            return
        ok = lib.usb_ctrl_open_hid_vid_pid(self.handle, vid, pid)
        if ok:
            info = call_str(lib.usb_ctrl_hid_info, self.handle)
            self._log(f"HID opened: {info}")
        else:
            self._log("Failed to open HID device by VID:PID")
        self._update_status()

    def _close_hid(self):
        """关闭 HID 设备"""
        self._stop_async()
        lib.usb_ctrl_close_hid(self.handle)
        self._log("HID device closed")
        self._update_status()

    def _hid_info(self):
        """获取 HID 信息"""
        info = call_str(lib.usb_ctrl_hid_info, self.handle)
        self._log(f"HID Info: {info}" if info else "No HID device opened")

    # ========================================================================
    # HID I/O 操作
    # ========================================================================
    def _hid_read(self):
        """同步 HID 读取"""
        try:
            length = int(self.ent_read_len.get())
            timeout = int(self.ent_read_timeout.get())
        except ValueError:
            self._log("ERROR: Invalid length/timeout")
            return
        result = call_json(lib.usb_ctrl_hid_read, self.handle, length, timeout)
        self._log(f"HID Read: {format_result(result)}")

    def _hid_read_latest(self):
        """读取最新 HID 报告"""
        try:
            length = int(self.ent_read_len.get())
        except ValueError:
            self._log("ERROR: Invalid length")
            return
        result = call_json(lib.usb_ctrl_hid_read_latest, self.handle, length, 10)
        self._log(f"HID ReadLatest: {format_result(result)}")

    def _hid_write(self):
        """同步 HID 写入"""
        hex_str = self.ent_write_data.get().strip()
        if not hex_str:
            self._log("ERROR: No data")
            return
        try:
            arr, length = hex_to_bytes(hex_str)
        except ValueError:
            self._log("ERROR: Invalid hex data")
            return
        try:
            timeout = int(self.ent_read_timeout.get())
        except ValueError:
            timeout = 1000
        result = call_json(lib.usb_ctrl_hid_write, self.handle, arr, length, timeout)
        self._log(f"HID Write: {format_result(result)}")

    def _hid_get_feature(self):
        """获取特性报告"""
        try:
            report_id = int(self.ent_report_id.get(), 16)
            length = int(self.ent_read_len.get())
        except ValueError:
            self._log("ERROR: Invalid report_id/length")
            return
        result = call_json(lib.usb_ctrl_hid_get_feature, self.handle, report_id, length, 1000)
        self._log(f"GetFeature: {format_result(result)}")

    def _hid_send_feature(self):
        """发送特性报告"""
        hex_str = self.ent_feature_data.get().strip()
        if not hex_str:
            self._log("ERROR: No data")
            return
        try:
            arr, length = hex_to_bytes(hex_str)
        except ValueError:
            self._log("ERROR: Invalid hex data")
            return
        result = call_json(lib.usb_ctrl_hid_send_feature, self.handle, arr, length, 1000)
        self._log(f"SendFeature: {format_result(result)}")

    # ========================================================================
    # 批量传输
    # ========================================================================
    def _bulk_read(self):
        """批量读取"""
        try:
            ep = int(self.ent_bulk_ep.get(), 16)
            length = int(self.ent_read_len.get())
        except ValueError:
            self._log("ERROR: Invalid endpoint/length")
            return
        result = call_json(lib.usb_ctrl_bulk_read, self.handle, ep, length, 2000)
        self._log(f"BulkRead: {format_result(result)}")

    def _bulk_write(self):
        """批量写入"""
        try:
            ep = int(self.ent_bulk_ep.get(), 16)
        except ValueError:
            self._log("ERROR: Invalid endpoint")
            return
        hex_str = self.ent_write_data.get().strip()
        if not hex_str:
            self._log("ERROR: No data")
            return
        try:
            arr, length = hex_to_bytes(hex_str)
        except ValueError:
            self._log("ERROR: Invalid hex data")
            return
        result = call_json(lib.usb_ctrl_bulk_write, self.handle, ep, arr, length, 2000)
        self._log(f"BulkWrite: {format_result(result)}")

    # ========================================================================
    # 异步传输
    # ========================================================================
    def _start_async(self):
        """启动连续异步读取"""
        if not lib.usb_ctrl_is_hid_open(self.handle):
            self._log("ERROR: No HID device opened")
            return
        try:
            length = int(self.ent_async_len.get())
            timeout = int(self.ent_async_timeout.get())
        except ValueError:
            self._log("ERROR: Invalid length/timeout")
            return

        lib.usb_ctrl_async_start(self.handle)
        lib.usb_ctrl_async_read_continuous(self.handle, length, timeout)
        self.async_running = True
        self.async_count = 0

        self.btn_async_start.config(state=tk.DISABLED)
        self.btn_async_stop.config(state=tk.NORMAL)
        self.lbl_async_status.config(text="Status: Running", foreground="green")

        self._poll_async()

    def _stop_async(self):
        """停止连续异步读取"""
        if not self.async_running:
            return
        self.async_running = False
        lib.usb_ctrl_async_stop_continuous(self.handle)
        lib.usb_ctrl_async_stop(self.handle)

        self.btn_async_start.config(state=tk.NORMAL)
        self.btn_async_stop.config(state=tk.DISABLED)
        self.lbl_async_status.config(text="Status: Idle", foreground="black")

    def _poll_async(self):
        """轮询异步读取结果"""
        if not self.async_running:
            return

        # 轮询所有可用结果
        while True:
            ptr = lib.usb_ctrl_async_poll(self.handle)
            if not ptr:
                break
            result = ctypes.cast(ptr, ctypes.c_char_p).value
            s = result.decode("utf-8", errors="replace") if result else ""
            lib.usb_ctrl_free_str(ptr)
            self.async_count += 1
            self.lbl_async_count.config(text=f"Reads: {self.async_count}")
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

        # 更新 pending
        pending = lib.usb_ctrl_async_pending(self.handle)
        self.lbl_async_pending.config(text=f"Pending: {pending}")

        # 继续轮询（100ms 间隔）
        if self.async_running:
            self.root.after(100, self._poll_async)

    # ========================================================================
    # 热插拔
    # ========================================================================
    def _start_hotplug(self):
        """启动热插拔监听"""
        if not lib.usb_ctrl_is_hotplug_supported():
            self._log("Hotplug not supported on this platform")
            return
        ok = lib.usb_ctrl_hotplug_start(self.handle)
        if not ok:
            self._log("Failed to start hotplug monitoring")
            return
        self.hotplug_running = True
        self.btn_hotplug_start.config(state=tk.DISABLED)
        self.btn_hotplug_stop.config(state=tk.NORMAL)
        self.lbl_hotplug_status.config(text="Status: Monitoring", foreground="green")
        self._log("Hotplug monitoring started")
        self._poll_hotplug()

    def _stop_hotplug(self):
        """停止热插拔监听"""
        if not self.hotplug_running:
            return
        self.hotplug_running = False
        lib.usb_ctrl_hotplug_stop(self.handle)
        self.btn_hotplug_start.config(state=tk.NORMAL)
        self.btn_hotplug_stop.config(state=tk.DISABLED)
        self.lbl_hotplug_status.config(text="Status: Idle", foreground="black")
        self._log("Hotplug monitoring stopped")

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
                self.hotplug_log.config(state=tk.NORMAL)
                self.hotplug_log.insert(tk.END, line)
                self.hotplug_log.see(tk.END)
                self.hotplug_log.config(state=tk.DISABLED)
                self._log(f"Hotplug {symbol} {summary}")
            except json.JSONDecodeError:
                self._log(f"Hotplug: {s}")

        # 继续轮询（200ms 间隔）
        if self.hotplug_running:
            self.root.after(200, self._poll_hotplug)


# ============================================================================
# 入口
# ============================================================================
def main():
    root = tk.Tk()
    app = UsbTestApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
