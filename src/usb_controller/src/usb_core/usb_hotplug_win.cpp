// ============================================================================
// usb_hotplug_win.cpp - USB 热插拔异步监听模块 - Windows 版本（实现文件）
//
// 功能说明：
//   实现 UsbHotplugWin 类，使用 Windows 原生设备通知机制。
//   参考 USBHotplugWindows 实现，适配到 usb_ctrl 命名空间。
//
// 核心流程：
//   1. start() 初始化设备快照，启动通知窗口线程
//   2. 通知线程创建隐藏窗口，注册 USB 设备通知
//   3. WM_DEVICECHANGE 消息触发 win_diff_devices()
//   4. diff 对比当前设备与上次快照，触发 Arrived/Left 回调
//   5. stop() 发送 WM_CLOSE 关闭窗口，等待线程退出
// ============================================================================

#ifdef _WIN32

#include "usb_hotplug_win.hpp"
#include "libusb.h"
#include <algorithm>
#include <iostream>

namespace usb_ctrl {
namespace core {

// ============================================================================
// UsbHotplugWin 构造函数
// ============================================================================
UsbHotplugWin::UsbHotplugWin() {}

// ============================================================================
// UsbHotplugWin 析构函数
// ============================================================================
UsbHotplugWin::~UsbHotplugWin() { stop(); }

// ============================================================================
// is_supported - Windows 始终支持热插拔
// ============================================================================
bool UsbHotplugWin::is_supported() { return true; }

// ============================================================================
// set_callback - 设置热插拔回调函数
// ============================================================================
void UsbHotplugWin::set_callback(HotplugCallback cb) { _callback = std::move(cb); }

// ============================================================================
// set_vid_filter - 设置 VID 过滤条件
// ============================================================================
void UsbHotplugWin::set_vid_filter(uint16_t vid) { _vid = vid; }

// ============================================================================
// set_pid_filter - 设置 PID 过滤条件
// ============================================================================
void UsbHotplugWin::set_pid_filter(uint16_t pid) { _pid = pid; }

// ============================================================================
// set_class_filter - 设置设备类过滤条件（暂未实现）
// ============================================================================
void UsbHotplugWin::set_class_filter(uint8_t cls) { _cls = cls; }

// ============================================================================
// start - 启动热插拔监听
//
// 1. 初始化设备快照
// 2. 启动通知窗口线程
// ============================================================================
bool UsbHotplugWin::start() {
    // 如果已在监听，直接返回成功
    if (is_listening()) return true;

    // 初始化设备快照
    {
        std::lock_guard<std::mutex> lock(_snapshot_mutex);
        _snapshot = _enum_devices();
    }

    // 启动通知窗口线程
    _running.store(true, std::memory_order_release);
    _thread = std::thread(&UsbHotplugWin::_win_notify_thread, this);
    return true;
}

// ============================================================================
// stop - 停止热插拔监听
//
// 发送 WM_CLOSE 关闭隐藏窗口，等待线程退出。
// ============================================================================
void UsbHotplugWin::stop() {
    // 停止通知线程
    _running.store(false, std::memory_order_release);

    // 发送 WM_CLOSE 关闭隐藏窗口
    if (_hwnd) {
        PostMessage(_hwnd, WM_CLOSE, 0, 0);
    }

    // 等待线程结束
    if (_thread.joinable()) {
        _thread.join();
    }

    _hwnd = nullptr;

    // 清空快照
    {
        std::lock_guard<std::mutex> lock(_snapshot_mutex);
        _snapshot.clear();
    }
}

// ============================================================================
// is_listening - 检查是否正在监听
// ============================================================================
bool UsbHotplugWin::is_listening() const {
    return _running.load(std::memory_order_acquire);
}

// ============================================================================
// _win_wndproc - 隐藏窗口的窗口过程
//
// 接收 WM_DEVICECHANGE 消息，触发设备列表 diff。
// ============================================================================
LRESULT CALLBACK UsbHotplugWin::_win_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DEVICECHANGE) {
        // 设备到达或移除
        if (wp == DBT_DEVICEARRIVAL || wp == DBT_DEVICEREMOVECOMPLETE) {
            // 获取 UsbHotplugWin 实例指针
            auto* self = reinterpret_cast<UsbHotplugWin*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (self) {
                self->win_diff_devices();
            }
        }
        return TRUE;
    }
    if (msg == WM_CLOSE) {
        DestroyWindow(hwnd);
        return 0;
    }
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// ============================================================================
// _win_notify_thread - Windows 通知窗口线程
//
// 创建隐藏消息窗口，注册 USB 设备通知，处理消息循环。
// ============================================================================
void UsbHotplugWin::_win_notify_thread() {
    // 注册窗口类
    WNDCLASSA wc = {};
    wc.lpfnWndProc = _win_wndproc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "UsbHotplugWnd";
    RegisterClassA(&wc);

    // 创建隐藏消息窗口（不可见，仅接收消息）
    _hwnd = CreateWindowA(
        "UsbHotplugWnd", "",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, GetModuleHandleA(nullptr), this);

    // 设置窗口用户数据为当前实例指针
    SetWindowLongPtr(_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // 注册 USB 设备接口变更通知
    DEV_BROADCAST_DEVICEINTERFACE_A filter = {};
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
    _dev_notify = RegisterDeviceNotificationA(
        _hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);

    // 消息循环
    MSG msg;
    while (_running.load(std::memory_order_acquire)) {
        // 非阻塞获取消息
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // 无消息时短暂休眠，降低 CPU 占用
            Sleep(10); // 10ms
        }
    }

    // 清理
    if (_dev_notify) {
        UnregisterDeviceNotification(_dev_notify);
        _dev_notify = nullptr;
    }
    if (_hwnd) {
        SetWindowLongPtr(_hwnd, GWLP_USERDATA, 0);
        DestroyWindow(_hwnd);
        _hwnd = nullptr;
    }
    UnregisterClassA("UsbHotplugWnd", GetModuleHandleA(nullptr));
}

// ============================================================================
// _enum_devices - 使用 SetupDiGetClassDevs 枚举当前 USB 设备列表
//
// 参考 USBHotplugWindows::get_existing_decice_info() 实现。
// ============================================================================
std::vector<UsbHotplugWin::DevKey> UsbHotplugWin::_enum_devices() {
    std::vector<DevKey> devs;

    // 获取 USB 设备信息集
    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        &GUID_DEVINTERFACE_USB_DEVICE, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE) return devs;

