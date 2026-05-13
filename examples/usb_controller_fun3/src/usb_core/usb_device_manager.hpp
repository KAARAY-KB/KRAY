// ============================================================================
// usb_device_manager.hpp - USB 设备管理器模块（头文件）
//
// 功能说明：
//   本文件定义了 UsbDeviceManager 类，负责 USB 设备的枚举、刷新和查询。
//   它是对 libusb 设备列表的 C++ 封装，提供设备列表的获取、过滤和格式化输出。
//
// 核心职责：
//   - 调用 libusb_get_device_list() 枚举系统所有 USB 设备
//   - 将原始 libusb_device 列表转换为 UsbDevice 对象列表
//   - 提供多种设备列表格式化输出（摘要/详情/树形/HID 过滤）
//   - 提供设备查找功能（按 VID:PID、按类代码、按 HID 类型）
//
// 依赖关系：
//   - usb_device.hpp：UsbDevice 类
//   - usb_context.hpp：UsbContext 类
//   - libusb.h：底层 USB 设备枚举 API
// ============================================================================

#pragma once

#include "usb_device.hpp" // UsbDevice 类定义
#include <memory>         // std::unique_ptr 智能指针
#include <vector>         // std::vector 动态数组
#include <string>         // std::string 字符串

// 前向声明 libusb 设备结构体
struct libusb_device;

namespace usb_ctrl {
namespace core {

// 前向声明 UsbContext 类（避免循环依赖）
class UsbContext;

// ============================================================================
// UsbDeviceManager - USB 设备管理器类
//
// 管理系统中所有 USB 设备的列表，提供设备枚举、刷新、查询和格式化输出。
// 持有 UsbContext 的引用（不拥有所有权），通过 refresh() 方法更新设备列表。
// ============================================================================
class UsbDeviceManager {
public:
    // 构造函数：使用给定的 USB 上下文初始化管理器
    // 构造时会自动调用 refresh() 枚举设备
    // @param ctx USB 上下文引用
    explicit UsbDeviceManager(UsbContext& ctx);

    // 禁止拷贝（管理器持有设备列表的唯一所有权）
    UsbDeviceManager(const UsbDeviceManager&) = delete;
    UsbDeviceManager& operator=(const UsbDeviceManager&) = delete;

    // 刷新设备列表：重新枚举系统所有 USB 设备
    // 会清空现有列表并重新获取
    void refresh();

    // 获取当前设备数量
    // @return 设备列表中的设备数量
    size_t count() const;

    // 获取设备简要摘要列表（每个设备一行）
    // @return 格式化的设备摘要字符串
    std::string list_summary() const;

    // 获取设备详细列表（每个设备包含完整描述符信息）
    // @return 格式化的设备详情字符串
    std::string list_detail() const;

    // 获取设备树形结构列表（层级缩进格式）
    // @return 格式化的设备树字符串
    std::string list_tree() const;

    // 获取 HID 设备摘要列表（仅显示 HID 类设备）
    // @return 格式化的 HID 设备摘要字符串
    std::string list_hid_summary() const;

    // 按 VID 和 PID 查找设备
    // @param vid 厂商 ID
    // @param pid 产品 ID
    // @return 匹配的设备列表（每个设备是独立的 UsbDevice 副本）
    std::vector<UsbDevice> find_by_vid_pid(uint16_t vid, uint16_t pid);

    // 查找所有 HID 类设备
    // @return HID 设备列表
    std::vector<UsbDevice> find_hid_devices();

    // 按设备类代码查找设备
    // @param cls USB 设备类代码（如 0x03=HID, 0x08=Mass Storage）
    // @return 匹配的设备指针列表（指向内部设备列表中的设备）
    std::vector<UsbDevice*> find_by_class(uint8_t cls);

    // 获取设备列表的可读写引用
    // @return 设备列表引用
    std::vector<UsbDevice>& devices() { return _devices; }

    // 获取设备列表的只读引用
    // @return 设备列表常量引用
    const std::vector<UsbDevice>& devices() const { return _devices; }

private:
    // 将 libusb 原始设备列表转换为 UsbDevice 对象列表
    // @param list  libusb 原始设备指针数组
    // @param count 设备数量
    // @return UsbDevice 对象列表
    std::vector<UsbDevice> _raw_to_list(libusb_device** list, ssize_t count);

    UsbContext& _ctx;                // USB 上下文引用（不拥有所有权）
    std::vector<UsbDevice> _devices; // 设备列表（拥有设备对象所有权）
};

} // namespace core
} // namespace usb_ctrl
