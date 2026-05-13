// ============================================================================
// main.cpp - USB 控制器演示程序
//
// 功能说明：
//   本文件是 USB 控制器库的交互式演示程序。
//   提供命令行菜单界面，让用户可以交互式地测试 USB 控制器的各项功能。
//
// 程序结构：
//   1. 全局变量和工具函数
//   2. 菜单显示函数
//   3. 各功能演示函数（demo_*）
//   4. main() 主函数（事件循环）
//
// 演示功能列表：
//   1.  列出所有 USB 设备
//   2.  列出设备详细信息
//   3.  树形视图显示设备
//   4.  列出 HID 设备
//   5.  按索引打开 HID 设备
//   6.  按 VID:PID 打开 HID 设备
//   7.  显示设备详情
//   8.  同步 HID 读取
//   9.  同步 HID 写入
//   10. 获取 HID 特性报告
//   11. 发送 HID 特性报告
//   12. 单次异步 HID 读取
//   13. 连续异步 HID 读取（5 秒）
//   14. 原始端点批量读取
//   15. 原始端点批量写入
//   16. 按 VID:PID 查找设备
//   17. 自动运行所有演示
//   18. 读取最新 HID 报告（先排空缓冲区）
//
// 依赖关系：
//   - usb_controller.hpp：UsbController 统一接口
//   - <iostream>：控制台输入输出
//   - <thread>/<chrono>：延时和异步演示
//   - <atomic>：线程安全的标志变量
// ============================================================================

#include "usb_controller.hpp"

#include <iostream>
#include <iomanip>    // std::hex, std::dec 格式化
#include <string>
#include <thread>
#include <chrono>     // std::chrono::seconds, milliseconds
#include <atomic>
#include <sstream>

using namespace usb_ctrl; // 使用 usb_ctrl 命名空间

// ============================================================================
// 全局运行标志（原子变量，线程安全）
// 用于控制主事件循环的退出
// ============================================================================
static std::atomic<bool> g_running{true};

// ============================================================================
// print_separator - 打印分隔线
//
// 在控制台输出中插入分隔线，用于区分不同的演示输出。
// 如果提供了标题，会在分隔线中显示标题。
//
// @param title 可选的标题文本（nullptr 表示无标题）
// ============================================================================
void print_separator(const char* title = nullptr) {
    std::cout << "\n";
    if (title)
        std::cout << "===== " << title << " =====\n";
    else
        std::cout << "========================================\n";
}

// ============================================================================
// print_transfer_result - 打印传输结果
//
// 使用 format_transfer_result 格式化传输结果并输出到控制台。
//
// @param label 标签文本（如 "Read", "Write"）
// @param r     传输结果结构体
// ============================================================================
void print_transfer_result(const char* label, const TransferResult& r) {
    std::cout << "  [" << label << "] " << format_transfer_result(r, 32) << "\n";
}

// ============================================================================
// parse_hex - 解析十六进制字符串为字节数组
//
// 将 "AABBCCDD" 格式的十六进制字符串解析为字节数组 {0xAA, 0xBB, 0xCC, 0xDD}。
// 用于用户输入 HID 报告数据。
//
// @param hex_str 十六进制字符串（每两个字符一个字节）
// @return 解析后的字节数组
// ============================================================================
std::vector<uint8_t> parse_hex(const std::string& hex_str) {
    std::vector<uint8_t> result; // 结果数组
    // 每两个字符解析为一个字节
    for (size_t i = 0; i + 1 < hex_str.length(); i += 2) {
        std::string byte = hex_str.substr(i, 2); // 取两个字符
        result.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16))); // 十六进制转整数
    }
    return result;
}