    SP_DEVICE_INTERFACE_DATA ifaceData = {};
    ifaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    // 遍历所有设备接口
    for (DWORD idx = 0; ; ++idx) {
        // 获取设备接口信息
        if (!SetupDiEnumDeviceInterfaces(hDevInfo, nullptr, &GUID_DEVINTERFACE_USB_DEVICE, idx, &ifaceData)) {
            break;
        }

        // 获取设备接口详细信息（先查询所需大小）
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifaceData, nullptr, 0, &requiredSize, nullptr);
        if (requiredSize == 0) continue;

        // 分配缓冲区并获取详细信息
        auto* detailData = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_A>(malloc(requiredSize));
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        if (!SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifaceData, detailData, requiredSize, &requiredSize, nullptr)) {
            free(detailData);
            continue;
        }

        // 解析设备路径，提取 VID/PID/SN
        DevKey key = {};
        std::string devPath(detailData->DevicePath);
        std::tie(key.vid, key.pid, key.sn) = _parse_dev_path(devPath);
        key.path = devPath;

        // 获取设备位置信息（Hub/Port）
        SP_DEVINFO_DATA devInfoData = {};
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (SetupDiEnumDeviceInfo(hDevInfo, idx, &devInfoData)) {
            DWORD size = 0;
            // 查询 SPDRP_LOCATION_INFORMATION 所需大小
            SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfoData, SPDRP_LOCATION_INFORMATION, nullptr, nullptr, 0, &size);
            if (size > 0) {
                auto* buffer = new BYTE[size];
                if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfoData, SPDRP_LOCATION_INFORMATION, nullptr, buffer, size, nullptr)) {
                    std::string locInfo(reinterpret_cast<const char*>(buffer), size);
                    std::tie(key.bus, key.port) = _parse_location_info(locInfo);
                }
                delete[] buffer;
            }

            // 获取制造商（宽字符读取，转 UTF-8）
            size = 0;
            SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_MFG, nullptr, nullptr, 0, &size);
            if (size > 0) {
                auto* wbuf = new WCHAR[size / sizeof(WCHAR) + 1];
                if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_MFG, nullptr, reinterpret_cast<PBYTE>(wbuf), size, nullptr)) {
                    key.mfr = _wstr_to_utf8(wbuf);
                }
                delete[] wbuf;
            }

            // 获取产品名（宽字符读取，转 UTF-8）
            size = 0;
            SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DEVICEDESC, nullptr, nullptr, 0, &size);
            if (size > 0) {
                auto* wbuf = new WCHAR[size / sizeof(WCHAR) + 1];
                if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DEVICEDESC, nullptr, reinterpret_cast<PBYTE>(wbuf), size, nullptr)) {
                    key.prod = _wstr_to_utf8(wbuf);
                }
                delete[] wbuf;
            }
        }

        devs.push_back(key);
        free(detailData);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return devs;
}

