// ============================================================================
// usb_device_base.hpp - USB 设备基类（头文件）
//
// 功能说明：
//   定义 UsbDeviceBase 基类，封装 usb_ctrl::UsbController 的通用操作，
//   为 Qt 上层提供简洁的设备操作接口。
//
// 设计模式：模板方法模式（Template Method）
//   基类提供通用的 open/close/read/write 流程，
//   子类可以覆写特定行为（如设备特定的端点配置）。
//
// 使用方式：
//   1. 创建子类实例（如 GT64HeDevice）
//   2. 调用 open() 打开设备
//   3. 调用 write() / start_read() / stop_read() 进行数据传输
//   4. 调用 close() 关闭设备
// ============================================================================

#pragma once

#include "usb_device_info.hpp"
#include "usb_controller.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <cstdint>

// ============================================================================
// UsbDeviceBase - USB 设备基类
//
// 封装 UsbController，提供设备打开/关闭、同步/异步读写的统一接口。
// Qt 上层通过此基类操作 USB 设备，无需直接接触底层 UsbController。
// ============================================================================
class UsbDeviceBase {
public:
    // 数据回调类型：接收到数据时触发
    using DataCallback = std::function<void(const std::vector<uint8_t>&)>;

    // 构造函数：绑定设备信息
    explicit UsbDeviceBase(const UsbDeviceInfo& info);

    // 虚析构函数：确保子类正确释放资源
    virtual ~UsbDeviceBase();

    // 打开设备（查找 HID 接口、声明接口、准备传输）
    bool open();

    // 关闭设备（释放接口、关闭句柄）
    void close();

    // 检查设备是否已打开
    bool is_open() const;

    // 获取设备信息
    const UsbDeviceInfo& info() const;

    // 同步写入数据
    bool write(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // 同步读取数据
    std::vector<uint8_t> read(int length, unsigned int timeout_ms = 1000);

    // 启动连续异步读取
    void start_read(int length, DataCallback cb, unsigned int timeout_ms = 1000);

    // 停止连续异步读取
    void stop_read();

    // 异步写入数据
    void write_async(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

protected:
    // 子类可覆写：打开设备后的额外初始化
    virtual bool on_opened() { return true; }

    // 子类可覆写：关闭设备前的额外清理
    virtual void on_closing() {}

    UsbDeviceInfo m_info;                           // 设备信息
    std::unique_ptr<usb_ctrl::UsbController> m_ctrl; // USB 控制器
    DataCallback m_read_cb;                         // 读取回调
};
