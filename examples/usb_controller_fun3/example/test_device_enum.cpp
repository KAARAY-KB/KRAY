// ============================================================================
// test_device_enum.cpp - 设备枚举单元测试
//
// 测试内容：
//   1. 列出所有 USB 设备
//   2. 列出设备详细信息
//   3. 树形视图显示设备
//   4. 列出 HID 设备
//   5. 按 VID:PID 查找设备
// ============================================================================

#include "common.h"

// ============================================================================
// 测试：列出所有 USB 设备
// ============================================================================
int test_list_devices(UsbController& ctrl) {
    print_sep("List All USB Devices");
    std::string list = ctrl.list_devices(); // 获取设备列表
    std::cout << list; // 输出列表
    TEST_ASSERT(!list.empty(), "Device list not empty"); // 断言列表非空
    return 0;
}

// ============================================================================
// 测试：列出设备详细信息
// ============================================================================
int test_list_detail(UsbController& ctrl) {
    print_sep("Device Detailed Info");
    size_t count = ctrl.device_count(); // 设备数量
    TEST_ASSERT(count > 0, "At least one device exists"); // 断言有设备
    // 输出前 3 个设备的详情
    for (size_t i = 0; i < count && i < 3; ++i) {
        std::cout << ctrl.device_detail(i) << "\n";
    }
    if (count > 3)
        std::cout << "... (" << count - 3 << " more devices)\n";
    return 0;
}

// ============================================================================
// 测试：树形视图
// ============================================================================
int test_tree_view(UsbController& ctrl) {
    print_sep("USB Device Tree");
    std::string tree = ctrl.list_devices_tree(); // 获取设备树
    std::cout << tree; // 输出树形结构
    TEST_ASSERT(!tree.empty(), "Tree view not empty"); // 断言非空
    return 0;
}

// ============================================================================
// 测试：列出 HID 设备
// ============================================================================
int test_list_hid(UsbController& ctrl) {
    print_sep("HID Devices");
    std::string hid_list = ctrl.list_hid_devices(); // 获取 HID 设备列表
    std::cout << hid_list; // 输出列表
    return 0;
}

// ============================================================================
// 测试：按 VID:PID 查找设备
// ============================================================================
int test_find_by_vid_pid(UsbController& ctrl) {
    print_sep("Find Device by VID:PID");
    // 获取第一个设备的 VID:PID 用于测试
    if (ctrl.device_count() == 0) {
        std::cout << "  No devices to search\n";
        return 0;
    }
    auto& devs = ctrl.devices(); // 设备列表
    uint16_t vid = devs[0].vid(); // 第一个设备的 VID
    uint16_t pid = devs[0].pid(); // 第一个设备的 PID
    std::cout << "  Searching for VID=0x" << std::hex << vid
              << " PID=0x" << pid << std::dec << "\n";
    auto found = ctrl.find_by_vid_pid(vid, pid); // 查找设备
    TEST_ASSERT(!found.empty(), "Find by VID:PID returns results"); // 断言找到
    for (auto& d : found)
        std::cout << "  Found: " << d.summary() << "\n";
    return 0;
}

// ============================================================================
// main - 运行所有设备枚举测试
// ============================================================================
int main() {
    std::cout << ">>> USB Device Enumeration Tests <<<\n";
    UsbController ctrl; // 创建控制器
    ctrl.set_debug_level(0); // 关闭调试
    ctrl.refresh_devices(); // 刷新设备
    std::cout << "Found " << ctrl.device_count() << " device(s)\n";

    int fail = 0; // 失败计数
    fail += test_list_devices(ctrl); // 测试列出设备
    fail += test_list_detail(ctrl); // 测试设备详情
    fail += test_tree_view(ctrl); // 测试树形视图
    fail += test_list_hid(ctrl); // 测试 HID 列表
    fail += test_find_by_vid_pid(ctrl); // 测试查找设备

    print_sep("Summary");
    std::cout << "  Total failures: " << fail << "\n";
    return fail;
}
