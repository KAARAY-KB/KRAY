// ============================================================================
// common.h - 单元测试公共工具头文件
//
// 提供所有测试文件共用的工具函数和宏定义：
//   - print_separator：打印分隔线
//   - print_transfer_result：打印传输结果
//   - parse_hex：解析十六进制字符串
//   - TEST_ASSERT：断言宏
//   - auto_open_hid：自动打开第一个 HID 设备
// ============================================================================

#pragma once

#include "usb_controller.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <thread>
#include <chrono>

using namespace usb_ctrl;

// ============================================================================
// 打印分隔线
// @param title 可选标题
// ============================================================================
inline void print_sep(const char* title = nullptr) {
    std::cout << "\n";
    if (title)
        std::cout << "===== " << title << " =====\n";
    else
        std::cout << "========================================\n";
}

// ============================================================================
// 打印传输结果
// @param label 标签
// @param r     传输结果
// ============================================================================
inline void print_result(const char* label, const TransferResult& r) {
    std::cout << "  [" << label << "] " << format_transfer_result(r, r.bytes_transferred) << "\n";
}

// ============================================================================
// 解析十六进制字符串为字节数组
// @param hex_str 十六进制字符串
// @return 字节数组
// ============================================================================
inline std::vector<uint8_t> parse_hex(const std::string& hex_str) {
    std::vector<uint8_t> result; // 结果数组
    for (size_t i = 0; i + 1 < hex_str.length(); i += 2) {
        std::string byte = hex_str.substr(i, 2); // 取两个字符
        result.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16))); // 转为字节
    }
    return result;
}

// ============================================================================
// 测试断言宏
// @param cond 条件
// @param msg  失败消息
// ============================================================================
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cout << "  [FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
            return 1; \
        } else { \
            std::cout << "  [PASS] " << msg << "\n"; \
        } \
    } while(0)

// ============================================================================
// 自动打开第一个 HID 设备
// @param ctrl USB 控制器引用
// @return true 成功打开
// ============================================================================
inline bool auto_open_hid(UsbController& ctrl) {
    if (ctrl.is_hid_open()) return true; // 已打开
    auto& devs = ctrl.devices(); // 获取设备列表
    for (size_t i = 0; i < devs.size(); ++i) {
        if (devs[i].has_hid_interface() && ctrl.open_hid_device(i)) {
            std::cout << "  Auto-opened [" << i << "] " << ctrl.hid_device_info() << "\n";
            return true;
        }
    }
    std::cout << "  No HID device available\n";
    return false;
}
