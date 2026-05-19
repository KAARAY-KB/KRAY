// ============================================================================
// test_bulk.cpp - 批量传输单元测试
//
// 测试内容：
//   1. 原始端点批量读取
//   2. 原始端点批量写入
// ============================================================================

#include "common.h"

// ============================================================================
// 测试：批量读取
// ============================================================================
int test_bulk_read(UsbController& ctrl) {
    print_sep("Bulk Read");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    // 使用 HID 设备的 IN 端点进行批量读取测试
    auto* hid = ctrl.hid_device(); // 获取 HID 设备
    if (!hid) { std::cout << "  No HID device\n"; return 0; }
    uint8_t in_ep = hid->in_endpoint().address; // IN 端点地址
    std::cout << "  Using IN endpoint: 0x" << std::hex << (int)in_ep << std::dec << "\n";
    auto result = ctrl.bulk_read(in_ep, 64, 2000); // 批量读取
    print_result("BulkRead", result); // 输出结果
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// 测试：批量写入
// ============================================================================
int test_bulk_write(UsbController& ctrl) {
    print_sep("Bulk Write");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    auto* hid = ctrl.hid_device(); // 获取 HID 设备
    if (!hid) { std::cout << "  No HID device\n"; return 0; }
    uint8_t out_ep = hid->out_endpoint().address; // OUT 端点地址
    if (out_ep == 0) {
        std::cout << "  No OUT endpoint, skip\n"; // 无 OUT 端点
        ctrl.close_hid_device();
        return 0;
    }
    std::cout << "  Using OUT endpoint: 0x" << std::hex << (int)out_ep << std::dec << "\n";
    std::vector<uint8_t> test_data = {0x00, 0x01, 0x02, 0x03}; // 测试数据
    auto result = ctrl.bulk_write(out_ep, test_data, 2000); // 批量写入
    print_result("BulkWrite", result); // 输出结果
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// main - 运行所有批量传输测试
// ============================================================================
int main() {
    std::cout << ">>> Bulk Transfer Tests <<<\n";
    UsbController ctrl; // 创建控制器
    ctrl.set_debug_level(0); // 关闭调试
    ctrl.refresh_devices(); // 刷新设备
    std::cout << "Found " << ctrl.device_count() << " device(s)\n";

    int fail = 0; // 失败计数
    fail += test_bulk_read(ctrl); // 测试批量读取
    fail += test_bulk_write(ctrl); // 测试批量写入

    print_sep("Summary");
    std::cout << "  Total failures: " << fail << "\n";
    return fail;
}
