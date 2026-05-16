// ============================================================================
// test_auto_all.cpp - 自动运行所有测试
//
// 依次执行所有模块的测试，无需用户交互。
// ============================================================================

#include "common.h"

// ============================================================================
// 运行设备枚举测试
// ============================================================================
static int run_device_enum(UsbController& ctrl) {
    print_sep("Device Enumeration");
    int fail = 0;
    // 列出设备
    std::string list = ctrl.list_devices();
    std::cout << list;
    fail += list.empty() ? 1 : 0;
    // HID 设备
    std::cout << ctrl.list_hid_devices();
    // 设备数量
    std::cout << "  Device count: " << ctrl.device_count() << "\n";
    return fail;
}

// ============================================================================
// 运行 HID I/O 测试
// ============================================================================
static int run_hid_io(UsbController& ctrl) {
    print_sep("HID I/O");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    int fail = 0;
    // 同步读取
    auto r = ctrl.hid_read(64, 1000);
    print_result("Read", r);
    // 读取最新
    r = ctrl.hid_read_latest(64, 10);
    print_result("ReadLatest", r);
    // 同步写入
    std::vector<uint8_t> test = {0x00, 0x01, 0x02, 0x03};
    r = ctrl.hid_write(test, 1000);
    print_result("Write", r);
    // 特性报告
    r = ctrl.hid_get_feature_report(0, 64, 1000);
    print_result("GetFeature", r);
    ctrl.close_hid_device(); // 关闭设备
    return fail;
}

// ============================================================================
// 运行异步传输测试
// ============================================================================
static int run_async(UsbController& ctrl) {
    print_sep("Async Transfer");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    ctrl.async_start(); // 启动异步引擎
    // 单次异步读取
    ctrl.hid_read_async(64, [](const TransferResult& r) {
        std::cout << "  [Async] " << format_transfer_result(r, r.bytes_transferred) << "\n";
    }, 2000);
    std::cout << "  Waiting 3s for async...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    // 连续异步读取 3 秒
    std::atomic<int> count{0};
    ctrl.hid_read_continuous(64, [&count](const TransferResult& r) {
        if (r.success && r.bytes_transferred > 0) {
            ++count;
            std::cout << "  [#" << count << "] " << r.bytes_transferred << " bytes\n";
        }
    }, 500);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ctrl.hid_stop_continuous(); // 停止连续读取
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "  Continuous reads: " << count << "\n";
    ctrl.async_stop(); // 停止异步引擎
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// main - 自动运行所有测试
// ============================================================================
int main() {
    std::cout << ">>> Auto Run All Tests <<<\n";
    UsbController ctrl; // 创建控制器
    ctrl.set_debug_level(0); // 关闭调试
    ctrl.refresh_devices(); // 刷新设备
    std::cout << "Found " << ctrl.device_count() << " device(s)\n";

    int fail = 0; // 失败计数
    fail += run_device_enum(ctrl); // 设备枚举
    fail += run_hid_io(ctrl); // HID I/O
    fail += run_async(ctrl); // 异步传输

    print_sep("All Tests Complete");
    std::cout << "  Total failures: " << fail << "\n";
    return fail;
}
