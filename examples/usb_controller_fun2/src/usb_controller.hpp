#pragma once

#include "usb_core/usb_context.hpp"
#include "usb_core/usb_device.hpp"
#include "usb_core/usb_device_manager.hpp"
#include "usb_transfer/usb_transfer.hpp"
#include "usb_transfer/hid_device.hpp"
#include "usb_transfer/async_transfer.hpp"

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace usb_ctrl {

// 类型别名：简化底层类型的使用
using usb_core::UsbContext;
using usb_core::UsbDevice;
using usb_core::UsbDeviceManager;
using usb_transfer::TransferResult;
using usb_transfer::SyncTransfer;
using usb_transfer::HidDevice;
using usb_transfer::AsyncTransferEngine;
using usb_transfer::AsyncHidTransfer;
using usb_transfer::AsyncCallback;

// USB 控制器类 (应用层统一 API)
// 封装所有 USB 操作，提供设备管理、HID 通信和异步传输的统一接口
// 是外部调用者使用的主要入口类
class UsbController {
public:
    UsbController();
    ~UsbController();

    // 禁止拷贝
    UsbController(const UsbController&) = delete;
    UsbController& operator=(const UsbController&) = delete;

    // 设置 libusb 调试日志级别
    void set_debug_level(int level);

    // 刷新设备列表 (重新枚举所有 USB 设备)
    void refresh_devices();

    // 获取所有设备的摘要信息列表
    std::string list_devices() const;

    // 获取所有设备的详细信息列表
    std::string list_devices_detail() const;

    // 获取所有 HID 设备的摘要信息列表
    std::string list_hid_devices() const;

    // 获取设备总数
    size_t device_count() const;

    // 获取设备列表引用
    std::vector<UsbDevice>& devices() { return _manager->devices(); }
    const std::vector<UsbDevice>& devices() const { return _manager->devices(); }

    // 获取指定索引设备的详细信息
    std::string device_detail(size_t index) const;

    // 根据 VID/PID 查找设备
    std::vector<UsbDevice> find_by_vid_pid(uint16_t vid, uint16_t pid);

    // 打开指定索引的 HID 设备
    bool open_hid_device(size_t index);

    // 根据 VID/PID 打开 HID 设备
    bool open_hid_device_by_vid_pid(uint16_t vid, uint16_t pid);

    // 关闭已打开的 HID 设备
    void close_hid_device();

    // 判断 HID 设备是否已打开
    bool is_hid_open() const;

    // 获取已打开 HID 设备的信息
    std::string hid_device_info() const;

    // 同步读取 HID 报告
    TransferResult hid_read(int length, unsigned int timeout_ms = 1000);

    // 同步写入 HID 报告
    TransferResult hid_write(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // 获取 HID 特性报告
    TransferResult hid_get_feature_report(uint8_t report_id, int length,
                                           unsigned int timeout_ms = 1000);

    // 发送 HID 特性报告
    TransferResult hid_send_feature_report(const std::vector<uint8_t>& data,
                                            unsigned int timeout_ms = 1000);

    // 启动异步传输引擎
    void async_start();

    // 停止异步传输引擎
    void async_stop();

    // 异步读取 HID 报告 (单次)
    void hid_read_async(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 异步写入 HID 报告 (单次)
    void hid_write_async(const std::vector<uint8_t>& data, AsyncCallback callback,
                          unsigned int timeout_ms = 1000);

    // 连续异步读取 HID 报告 (循环读取)
    void hid_read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 停止连续异步读取
    void hid_stop_continuous();

    // 获取待处理的异步传输请求数量
    size_t async_pending_count() const;

    // 获取底层 HID 设备对象指针
    HidDevice* hid_device() { return _hid_device.get(); }

    // 获取底层同步传输对象指针
    SyncTransfer* sync_transfer() { return _sync_transfer.get(); }

    // 获取底层异步传输引擎指针
    AsyncTransferEngine* async_engine() { return _async_engine.get(); }

private:
    std::unique_ptr<UsbContext> _ctx;                  // USB 上下文 (RAII 管理)
    std::unique_ptr<UsbDeviceManager> _manager;        // 设备管理器
    std::unique_ptr<HidDevice> _hid_device;            // HID 设备操作对象
    std::unique_ptr<SyncTransfer> _sync_transfer;      // 同步传输对象
    std::unique_ptr<AsyncTransferEngine> _async_engine; // 异步传输引擎
    std::unique_ptr<AsyncHidTransfer> _async_hid;      // HID 异步传输对象
};

} // namespace usb_ctrl