// ============================================================================
// print_menu - 打印交互式菜单
//
// 显示所有可用的演示功能选项，供用户选择。
// ============================================================================
void print_menu() {
    std::cout << "\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|     USB Controller - Unified API     |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|  1. List all USB devices             |\n"; // 列出所有 USB 设备
    std::cout << "|  2. List devices (detailed)          |\n"; // 列出设备详细信息
    std::cout << "|  3. List devices (tree view)         |\n"; // 树形视图
    std::cout << "|  4. List HID devices                 |\n"; // 列出 HID 设备
    std::cout << "|  5. Open HID device by index         |\n"; // 按索引打开 HID
    std::cout << "|  6. Open HID device by VID:PID       |\n"; // 按 VID:PID 打开 HID
    std::cout << "|  7. Show device detail               |\n"; // 显示设备详情
    std::cout << "|  8. HID read report (sync)           |\n"; // 同步 HID 读取
    std::cout << "|  9. HID write report (sync)          |\n"; // 同步 HID 写入
    std::cout << "| 10. HID get feature report           |\n"; // 获取特性报告
    std::cout << "| 11. HID send feature report          |\n"; // 发送特性报告
    std::cout << "| 12. HID read (single async)          |\n"; // 单次异步读取
    std::cout << "| 13. HID read (continuous, 5s)        |\n"; // 连续异步读取
    std::cout << "| 14. Raw endpoint bulk read           |\n"; // 原始批量读取
    std::cout << "| 15. Raw endpoint bulk write          |\n"; // 原始批量写入
    std::cout << "| 16. Find device by VID:PID           |\n"; // 按 VID:PID 查找
    std::cout << "| 17. Run all demos (auto)             |\n"; // 自动运行所有演示
    std::cout << "| 18. HID read latest (drain buffer)   |\n"; // 读取最新 HID 报告（排空缓冲区）
    std::cout << "|  0. Exit                             |\n"; // 退出
    std::cout << "+--------------------------------------+\n";
    std::cout << "Choice: "; // 提示用户输入
}

// ============================================================================
// demo_list_devices - 演示：列出所有 USB 设备
//
// 调用 UsbController::list_devices() 获取设备简要列表并输出。
// ============================================================================
void demo_list_devices(UsbController& ctrl) {
    print_separator("1. All USB Devices");
    std::cout << ctrl.list_devices(); // 输出设备列表
}

// ============================================================================
// demo_list_detail - 演示：列出设备详细信息
//
// 显示前 3 个设备的详细信息（避免输出过多）。
// ============================================================================
void demo_list_detail(UsbController& ctrl) {
    print_separator("2. Devices Detailed");
    // 遍历前 3 个设备（或全部，如果少于 3 个）
    for (size_t i = 0; i < ctrl.device_count() && i < 3; ++i) {
        std::cout << ctrl.device_detail(i) << "\n"; // 输出设备详情
    }
    // 如果设备超过 3 个，显示省略提示
    if (ctrl.device_count() > 3)
        std::cout << "... (" << ctrl.device_count() - 3 << " more devices)\n";
}

// ============================================================================
// demo_tree_view - 演示：树形视图显示设备
//
// 以层级缩进格式显示 USB 设备树结构。
// ============================================================================
void demo_tree_view(UsbController& ctrl) {
    print_separator("3. USB Device Tree");
    std::cout << ctrl.list_devices_tree(); // 输出设备树
}

// ============================================================================
// demo_hid_list - 演示：列出 HID 设备
//
// 仅显示 HID 类设备（键盘、鼠标等）。
// ============================================================================
void demo_hid_list(UsbController& ctrl) {
    print_separator("4. HID Devices");
    std::cout << ctrl.list_hid_devices(); // 输出 HID 设备列表
}

