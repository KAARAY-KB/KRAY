// ============================================================================
// test_hid_io.cpp - HID 数据读写单元测试
//
// 测试内容：
//   1. 同步 HID 读取
//   2. 同步 HID 写入
//   3. 读取最新 HID 报告（排空缓冲区）
//   4. 获取 HID 特性报告
//   5. 发送 HID 特性报告
// ============================================================================

#include "common.h"

// ============================================================================
// 测试：同步 HID 读取
// ============================================================================
int test_hid_read(UsbController& ctrl) {
    print_sep("HID Sync Read");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    auto result = ctrl.hid_read(64, 1000); // 同步读取 64 字节，超时 1s
    print_result("Read", result); // 输出结果
    // 超时不算失败（设备可能不主动发送数据）
    if (result.success) {
        TEST_ASSERT(result.bytes_transferred > 0, "Read got data"); // 断言读到数据
    } else {
        std::cout << "  (Timeout is OK if device doesn't send)\n";
    }
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// 测试：同步 HID 写入
// ============================================================================
int test_hid_write(UsbController& ctrl) {
    print_sep("HID Sync Write");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    std::vector<uint8_t> test_data = {0x00, 0x01, 0x02, 0x03}; // 测试数据
    auto result = ctrl.hid_write(test_data, 1000); // 同步写入
    print_result("Write", result); // 输出结果
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// 测试：读取最新 HID 报告（排空缓冲区）
// ============================================================================
int test_hid_read_latest(UsbController& ctrl) {
    print_sep("HID Read Latest (drain buffer)");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    auto result = ctrl.hid_read_latest(64, 10); // 排空后读取最新
    print_result("ReadLatest", result); // 输出结果
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// 测试：获取 HID 特性报告
// ============================================================================
int test_hid_get_feature(UsbController& ctrl) {
    print_sep("HID Get Feature Report");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    auto result = ctrl.hid_get_feature_report(0, 64, 1000); // 获取特性报告
    print_result("GetFeature", result); // 输出结果
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// 测试：发送 HID 特性报告
// ============================================================================
int test_hid_send_feature(UsbController& ctrl) {
    print_sep("HID Send Feature Report");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    std::vector<uint8_t> test_data = {0x00, 0x01, 0x02}; // 测试数据
    auto result = ctrl.hid_send_feature_report(test_data, 1000); // 发送特性报告
    print_result("SendFeature", result); // 输出结果
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// main - 运行所有 HID I/O 测试
// ============================================================================
int main() {
    std::cout << ">>> HID I/O Tests <<<\n";
    UsbController ctrl; // 创建控制器
    ctrl.set_debug_level(0); // 关闭调试
    ctrl.refresh_devices(); // 刷新设备
    std::cout << "Found " << ctrl.device_count() << " device(s)\n";

    int fail = 0; // 失败计数
    fail += test_hid_read(ctrl); // 测试同步读取
    fail += test_hid_write(ctrl); // 测试同步写入
    fail += test_hid_read_latest(ctrl); // 测试读取最新
    fail += test_hid_get_feature(ctrl); // 测试获取特性报告
    fail += test_hid_send_feature(ctrl); // 测试发送特性报告

    print_sep("Summary");
    std::cout << "  Total failures: " << fail << "\n";
    return fail;
}
