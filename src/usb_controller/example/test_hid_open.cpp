// ============================================================================
// test_hid_open.cpp - HID 设备打开/关闭单元测试
//
// 测试内容：
//   1. 按索引打开 HID 设备
//   2. 按 VID:PID 打开 HID 设备
//   3. 检查 HID 设备状态
//   4. 关闭 HID 设备
// ============================================================================

#include "common.h"

// ============================================================================
// 测试：按索引打开/关闭 HID 设备
// ============================================================================
int test_open_by_index(UsbController& ctrl) {
    print_sep("Open HID by Index");
    auto hid_devs = ctrl.find_hid_devices(); // 查找 HID 设备
    if (hid_devs.empty()) {
        std::cout << "  No HID devices found, skip\n";
        return 0;
    }
    // 在设备列表中找到 HID 设备的索引
    auto& all_devs = ctrl.devices(); // 所有设备
    size_t hid_idx = 0;
    for (size_t i = 0; i < all_devs.size(); ++i) {
        if (all_devs[i].has_hid_interface()) {
            hid_idx = i; // 找到第一个 HID 设备的索引
            break;
        }
    }
    // 尝试打开
    bool ok = ctrl.open_hid_device(hid_idx); // 按索引打开
    if (ok) {
        std::cout << "  Opened: " << ctrl.hid_device_info() << "\n";
        TEST_ASSERT(ctrl.is_hid_open(), "HID device is open"); // 断言已打开
        ctrl.close_hid_device(); // 关闭设备
        TEST_ASSERT(!ctrl.is_hid_open(), "HID device is closed"); // 断言已关闭
    } else {
        std::cout << "  Failed to open (may need sudo)\n";
    }
    return 0;
}

// ============================================================================
// 测试：按 VID:PID 打开/关闭 HID 设备
// ============================================================================
int test_open_by_vid_pid(UsbController& ctrl) {
    print_sep("Open HID by VID:PID");
    auto hid_devs = ctrl.find_hid_devices(); // 查找 HID 设备
    if (hid_devs.empty()) {
        std::cout << "  No HID devices found, skip\n";
        return 0;
    }
    // 使用第一个 HID 设备的 VID:PID
    uint16_t vid = hid_devs[0].vid(); // VID
    uint16_t pid = hid_devs[0].pid(); // PID
    std::cout << "  Trying VID=0x" << std::hex << vid
              << " PID=0x" << pid << std::dec << "\n";
    bool ok = ctrl.open_hid_device_by_vid_pid(vid, pid); // 按 VID:PID 打开
    if (ok) {
        std::cout << "  Opened: " << ctrl.hid_device_info() << "\n";
        TEST_ASSERT(ctrl.is_hid_open(), "HID device is open"); // 断言已打开
        ctrl.close_hid_device(); // 关闭设备
        TEST_ASSERT(!ctrl.is_hid_open(), "HID device is closed"); // 断言已关闭
    } else {
        std::cout << "  Failed to open (may need sudo)\n";
    }
    return 0;
}

// ============================================================================
// 测试：HID 设备信息查询
// ============================================================================
int test_hid_info(UsbController& ctrl) {
    print_sep("HID Device Info");
    if (!auto_open_hid(ctrl)) return 0; // 自动打开
    std::string info = ctrl.hid_device_info(); // 获取设备信息
    std::cout << "  Info: " << info << "\n";
    TEST_ASSERT(!info.empty(), "HID info not empty"); // 断言信息非空
    ctrl.close_hid_device(); // 关闭设备
    return 0;
}

// ============================================================================
// main - 运行所有 HID 打开/关闭测试
// ============================================================================
int main() {
    std::cout << ">>> HID Device Open/Close Tests <<<\n";
    UsbController ctrl; // 创建控制器
    ctrl.set_debug_level(0); // 关闭调试
    ctrl.refresh_devices(); // 刷新设备
    std::cout << "Found " << ctrl.device_count() << " device(s)\n";

    int fail = 0; // 失败计数
    fail += test_open_by_index(ctrl); // 测试按索引打开
    fail += test_open_by_vid_pid(ctrl); // 测试按 VID:PID 打开
    fail += test_hid_info(ctrl); // 测试设备信息

    print_sep("Summary");
    std::cout << "  Total failures: " << fail << "\n";
    return fail;
}
