// ============================================================================
// usb_hotplug_win.hpp - USB 热插拔异步监听模块 - Windows 版本（头文件）
//
// 功能说明：
//   使用 Windows 原生设备通知机制实现 USB 设备热插拔监听。
//   继承 UsbHotplugBase 抽象基类，提供 Windows 平台实现。
//
// 实现原理（参考 USBHotplugWindows）：
//   1. 创建隐藏消息窗口（HWND_MESSAGE）
//   2. 注册 USB 设备接口通知（RegisterDeviceNotification）
//   3. 窗口过程接收 WM_DEVICECHANGE 消息
//   4. 收到通知后枚举当前设备列表与快照对比（diff）
//   5. 将 diff 结果转换为 HotplugEvent 回调
//
// 依赖关系：
//   - usb_hotplug_base.hpp：抽象基类
//   - windows.h / dbt.h / setupapi.h：Windows 设备通知 API
// ============================================================================

#pragma once

#ifdef _WIN32

#include "usb_hotplug_base.hpp"
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <string>

#include <windows.h>
#include <dbt.h>
#include <setupapi.h>
#include <initguid.h>
#include <usbiodef.h>

namespace usb_ctrl {
namespace core {

// ============================================================================
// UsbHotplugWin - USB 热插拔异步监听类（Windows 实现）
// ============================================================================
class UsbHotplugWin : public UsbHotplugBase {
public:
    UsbHotplugWin();
    ~UsbHotplugWin() override;

    UsbHotplugWin(const UsbHotplugWin&) = delete;
    UsbHotplugWin& operator=(const UsbHotplugWin&) = delete;

    // Windows 始终支持热插拔
    static bool is_supported();

    void set_callback(HotplugCallback cb) override;
    void set_vid_filter(uint16_t vid) override;
    void set_pid_filter(uint16_t pid) override;
    void set_class_filter(uint8_t cls) override;
    bool start() override;
    void stop() override;
    bool is_listening() const override;

    // 公开 diff 方法，供窗口过程调用
    void win_diff_devices();

private:
    // 设备快照中的设备标识
    struct DevKey {
        uint16_t vid;       // 厂商 ID
        uint16_t pid;       // 产品 ID
        uint16_t bus;       // 总线编号（Hub 编号）
        uint16_t port;      // 端口号
        std::string sn;     // 序列号
        std::string mfr;    // 制造商
        std::string prod;   // 产品名
        std::string path;   // 设备路径
    };

    // Windows 通知线程
    void _win_notify_thread();

    // 使用 SetupDiGetClassDevs 获取当前 USB 设备列表
    std::vector<DevKey> _enum_devices();

    // 解析设备路径，提取 VID/PID/SN
    static std::tuple<uint16_t, uint16_t, std::string> _parse_dev_path(const std::string& path);

    // 解析位置信息，提取 Hub/Port
    static std::pair<uint16_t, uint16_t> _parse_location_info(const std::string& info);

    // 宽字符串转 UTF-8
    static std::string _wstr_to_utf8(const WCHAR* wstr);

    // 检查设备是否匹配过滤条件
    bool _match_filter(const DevKey& key) const;

    // 从 DevKey 创建 UsbDevice（用于回调）
    UsbDevice _make_device(const DevKey& key) const;

    // 静态窗口过程
    static LRESULT CALLBACK _win_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND _hwnd = nullptr;                     // 隐藏窗口句柄
    HDEVNOTIFY _dev_notify = nullptr;         // 设备通知句柄
    std::thread _thread;                      // 通知窗口线程
    std::atomic<bool> _running{false};        // 线程运行标志

    HotplugCallback _callback;                // 用户回调
    uint16_t _vid = 0xFFFF;                   // VID 过滤（0xFFFF = 匹配所有）
    uint16_t _pid = 0xFFFF;                   // PID 过滤（0xFFFF = 匹配所有）
    uint8_t  _cls = 0xFF;                     // 类代码过滤（暂未实现）

    std::vector<DevKey> _snapshot;            // 上一次设备快照
    std::mutex _snapshot_mutex;               // 快照互斥锁
};

} // namespace core
} // namespace usb_ctrl

#endif // _WIN32