// ============================================================================
// _parse_dev_path - 解析设备路径，提取 VID/PID/SN
//
// 设备路径格式：\\?\USB#VID_0001&PID_0002#abcdefg#{guid}
// ============================================================================
std::tuple<uint16_t, uint16_t, std::string> UsbHotplugWin::_parse_dev_path(const std::string& path) {
    uint16_t vid = 0, pid = 0;
    std::string sn;

    // 查找 "usb#" 起始位置
    size_t start = path.find("usb#");
    if (start == std::string::npos) return {vid, pid, sn};
    start += 4; // 跳过 "usb#"

    // 提取 VID
    size_t vidPos = path.find("vid_", start);
    if (vidPos != std::string::npos) {
        vidPos += 4; // 跳过 "vid_"
        size_t vidEnd = path.find("&", vidPos);
        if (vidEnd != std::string::npos) {
            vid = static_cast<uint16_t>(std::stoi(path.substr(vidPos, vidEnd - vidPos), nullptr, 16));
        }
    }

    // 提取 PID
    size_t pidPos = path.find("pid_", start);
    if (pidPos != std::string::npos) {
        pidPos += 4; // 跳过 "pid_"
        size_t pidEnd = path.find("#", pidPos);
        if (pidEnd != std::string::npos) {
            pid = static_cast<uint16_t>(std::stoi(path.substr(pidPos, pidEnd - pidPos), nullptr, 16));
        }
    }

    // 提取序列号（PID 后 # 到 #{ 之间）
    if (pidPos != std::string::npos) {
        size_t snStart = path.find("#", pidPos);
        if (snStart != std::string::npos) {
            snStart += 1; // 跳过 "#"
            size_t snEnd = path.find("#{", snStart);
            if (snEnd != std::string::npos) {
                sn = path.substr(snStart, snEnd - snStart);
            }
        }
    }

    return {vid, pid, sn};
}

// ============================================================================
// _parse_location_info - 解析位置信息，提取 Hub/Port
//
// 位置信息格式：Port_#0014.Hub_#0001
// ============================================================================
std::pair<uint16_t, uint16_t> UsbHotplugWin::_parse_location_info(const std::string& info) {
    uint16_t port = 0, hub = 0;

    // 提取 Port
    size_t portPos = info.find("Port_#");
    if (portPos != std::string::npos) {
        portPos += 6; // 跳过 "Port_#"
        size_t portEnd = info.find(".", portPos);
        if (portEnd != std::string::npos) {
            port = static_cast<uint16_t>(std::stoi(info.substr(portPos, portEnd - portPos)));
        }
    }

    // 提取 Hub
    size_t hubPos = info.find("Hub_#");
    if (hubPos != std::string::npos) {
        hubPos += 5; // 跳过 "Hub_#"
        size_t hubEnd = info.find(".", hubPos);
        if (hubEnd == std::string::npos) hubEnd = info.size();
        hub = static_cast<uint16_t>(std::stoi(info.substr(hubPos, hubEnd - hubPos)));
    }

    return {hub, port};
}