// ============================================================================
// demo_open_hid_by_index - 演示：按索引打开 HID 设备
//
// 显示所有有 HID 接口的设备，让用户选择索引打开。
// ============================================================================
void demo_open_hid_by_index(UsbController& ctrl) {
    print_separator("5. Open HID by Index");
    auto& devs = ctrl.devices(); // 获取设备列表
    if (devs.empty()) { std::cout << "No devices.\n"; return; } // 无设备
    // 列出所有有 HID 接口的设备
    for (size_t i = 0; i < devs.size(); ++i)
        if (devs[i].has_hid_interface()) std::cout << "[" << i << "] " << devs[i].summary() << "\n";
    // 获取用户输入的索引
    std::cout << "Enter index: ";
    size_t idx;
    std::cin >> idx;
    // 尝试打开设备
    if (ctrl.open_hid_device(idx))
        std::cout << "Opened: " << ctrl.hid_device_info() << "\n"; // 打开成功
    else
        std::cout << "Failed to open HID device\n"; // 打开失败
}

// ============================================================================
// demo_open_hid_by_vid_pid - 演示：按 VID:PID 打开 HID 设备
//
// 让用户输入 VID 和 PID（十六进制），查找并打开匹配的 HID 设备。
// ============================================================================
void demo_open_hid_by_vid_pid(UsbController& ctrl) {
    print_separator("6. Open HID by VID:PID");
    uint16_t vid, pid;
    // 获取用户输入的 VID（十六进制）
    std::cout << "Enter VID (hex): 0x";
    std::cin >> std::hex >> vid >> std::dec;
    // 获取用户输入的 PID（十六进制）
    std::cout << "Enter PID (hex): 0x";
    std::cin >> std::hex >> pid >> std::dec;
    // 尝试按 VID:PID 打开 HID 设备
    if (ctrl.open_hid_device_by_vid_pid(vid, pid))
        std::cout << "Opened: " << ctrl.hid_device_info() << "\n";
    else
        std::cout << "No matching HID device found\n";
}

// ============================================================================
// demo_device_detail - 演示：显示设备详情
//
// 让用户选择设备索引，显示该设备的完整描述符信息。
// ============================================================================
void demo_device_detail(UsbController& ctrl) {
    print_separator("7. Device Detail");
    if (ctrl.device_count() == 0) { std::cout << "No devices.\n"; return; }
    // 获取用户输入的索引
    std::cout << "Enter device index (0-" << ctrl.device_count() - 1 << "): ";
    size_t idx;
    std::cin >> idx;
    std::cout << ctrl.device_detail(idx); // 输出设备详情
}

// ============================================================================
// demo_hid_read - 演示：同步 HID 读取
//
// 如果未打开 HID 设备，自动尝试打开第一个 HID 设备。
// 然后执行一次同步中断读取。
// ============================================================================
void demo_hid_read(UsbController& ctrl) {
    print_separator("8. HID Read Report");
    // 如果未打开 HID 设备，自动尝试打开
    if (!ctrl.is_hid_open()) {
        auto& devs = ctrl.devices();
        bool opened = false;
        // 遍历设备，打开第一个有 HID 接口的设备
        for (size_t i = 0; i < devs.size(); ++i) {
            if (devs[i].has_hid_interface() && ctrl.open_hid_device(i)) {
                std::cout << "Auto-opened [" << i << "] " << ctrl.hid_device_info() << "\n";
                opened = true;
                break;
            }
        }
        if (!opened) { std::cout << "Could not auto-open any HID device\n"; return; }
    }
    // 获取读取长度（默认 64 字节）
    int len = 64;
    std::cout << "Read length (" << len << "): ";
    std::string input;
    std::cin.ignore(); // 忽略之前输入留下的换行符
    std::getline(std::cin, input);
    if (!input.empty()) len = std::stoi(input); // 用户自定义长度

    // 执行同步 HID 读取
    auto result = ctrl.hid_read(len, 1000);
    print_transfer_result("Read", result); // 输出结果
}

