// ============================================================================
// test_hotplug.cpp - USB 热插拔监听单元测试
//
// 测试内容：
//   1. 检查平台是否支持热插拔
//   2. 启动热插拔监听
//   3. 等待热插拔事件（5 秒超时）
//   4. 停止热插拔监听
// ============================================================================

#include "common.h"
#include <libusb.h> // libusb_get_version

// ============================================================================
// 测试：热插拔监听
// ============================================================================
int test_hotplug(UsbController& ctrl) {
    print_sep("Hotplug Monitor");

    // 显示 libusb 版本
    const libusb_version* ver = libusb_get_version(); // 获取版本
    std::cout << "  libusb version: "
              << ver->major << "." << ver->minor << "." << ver->micro << "\n";

    // 检查平台支持
    if (!UsbController::is_hotplug_supported()) {
        std::cout << "  Hotplug NOT supported on this platform\n";
        std::cout << "  (Requires libusb >= 1.0.22)\n";
        return 0;
    }
    TEST_ASSERT(true, "Hotplug is supported"); // 断言支持

    // 事件计数器
    std::atomic<int> arrive{0}; // 插入计数
    std::atomic<int> leave{0};  // 拔出计数

    // 启动热插拔监听
    bool ok = ctrl.hotplug_start([&arrive, &leave](HotplugEvent ev, UsbDevice& dev) {
        // 热插拔回调
        if (ev == HotplugEvent::Arrived) {
            ++arrive;
            std::cout << "  [+] Device arrived: " << dev.summary() << "\n";
        } else {
            ++leave;
            std::cout << "  [-] Device left:    " << dev.summary() << "\n";
        }
    });
    TEST_ASSERT(ok, "Hotplug start success"); // 断言启动成功

    if (!ok) return 1;

    std::cout << "  Monitoring for 5 seconds...\n";
    std::cout << "  (Plug/unplug USB devices to trigger events)\n";
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 监听 5 秒

    ctrl.hotplug_stop(); // 停止监听
    std::cout << "  Stopped. Arrived: " << arrive << ", Left: " << leave << "\n";
    TEST_ASSERT(ctrl.is_hotplug_listening() == false, "Hotplug stopped"); // 断言已停止
    return 0;
}

// ============================================================================
// main - 运行热插拔测试
// ============================================================================
int main() {
    std::cout << ">>> Hotplug Monitor Test <<<\n";
    UsbController ctrl; // 创建控制器
    ctrl.set_debug_level(0); // 关闭调试
    ctrl.refresh_devices(); // 刷新设备

    int fail = test_hotplug(ctrl); // 运行测试

    print_sep("Summary");
    std::cout << "  Total failures: " << fail << "\n";
    return fail;
}
