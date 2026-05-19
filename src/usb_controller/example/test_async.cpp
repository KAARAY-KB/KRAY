// ============================================================================
// test_async.cpp - 异步传输单元测试
//
// 测试内容：
//   1. 单次异步 HID 读取
//   2. 连续异步 HID 读取（5 秒）
//   3. 连续异步读取（手动启停）
// ============================================================================

#include "common.h"

// ============================================================================
// 测试：单次异步 HID 读取
// ============================================================================
int test_async_single(UsbController& ctrl) {
    print_sep("Async Single Read");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    ctrl.async_start(); // 启动异步引擎
    std::cout << "  Submitting async read...\n";
    ctrl.hid_read_async(64, [](const TransferResult& r) {
        // 异步读取完成回调
        std::cout << "  [Async] " << format_transfer_result(r, r.bytes_transferred) << "\n";
    }, 2000);
    std::cout << "  Waiting 3s for completion...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3)); // 等待完成
    std::cout << "  Pending: " << ctrl.async_pending_count() << "\n";
    ctrl.async_stop(); // 停止异步引擎
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// 测试：连续异步 HID 读取（5 秒）
// ============================================================================
int test_async_continuous(UsbController& ctrl) {
    print_sep("Async Continuous Read (5s)");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    ctrl.async_start(); // 启动异步引擎
    std::atomic<int> count{0}; // 读取计数
    ctrl.hid_read_continuous(64, [&count](const TransferResult& r) {
        // 连续读取回调
        if (r.success && r.bytes_transferred > 0) {
            ++count;
            std::cout << "  [#" << count << "] "
                      << format_transfer_result(r, r.bytes_transferred) << "\n";
        }
    }, 500);
    std::cout << "  Reading for 5 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 持续 5 秒
    ctrl.hid_stop_continuous(); // 停止连续读取
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 等待回调完成
    std::cout << "  Total reads: " << count << "\n";
    ctrl.async_stop(); // 停止异步引擎
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// 测试：连续异步读取（手动启停，3 秒后停止）
// ============================================================================
int test_async_manual(UsbController& ctrl) {
    print_sep("Async Manual Start/Stop (3s)");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    ctrl.async_start(); // 启动异步引擎
    std::atomic<int> count{0}; // 读取计数
    ctrl.hid_read_continuous(64, [&count](const TransferResult& r) {
        // 连续读取回调
        if (r.success && r.bytes_transferred > 0) {
            ++count;
            std::cout << "  [#" << count << "] "
                      << r.bytes_transferred << " bytes\n";
        }
    }, 500);
    std::cout << "  Started, will stop after 3s...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3)); // 3 秒后停止
    ctrl.hid_stop_continuous(); // 停止连续读取
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 等待回调
    std::cout << "  Stopped. Total: " << count << "\n";
    ctrl.async_stop(); // 停止异步引擎
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// main - 运行所有异步传输测试
// ============================================================================
int main() {
    std::cout << ">>> Async Transfer Tests <<<\n";
    UsbController ctrl; // 创建控制器
    ctrl.set_debug_level(0); // 关闭调试
    ctrl.refresh_devices(); // 刷新设备
    std::cout << "Found " << ctrl.device_count() << " device(s)\n";

    int fail = 0; // 失败计数
    fail += test_async_single(ctrl); // 测试单次异步
    fail += test_async_continuous(ctrl); // 测试连续异步 5s
    fail += test_async_manual(ctrl); // 测试手动启停

    print_sep("Summary");
    std::cout << "  Total failures: " << fail << "\n";
    return fail;
}
