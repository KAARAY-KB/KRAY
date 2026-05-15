// ============================================================================
// usb_controller.hpp - USB 控制器统一接口（头文件）
//
// 功能说明：
//   本文件定义了 UsbController 类，作为整个 USB 控制库的统一对外接口。
//   它将 usb_core（设备管理）和 usb_io（数据传输）两个子模块
//   整合为一个易用的高层 API。
//
// 设计模式：外观模式（Facade Pattern）
//   UsbController 作为外观类，封装了底层复杂的 USB 操作细节，
//   对外提供简洁统一的接口。用户只需使用 UsbController 即可完成
//   设备枚举、HID 设备打开/关闭、同步/异步数据传输等所有操作。
//
// 内部组件：
//   - UsbContext        ：USB 上下文（libusb 初始化）
//   - UsbDeviceManager  ：设备管理器（设备枚举和查询）
//   - HidDevice         ：HID 设备操作（打开/关闭/报告读写）
//   - SyncTransfer      ：同步传输（批量/中断/控制传输）
//   - AsyncTransfer     ：异步传输（引擎管理 + HID 传输操作，单类）
//
// 依赖关系：
//   - usb_core/*    ：USB 核心模块（上下文、设备、设备管理器）
//   - usb_io/*    ：USB 数据传输模块（同步传输、HID 设备、异步传输）
// ============================================================================

#pragma once