// ============================================================================
// demo_hid_read_latest - 演示：读取最新的 HID 输入报告（先排空缓冲区）
//
// 与 demo_hid_read 的区别：
//   demo_hid_read 调用 hid_read()，只能拿到 libusb 缓冲区中最旧的数据包。
//   本函数调用 hid_read_latest()，先以短超时循环排空缓冲区，
//   返回最后一次成功读取的数据（即设备最新发送的数据）。
//
// 适用场景：设备持续发送数据（如每 1 秒发一次），但你只关心当前最新值。
// ============================================================================
void demo_hid_read_latest(UsbController& ctrl) {
    print_separator("18. HID Read Latest Report (drain buffer first)");
    // 如果未打开 HID 设备，自动尝试打开
    if (!ctrl.is_hid_open()) {
        auto& devs = ctrl.devices();
        bool opened = false;
        // 遍历设备，打开第一个有 HID 接口的设备
        for (size_t i = 0; i < devs.size(); ++i) {
            if (devs[i].has_hid_interface() && ctrl.open_hid_device(i)) {
                std::cout << "Auto-opened [" << i << "] " << ctrl.hid_device_info() << "\n";
                opened = true;
                break;
            }
        }
        if (!opened) { std::cout << "Could not auto-open any HID device\n"; return; }
    }
    // 获取读取长度（默认 64 字节）
    int len = 64;
    std::cout << "Read length (" << len << "): ";
    std::string input;
    std::cin.ignore(); // 忽略之前输入留下的换行符
    std::getline(std::cin, input);
    if (!input.empty()) len = std::stoi(input); // 用户自定义长度

    // 执行排空缓冲区后的最新数据读取
    // 默认每次排空读取超时 10ms，循环直到队列为空
    auto result = ctrl.hid_read_latest(len, 10);
    print_transfer_result("ReadLatest", result); // 输出结果
}

// ============================================================================
// demo_hid_write - 演示：同步 HID 写入
//
// 让用户输入十六进制数据，向已打开的 HID 设备发送输出报告。
// ============================================================================
void demo_hid_write(UsbController& ctrl) {
    print_separator("9. HID Write Report");
    if (!ctrl.is_hid_open()) {
        std::cout << "No HID device opened. Use option 5 or 6 first.\n"; // 提示先打开设备
        return;
    }
    // 获取用户输入的十六进制数据
    std::cout << "Enter hex data (e.g. 0001020304): ";
    std::string hex_str;
    std::cin >> hex_str;
    auto data = parse_hex(hex_str); // 解析十六进制字符串
    // 执行同步 HID 写入
    auto result = ctrl.hid_write(data, 1000);
    print_transfer_result("Write", result);
}

// ============================================================================
// demo_hid_get_feature - 演示：获取 HID 特性报告
//
// 通过控制传输读取 HID 设备的特性报告。
// ============================================================================
void demo_hid_get_feature(UsbController& ctrl) {
    print_separator("10. HID Get Feature Report");
    if (!ctrl.is_hid_open()) { std::cout << "No HID device opened.\n"; return; }
    // 获取报告 ID（默认 0）
    uint8_t report_id = 0;
    int len = 64;
    std::cout << "Report ID (0): ";
    std::string input;
    std::cin.ignore();
    std::getline(std::cin, input);
    if (!input.empty()) report_id = static_cast<uint8_t>(std::stoi(input, nullptr, 16));
    // 获取读取长度
    std::cout << "Length (64): ";
    std::getline(std::cin, input);
    if (!input.empty()) len = std::stoi(input);

    // 执行获取特性报告
    auto result = ctrl.hid_get_feature_report(report_id, len, 1000);
    print_transfer_result("GetFeature", result);
}

// ============================================================================
// demo_hid_send_feature - 演示：发送 HID 特性报告
//
// 通过控制传输向 HID 设备发送特性报告。
// ============================================================================
void demo_hid_send_feature(UsbController& ctrl) {
    print_separator("11. HID Send Feature Report");
    if (!ctrl.is_hid_open()) { std::cout << "No HID device opened.\n"; return; }
    // 获取用户输入的十六进制数据（第一个字节为报告 ID）
    std::cout << "Enter hex data (1st byte=reportID): ";
    std::string hex_str;
    std::cin >> hex_str;
    auto data = parse_hex(hex_str); // 解析十六进制字符串
    // 执行发送特性报告
    auto result = ctrl.hid_send_feature_report(data, 1000);
    print_transfer_result("SendFeature", result);
}

