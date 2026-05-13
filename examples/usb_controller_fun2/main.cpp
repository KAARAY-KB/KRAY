#include "usb_controller.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

using namespace usb_ctrl;

// 全局运行标志：用于控制程序运行状态
static std::atomic<bool> g_running{true};

// 打印分隔线
// title: 可选的标题文本，如果提供则显示带标题的分隔线
void print_separator(const char* title = nullptr) {
    std::cout << "\n";
    if (title) {
        std::cout << "===== " << title << " =====\n";
    } else {
        std::cout << "========================================\n";
    }
}

// 打印传输结果信息
// label: 操作标签 (如 "Read", "Write")
// r: 传输结果对象
void print_transfer_result(const char* label, const TransferResult& r) {
    std::cout << "  " << label << ": ";
    if (r.success) {
        std::cout << "SUCCESS, " << r.bytes_transferred << " bytes";
        if (!r.data.empty()) {
            std::cout << "\n    Hex: " << usb_transfer::bytes_to_hex(r.data, 64);
            std::cout << "\n    ASCII: " << usb_transfer::bytes_to_ascii(r.data, 64);
        }
    } else {
        std::cout << "FAILED: " << r.error_message;
    }
    std::cout << "\n";
}

// 演示 1：列出所有 USB 设备
// 显示所有设备的摘要信息和 HID 设备列表
void demo_list_devices(UsbController& ctrl) {
    print_separator("1. List All USB Devices (Summary)");
    std::cout << ctrl.list_devices();

    print_separator("2. List HID Devices");
    std::cout << ctrl.list_hid_devices();
}

// 演示 2：显示设备详细信息
// 显示第一个设备的完整描述符信息
void demo_device_detail(UsbController& ctrl) {
    print_separator("3. Device Detail (first device if available)");
    if (ctrl.device_count() > 0) {
        std::cout << ctrl.device_detail(0);
    } else {
        std::cout << "No devices found.\n";
    }
}

// 演示 3：HID 同步 I/O 操作
// 演示同步读取、写入和获取特性报告
void demo_hid_sync_io(UsbController& ctrl) {
    print_separator("4. HID Synchronous I/O Demo");

    // 如果尚未打开 HID 设备，尝试自动打开第一个 HID 设备
    if (!ctrl.is_hid_open()) {
        std::cout << "No HID device opened. Trying to auto-open first HID device...\n";
        auto& devs = ctrl.devices();
        bool opened = false;
        for (size_t i = 0; i < devs.size(); ++i) {
            if (devs[i].has_hid_interface()) {
                if (ctrl.open_hid_device(i)) {
                    opened = true;
                    std::cout << "Opened HID device at index " << i << "\n";
                    break;
                }
            }
        }
        if (!opened) {
            std::cout << "No HID device available to open.\n";
            return;
        }
    }

    std::cout << ctrl.hid_device_info() << "\n\n";

    // 演示同步读取 HID 报告
    std::cout << "  Reading HID report (64 bytes, 1s timeout)...\n";
    auto result = ctrl.hid_read(64, 1000);
    print_transfer_result("Read", result);

    // 演示同步写入 HID 报告
    std::cout << "  Writing HID report (test data)...\n";
    std::vector<uint8_t> test_data = {0x00, 0x01, 0x02, 0x03};
    result = ctrl.hid_write(test_data, 1000);
    print_transfer_result("Write", result);

    // 演示获取 HID 特性报告
    std::cout << "  Getting feature report (report_id=0)...\n";
    result = ctrl.hid_get_feature_report(0, 64, 1000);
    print_transfer_result("GetFeature", result);
}

// 演示 4：HID 异步 I/O 操作
// 演示单次异步读取操作
void demo_hid_async_io(UsbController& ctrl) {
    print_separator("5. HID Asynchronous I/O Demo");

    if (!ctrl.is_hid_open()) {
        std::cout << "No HID device opened. Skipping async demo.\n";
        return;
    }

    std::cout << ctrl.hid_device_info() << "\n\n";

    // 提交异步读取请求 (64 字节，2 秒超时)
    std::cout << "  Submitting async read (64 bytes, 2s timeout)...\n";
    ctrl.hid_read_async(64, [](const TransferResult& r) {
        std::cout << "  [Async Callback] Read result: ";
        if (r.success) {
            std::cout << "SUCCESS, " << r.bytes_transferred << " bytes";
            if (!r.data.empty()) {
                std::cout << "\n    Hex: " << usb_transfer::bytes_to_hex(r.data, 32);
            }
        } else {
            std::cout << "FAILED: " << r.error_message;
        }
        std::cout << "\n";
    }, 2000);

    // 等待 3 秒让异步操作完成
    std::cout << "  Waiting 3 seconds for async completion...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "  Pending async operations: " << ctrl.async_pending_count() << "\n";
}

// 演示 5：HID 连续读取操作
// 演示连续异步读取 5 秒钟
void demo_continuous_read(UsbController& ctrl) {
    print_separator("6. HID Continuous Read Demo (5 seconds)");

    if (!ctrl.is_hid_open()) {
        std::cout << "No HID device opened. Skipping continuous read demo.\n";
        return;
    }

    std::atomic<int> read_count{0};

    // 启动连续读取，每次读取 64 字节
    ctrl.hid_read_continuous(64, [&read_count](const TransferResult& r) {
        if (r.success && r.bytes_transferred > 0) {
            ++read_count;
            std::cout << "  [Continuous #" << read_count << "] "
                      << r.bytes_transferred << " bytes: "
                      << usb_transfer::bytes_to_hex(r.data, 32) << "\n";
        }
    }, 500);

    // 运行 5 秒钟
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 停止连续读取
    ctrl.hid_stop_continuous();
    std::cout << "  Continuous read stopped. Total reads: " << read_count << "\n";

    // 等待异步操作清理完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// 演示 6：根据 VID/PID 查找设备
// 演示使用厂商 ID 和产品 ID 查找特定设备
void demo_find_device(UsbController& ctrl) {
    print_separator("7. Find Device by VID:PID");

    uint16_t vid = 0x1366;
    uint16_t pid = 0x1024;
    std::cout << "  Searching for VID:0x" << std::hex << std::uppercase
              << std::setw(4) << std::setfill('0') << vid
              << " PID:0x" << std::setw(4) << pid << std::dec << "\n";

    auto devices = ctrl.find_by_vid_pid(vid, pid);
    if (devices.empty()) {
        std::cout << "  No matching devices found.\n";
    } else {
        for (const auto& dev : devices) {
            std::cout << "  Found: " << dev.summary() << "\n";
        }
    }
}

// 主函数：USB 控制器演示程序入口
int main() {
    try {
        std::cout << "USB Controller - Demo Application\n";
        std::cout << "==================================\n";

        // 创建 USB 控制器实例
        UsbController ctrl;
        ctrl.set_debug_level(0);

        // 刷新设备列表
        ctrl.refresh_devices();
        std::cout << "Found " << ctrl.device_count() << " USB device(s)\n";

        // 运行各项演示
        demo_list_devices(ctrl);
        demo_device_detail(ctrl);

        // 启动异步传输引擎
        ctrl.async_start();

        demo_hid_sync_io(ctrl);
        demo_hid_async_io(ctrl);
        demo_continuous_read(ctrl);
        demo_find_device(ctrl);

        // 清理资源
        ctrl.close_hid_device();
        ctrl.async_stop();

        print_separator("Demo Complete");
        std::cout << "All demonstrations finished successfully.\n";

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