// ============================================================================
// _wstr_to_utf8 - 宽字符串转 UTF-8
// ============================================================================
std::string UsbHotplugWin::_wstr_to_utf8(const WCHAR* wstr) {
    if (!wstr || !wstr[0]) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string utf8(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &utf8[0], len, nullptr, nullptr);
    return utf8;
}

// ============================================================================
// _match_filter - 检查设备是否匹配过滤条件
// ============================================================================
bool UsbHotplugWin::_match_filter(const DevKey& key) const {
    // VID 过滤（0xFFFF = 匹配所有）
    if (_vid != 0xFFFF && key.vid != _vid) return false;
    // PID 过滤（0xFFFF = 匹配所有）
    if (_pid != 0xFFFF && key.pid != _pid) return false;
    return true;
}

// ============================================================================
// _make_device - 从 DevKey 创建 UsbDevice
//
// 尝试通过 libusb 查找匹配的设备创建完整 UsbDevice。
// 如果找不到（设备已拔出），用 DeviceInfo 创建离线 UsbDevice。
// ============================================================================
UsbDevice UsbHotplugWin::_make_device(const DevKey& key) const {
    // 尝试通过 libusb 查找设备
    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(nullptr, &list);
    if (cnt > 0 && list) {
        for (ssize_t i = 0; i < cnt; ++i) {
            libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(list[i], &desc) == 0) {
                if (desc.idVendor == key.vid && desc.idProduct == key.pid) {
                    UsbDevice dev(list[i]);
                    libusb_free_device_list(list, 1);
                    return dev;
                }
            }
        }
        libusb_free_device_list(list, 1);
    }

    // 找不到 libusb 设备，用 DeviceInfo 创建离线 UsbDevice
    DeviceInfo info;
    info.vendor_id = key.vid;
    info.product_id = key.pid;
    info.bus_number = static_cast<uint8_t>(key.bus);
    info.port_number = static_cast<uint8_t>(key.port);
    info.manufacturer = key.mfr;
    info.product = key.prod;
    info.serial_number = key.sn;
    // SN 转大写，与 libusb 读取的格式一致
    std::transform(info.serial_number.begin(), info.serial_number.end(),
                   info.serial_number.begin(), ::toupper);
    return UsbDevice(info);
}

// ============================================================================
// win_diff_devices - 设备列表 diff：对比当前设备与上次快照
//
// 当 WM_DEVICECHANGE 消息触发时调用。
// 对比当前设备列表与上次快照，确定新增/移除的设备，触发回调。
// ============================================================================
void UsbHotplugWin::win_diff_devices() {
    if (!_callback) return;

    // 枚举当前设备列表
    std::vector<DevKey> current = _enum_devices();

    std::vector<DevKey> arrived;
    std::vector<DevKey> left;

    {
        std::lock_guard<std::mutex> lock(_snapshot_mutex);

    // 查找新增设备（在 current 中但不在 snapshot 中）
        for (const auto& cur : current) {
        // 检查过滤条件
            if (!_match_filter(cur)) continue;

            bool found = false;
            for (const auto& snap : _snapshot) {
                if (snap.vid == cur.vid && snap.pid == cur.pid &&
                    snap.bus == cur.bus && snap.port == cur.port) {
                    found = true;
                    break;
                }
            }
            if (!found) arrived.push_back(cur);
        }

        // 查找移除设备（在 snapshot 中但不在 current 中）
        for (const auto& snap : _snapshot) {
        // 检查过滤条件
            if (!_match_filter(snap)) continue;
            bool found = false;
            for (const auto& cur : current) {
                if (snap.vid == cur.vid && snap.pid == cur.pid &&
                    snap.bus == cur.bus && snap.port == cur.port) {
                    found = true;
                    break;
                }
            }
            if (!found) left.push_back(snap);
        }
        _snapshot = current;
    }

    // 触发新增设备回调
    for (const auto& key : arrived) {
        UsbDevice dev = _make_device(key);
        _callback(HotplugEvent::Arrived, dev);
    }

    // 触发移除设备回调
    for (const auto& key : left) {
        UsbDevice dev = _make_device(key);
        _callback(HotplugEvent::Left, dev);
    }
}

} // namespace core
} // namespace usb_ctrl

#endif // _WIN32