// ============================================================================
// demo_async_read - 演示：单次异步 HID 读取
//
// 提交一次异步中断读取请求，等待 3 秒后查看结果。
// 异步读取不会阻塞主线程。
// ============================================================================
void demo_async_read(UsbController& ctrl) {
    print_separator("12. HID Async Read (single)");
    if (!ctrl.is_hid_open()) { std::cout << "No HID device opened.\n"; return; }
    std::cout << ctrl.hid_device_info() << "\n\n"; // 显示设备信息

    // 提交异步读取请求
    ctrl.hid_read_async(64, [](const TransferResult& r) {
        // 回调函数：传输完成时调用
        std::cout << "  [Async Single] " << format_transfer_result(r, 32) << "\n";
    }, 2000);

    // 等待异步传输完成
    std::cout << "  Submitted async read, waiting 3s...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "  Pending: " << ctrl.async_pending_count() << "\n"; // 显示待处理数量
}

// ============================================================================
// demo_continuous_read - 演示：连续异步 HID 读取
//
// 启动连续异步读取，持续 5 秒后停止。
// 每次读取完成后自动提交下一次读取。
// ============================================================================
void demo_continuous_read(UsbController& ctrl) {
    print_separator("13. HID Continuous Read (5 seconds)");
    if (!ctrl.is_hid_open()) { std::cout << "No HID device opened.\n"; return; }

    // 读取计数器（原子变量，线程安全）
    std::atomic<int> count{0};
    // 启动连续读取
    ctrl.hid_read_continuous(64, [&count](const TransferResult& r) {
        // 回调函数：每次读取完成时调用
        if (r.success && r.bytes_transferred > 0) {
            ++count;
            std::cout << "  [#" << count << "] " << r.bytes_transferred
                      << " bytes: " << transfer::bytes_to_hex(r.data, 32) << "\n";
        }
    }, 500);

    // 持续 5 秒
    std::cout << "  Reading continuously for 5 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    ctrl.hid_stop_continuous(); // 停止连续读取
    std::cout << "  Stopped. Total reads: " << count << "\n"; // 显示总读取次数
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 等待最后的回调完成
}

// ============================================================================
// demo_bulk_read - 演示：原始端点批量读取
//
// 让用户指定端点地址和长度，执行原始批量读取。
// ============================================================================
void demo_bulk_read(UsbController& ctrl) {
    print_separator("14. Raw Bulk Read");
    if (!ctrl.is_hid_open()) { std::cout << "No device opened.\n"; return; }
    uint8_t ep;
    int len;
    // 获取端点地址（十六进制）
    std::cout << "Endpoint (hex): 0x";
    std::cin >> std::hex >> ep >> std::dec;
    // 获取读取长度
    std::cout << "Length: ";
    std::cin >> len;
    // 执行批量读取
    auto result = ctrl.bulk_read(ep, len, 2000);
    print_transfer_result("BulkRead", result);
}

// ============================================================================
// demo_bulk_write - 演示：原始端点批量写入
//
// 让用户指定端点地址和十六进制数据，执行原始批量写入。
// ============================================================================
void demo_bulk_write(UsbController& ctrl) {
    print_separator("15. Raw Bulk Write");
    if (!ctrl.is_hid_open()) { std::cout << "No device opened.\n"; return; }
    uint8_t ep;
    std::string hex_str;
    // 获取端点地址（十六进制）
    std::cout << "Endpoint (hex): 0x";
    std::cin >> std::hex >> ep >> std::dec;
    // 获取十六进制数据
    std::cout << "Hex data: ";
    std::cin >> hex_str;
    auto data = parse_hex(hex_str); // 解析数据
    // 执行批量写入
    auto result = ctrl.bulk_write(ep, data, 2000);
    print_transfer_result("BulkWrite", result);
}