#include "usb_core/usb_context.hpp"
#include "usb_core/usb_device.hpp"
#include "usb_core/usb_device_manager.hpp"
#include "usb_io/sync_transfer.hpp"
#include "usb_io/hid_device.hpp"
#include "usb_io/async_transfer.hpp"

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace usb_ctrl {

// 导入常用类型到当前命名空间，简化使用
using core::UsbContext;          // USB 上下文
using core::UsbDevice;           // USB 设备
using core::UsbDeviceManager;    // USB 设备管理器
using core::DeviceInfo;          // 设备信息结构体
using core::EndpointInfo;        // 端点信息结构体
using io::TransferResult;  // 传输结果结构体
using io::SyncTransfer;    // 同步传输类
using io::HidDevice;       // HID 设备类
using core::UsbEventThread;    // 事件处理线程（独立于传输逻辑）
using io::AsyncTransfer;       // 异步传输类
using io::AsyncCallback;       // 异步回调类型

// ============================================================================
// UsbController - USB 控制器统一接口类
//
// 提供 USB 设备操作的完整功能：
//   1. 设备管理：枚举、查询、查找设备
//   2. HID 设备：打开/关闭、读取/写入报告、特性报告
//   3. 同步传输：批量传输、中断传输
//   4. 异步传输：单次异步读写、连续异步读取
//
// 使用示例：
//   UsbController ctrl;
//   ctrl.refresh_devices();
//   std::cout << ctrl.list_devices();
//   ctrl.open_hid_device(0);
//   auto result = ctrl.hid_read(64);
// ============================================================================
class UsbController {
public:
    // 构造函数：初始化 USB 上下文并枚举设备
    UsbController();

    // 析构函数：自动关闭 HID 设备并停止异步引擎
    ~UsbController();

    // 禁止拷贝（控制器拥有唯一的所有权）
    UsbController(const UsbController&) = delete;
    UsbController& operator=(const UsbController&) = delete;

    // ========================================================================
    // 基础设置
    // ========================================================================

    // 设置 libusb 调试输出级别
    // @param level 调试级别：0=无输出, 1=错误, 2=警告, 3=信息, 4=调试
    void set_debug_level(int level);

    // 刷新设备列表（重新枚举所有 USB 设备）
    void refresh_devices();

    // ========================================================================
    // 设备查询
    // ========================================================================

    // 获取设备简要列表（每个设备一行摘要）
    // @return 格式化的设备摘要字符串
    std::string list_devices() const;

    // 获取设备详细列表（包含完整描述符信息）
    // @return 格式化的设备详情字符串
    std::string list_devices_detail() const;

    // 获取设备树形结构列表（层级缩进格式）
    // @return 格式化的设备树字符串
    std::string list_devices_tree() const;

    // 获取 HID 设备列表（仅显示 HID 类设备）
    // @return 格式化的 HID 设备摘要字符串
    std::string list_hid_devices() const;

    // 获取设备数量
    // @return 当前枚举到的设备数量
    size_t device_count() const;

    // 获取指定索引设备的详细信息
    // @param index 设备在列表中的索引
    // @return 设备详情字符串
    std::string device_detail(size_t index) const;

    // 获取设备列表的可读写引用
    // @return 设备列表引用
    std::vector<UsbDevice>& devices() { return _manager->devices(); }

    // 获取设备列表的只读引用
    // @return 设备列表常量引用
    const std::vector<UsbDevice>& devices() const { return _manager->devices(); }

    // 按 VID 和 PID 查找设备
    // @param vid 厂商 ID
    // @param pid 产品 ID
    // @return 匹配的设备列表
    std::vector<UsbDevice> find_by_vid_pid(uint16_t vid, uint16_t pid);

    // 查找所有 HID 类设备
    // @return HID 设备列表
    std::vector<UsbDevice> find_hid_devices();

    // ========================================================================
    // HID 设备操作
    // ========================================================================

    // 按索引打开 HID 设备
    // 自动检测 HID 接口、分离内核驱动、声明接口。
    // @param index 设备在列表中的索引
    // @return true 表示打开成功
    bool open_hid_device(size_t index);

    // 按 VID/PID 打开 HID 设备
    // 查找匹配的设备并打开第一个有 HID 接口的设备。
    // @param vid 厂商 ID
    // @param pid 产品 ID
    // @return true 表示打开成功
    bool open_hid_device_by_vid_pid(uint16_t vid, uint16_t pid);

    // 关闭当前打开的 HID 设备
    void close_hid_device();

    // 检查 HID 设备是否已打开
    // @return true 表示设备已打开
    bool is_hid_open() const;

    // 获取 HID 设备信息
    // @return 包含接口编号和端点信息的字符串
    std::string hid_device_info() const;

    // 读取 HID 输入报告（通过中断 IN 端点）
    // @param length     要读取的字节数
    // @param timeout_ms 超时时间（毫秒）
    // @return TransferResult 包含读取结果和数据
    TransferResult hid_read(int length, unsigned int timeout_ms = 1000);

    // 读取最新的 HID 输入报告（先排空缓冲区，再等待设备新数据）
//
// 两步策略：
//   1. 以短超时循环读取，排空 libusb 内部已有的缓冲数据
//   2. 以 1100ms 长超时等待设备的下一个数据包，确保拿到最新数据
//
// 适用场景：设备持续发送数据，但你只关心当前最新值。
//
// @param length            要读取的字节数
// @param drain_timeout_ms  排空缓冲区时每次读取的超时（毫秒），默认 10ms
// @return TransferResult 包含最新读取结果和数据
TransferResult hid_read_latest(int length, unsigned int drain_timeout_ms = 10);

    // 写入 HID 输出报告（通过中断 OUT 端点）
    // @param data       要写入的数据
    // @param timeout_ms 超时时间（毫秒）
    // @return TransferResult 包含写入结果
    TransferResult hid_write(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // 获取 HID 特性报告（通过控制传输）
    // @param report_id  报告 ID
    // @param length     期望读取的数据长度
    // @param timeout_ms 超时时间（毫秒）
    // @return TransferResult 包含读取结果和数据
    TransferResult hid_get_feature_report(uint8_t report_id, int length, unsigned int timeout_ms = 1000);

    // 发送 HID 特性报告（通过控制传输）
    // @param data       要发送的数据
    // @param timeout_ms 超时时间（毫秒）
    // @return TransferResult 包含发送结果
    TransferResult hid_send_feature_report(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // ========================================================================
    // 同步传输（需要先打开 HID 设备）
    // ========================================================================

    // 批量读取
    // @param endpoint   端点地址
    // @param length     要读取的字节数
    // @param timeout_ms 超时时间（毫秒）
    // @return TransferResult 包含读取结果
    TransferResult bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);

    // 批量写入
    // @param endpoint   端点地址
    // @param data       要写入的数据
    // @param timeout_ms 超时时间（毫秒）
    // @return TransferResult 包含写入结果
    TransferResult bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // 中断读取
    // @param endpoint   端点地址
    // @param length     要读取的字节数
    // @param timeout_ms 超时时间（毫秒）
    // @return TransferResult 包含读取结果
    TransferResult interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);

    // 中断写入
    // @param endpoint   端点地址
    // @param data       要写入的数据
    // @param timeout_ms 超时时间（毫秒）
    // @return TransferResult 包含写入结果
    TransferResult interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    // ========================================================================
    // 异步传输
    // ========================================================================

    // 启动异步传输引擎（自动在需要时调用）
    void async_start();

    // 停止异步传输引擎
    void async_stop();

    // 单次异步 HID 读取
    // @param length     要读取的字节数
    // @param callback   完成回调函数
    // @param timeout_ms 超时时间（毫秒）
    void hid_read_async(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 单次异步 HID 写入
    // @param data       要写入的数据
    // @param callback   完成回调函数
    // @param timeout_ms 超时时间（毫秒）
    void hid_write_async(const std::vector<uint8_t>& data, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 连续异步 HID 读取（自动循环读取）
    // @param length     每次读取的字节数
    // @param callback   每次读取完成后的回调函数
    // @param timeout_ms 超时时间（毫秒）
    void hid_read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);

    // 停止连续异步读取
    void hid_stop_continuous();

    // 获取异步传输待处理数量
    // @return 待处理传输数量
    size_t async_pending_count() const;

    // ========================================================================
    // 底层对象访问（高级用法）
    // ========================================================================

    // 获取 HID 设备对象指针
    // @return HidDevice 指针（未打开时返回 nullptr）
    HidDevice* hid_device() { return _hid_device.get(); }

    // 获取同步传输对象指针
    // @return SyncTransfer 指针（未打开设备时返回 nullptr）
    SyncTransfer* sync_transfer() { return _sync_transfer.get(); }

    // 获取异步传输对象指针
    // @return AsyncTransfer 指针（未启动时返回 nullptr）
    AsyncTransfer* async_transfer() { return _async_transfer.get(); }

private:
    std::unique_ptr<UsbContext> _ctx;                   // USB 上下文（libusb 初始化）
    std::unique_ptr<UsbDeviceManager> _manager;         // 设备管理器（设备枚举和查询）
    std::unique_ptr<HidDevice> _hid_device;             // HID 设备（当前打开的设备）
    std::unique_ptr<SyncTransfer> _sync_transfer;       // 同步传输对象
    std::unique_ptr<UsbEventThread> _event_thread;      // 事件处理线程（独立于传输逻辑）
    std::unique_ptr<AsyncTransfer> _async_transfer;     // 异步传输对象

    // 确保异步传输对象已创建并绑定设备（内部辅助方法）
    void _ensure_async_bound();
};

} // namespace usb_ctrl