// ============================================================================
// demo_find_device - 演示：按 VID:PID 查找设备
//
// 让用户输入 VID 和 PID，查找匹配的设备并显示。
// ============================================================================
void demo_find_device(UsbController& ctrl) {
    print_separator("16. Find by VID:PID");
    uint16_t vid, pid;
    // 获取 VID（十六进制）
    std::cout << "VID (hex): 0x";
    std::cin >> std::hex >> vid;
    // 获取 PID（十六进制）
    std::cout << "PID (hex): 0x";
    std::cin >> std::hex >> pid >> std::dec;

    // 查找匹配的设备
    auto devs = ctrl.find_by_vid_pid(vid, pid);
    if (devs.empty()) {
        std::cout << "  No matching devices.\n";
    } else {
        // 列出所有匹配的设备
        for (size_t i = 0; i < devs.size(); ++i)
            std::cout << "  [" << i << "] " << devs[i].summary() << "\n";
        std::cout << "  Total: " << devs.size() << " device(s)\n"; // 显示总数
    }
}

// ============================================================================
// demo_run_all - 演示：自动运行所有演示
//
// 自动执行一系列演示操作，无需用户交互。
// 包括：设备列表、HID 列表、打开 HID、同步读写、异步读写、连续读取等。
// ============================================================================
void demo_run_all(UsbController& ctrl) {
    std::cout << "\n  ===== AUTO DEMO MODE =====\n"; // 自动演示模式标题

    // 演示 1：列出所有设备
    demo_list_devices(ctrl);
    // 演示 2：列出 HID 设备
    demo_hid_list(ctrl);

    // 启动异步引擎
    ctrl.async_start();

    // 自动打开第一个 HID 设备
    auto& devs = ctrl.devices();
    bool hid_opened = false;
    for (size_t i = 0; i < devs.size(); ++i) {
        if (devs[i].has_hid_interface()) {
            std::cout << "\n  Trying to open [" << i << "] " << devs[i].summary() << "\n";
            if (ctrl.open_hid_device(i)) {
                std::cout << "  => Opened: " << ctrl.hid_device_info() << "\n";
                hid_opened = true;
                break;
            }
        }
    }

    // 如果成功打开 HID 设备，执行 I/O 演示
    if (hid_opened) {
        // 同步 HID 读取
        auto result = ctrl.hid_read(64, 1000);
        print_transfer_result("Read", result);

        // 读取最新 HID 报告（排空缓冲区）
        result = ctrl.hid_read_latest(64, 10);
        print_transfer_result("ReadLatest", result);

        // 同步 HID 写入（测试数据）
        std::vector<uint8_t> test = {0x00, 0x01, 0x02, 0x03};
        result = ctrl.hid_write(test, 1000);
        print_transfer_result("Write", result);

        // 单次异步 HID 读取
        ctrl.hid_read_async(64, [](const TransferResult& r) {
            std::cout << "  [Async] " << format_transfer_result(r, 16) << "\n";
        }, 2000);

        std::cout << "  Waiting for async completion...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3)); // 等待异步完成

        // 连续异步 HID 读取（3 秒）
        std::atomic<int> count{0};
        ctrl.hid_read_continuous(64, [&count](const TransferResult& r) {
            if (r.success && r.bytes_transferred > 0) {
                ++count;
                std::cout << "  [#" << count << "] " << r.bytes_transferred << " bytes\n";
            }
        }, 500);

        std::cout << "  Continuous read for 3 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
        ctrl.hid_stop_continuous(); // 停止连续读取
        std::cout << "  Continuous reads: " << count << "\n"; // 显示总读取次数

        // 获取 HID 特性报告
        result = ctrl.hid_get_feature_report(0, 64, 1000);
        print_transfer_result("GetFeature", result);

        // 关闭 HID 设备
        ctrl.close_hid_device();
    } else {
        std::cout << "  No HID device available for I/O demo\n"; // 无 HID 设备
    }

    // 停止异步引擎
    ctrl.async_stop();
    print_separator("Auto Demo Complete");
}

// ============================================================================
// ASCII 艺术字：USB CONTROLLER 横幅
// ============================================================================
static const char * _usb_controller_log = R"(
                                                                                      
     _____ _____ _____    _____ _____ _____ _____ _____ _____ __    __    _____ _____ 
    |  |  |   __| __  |  |     |     |   | |_   _| __  |     |  |  |  |  |   __| __  |
    |  |  |__   | __ -|  |   --|  |  | | | | | | |    -|  |  |  |__|  |__|   __|    -|
    |_____|_____|_____|  |_____|_____|_|___| |_| |__|__|_____|_____|_____|_____|__|__|
                                                                                      
)";

// ============================================================================
// main - 程序入口函数
//
// 创建 UsbController 实例，进入交互式菜单循环。
// 用户选择菜单项执行对应的演示功能。
//
// 异常处理：
//   捕获 std::exception，防止 libusb 初始化失败等异常导致程序崩溃。
//
// @return 0 表示正常退出，1 表示异常退出
// ============================================================================
int main() {
    try {
        // 打印 ASCII 艺术字横幅
        std::cout << _usb_controller_log << "\n";
        std::cout << "       USB Controller Demo v3.0 - Unified API Edition\n\n";

        // 创建 USB 控制器实例（自动初始化 libusb 并枚举设备）
        UsbController ctrl;
        ctrl.set_debug_level(0); // 关闭 libusb 调试输出
        ctrl.refresh_devices();  // 刷新设备列表
        std::cout << "Found " << ctrl.device_count() << " USB device(s)\n"; // 显示设备数量

        int choice;
        while (g_running) {
            print_menu(); // 显示菜单
            std::cin >> choice; // 获取用户选择
            // 输入验证：如果输入无效，清除错误状态并重试
            if (std::cin.fail()) { std::cin.clear(); std::cin.ignore(10000, '\n'); continue; }

            switch (choice) {
                case 1:  demo_list_devices(ctrl); break;       // 列出所有设备
                case 2:  demo_list_detail(ctrl); break;        // 设备详细信息
                case 3:  demo_tree_view(ctrl); break;          // 树形视图
                case 4:  demo_hid_list(ctrl); break;           // HID 设备列表
                case 5:  demo_open_hid_by_index(ctrl); break;  // 按索引打开 HID
                case 6:  demo_open_hid_by_vid_pid(ctrl); break;// 按 VID:PID 打开 HID
                case 7:  demo_device_detail(ctrl); break;      // 设备详情
                case 8:  demo_hid_read(ctrl); break;           // 同步 HID 读取
                case 9:  demo_hid_write(ctrl); break;          // 同步 HID 写入
                case 10: demo_hid_get_feature(ctrl); break;    // 获取特性报告
                case 11: demo_hid_send_feature(ctrl); break;   // 发送特性报告
                case 12: demo_async_read(ctrl); break;         // 单次异步读取
                case 13: demo_continuous_read(ctrl); break;    // 连续异步读取
                case 14: demo_bulk_read(ctrl); break;          // 原始批量读取
                case 15: demo_bulk_write(ctrl); break;         // 原始批量写入
                case 16: demo_find_device(ctrl); break;        // 按 VID:PID 查找
                case 17: demo_run_all(ctrl); break;            // 自动运行所有演示
                case 18: demo_hid_read_latest(ctrl); break; // 读取最新 HID 报告（排空缓冲区）
                case 0:  g_running = false; break;             // 退出程序
                default: std::cout << "Invalid choice\n"; break; // 无效选择
            }
        }

        // 程序退出前清理资源
        ctrl.close_hid_device(); // 关闭 HID 设备
        ctrl.async_stop();       // 停止异步引擎
        std::cout << "USB Controller exited.\n";

    } catch (const std::exception& e) {
        // 捕获并显示致命错误
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
