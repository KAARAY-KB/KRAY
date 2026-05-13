#include "usb_controller.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>

using namespace usb_ctrl;

static std::atomic<bool> g_running{true};

void print_separator(const char* title = nullptr) {
    std::cout << "\n";
    if (title)
        std::cout << "===== " << title << " =====\n";
    else
        std::cout << "========================================\n";
}

void print_transfer_result(const char* label, const TransferResult& r) {
    std::cout << "  [" << label << "] " << format_transfer_result(r, 32) << "\n";
}

std::vector<uint8_t> parse_hex(const std::string& hex_str) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i + 1 < hex_str.length(); i += 2) {
        std::string byte = hex_str.substr(i, 2);
        result.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16)));
    }
    return result;
}

void print_menu() {
    std::cout << "\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|     USB Controller - Unified API     |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "|  1. List all USB devices             |\n";
    std::cout << "|  2. List devices (detailed)          |\n";
    std::cout << "|  3. List devices (tree view)         |\n";
    std::cout << "|  4. List HID devices                 |\n";
    std::cout << "|  5. Open HID device by index         |\n";
    std::cout << "|  6. Open HID device by VID:PID       |\n";
    std::cout << "|  7. Show device detail               |\n";
    std::cout << "|  8. HID read report (sync)           |\n";
    std::cout << "|  9. HID write report (sync)          |\n";
    std::cout << "| 10. HID get feature report           |\n";
    std::cout << "| 11. HID send feature report          |\n";
    std::cout << "| 12. HID read (single async)          |\n";
    std::cout << "| 13. HID read (continuous, 5s)        |\n";
    std::cout << "| 14. Raw endpoint bulk read           |\n";
    std::cout << "| 15. Raw endpoint bulk write          |\n";
    std::cout << "| 16. Find device by VID:PID           |\n";
    std::cout << "| 17. Run all demos (auto)             |\n";
    std::cout << "|  0. Exit                             |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "Choice: ";
}

void demo_list_devices(UsbController& ctrl) {
    print_separator("1. All USB Devices");
    std::cout << ctrl.list_devices();
}

void demo_list_detail(UsbController& ctrl) {
    print_separator("2. Devices Detailed");
    for (size_t i = 0; i < ctrl.device_count() && i < 3; ++i) {
        std::cout << ctrl.device_detail(i) << "\n";
    }
    if (ctrl.device_count() > 3)
        std::cout << "... (" << ctrl.device_count() - 3 << " more devices)\n";
}

void demo_tree_view(UsbController& ctrl) {
    print_separator("3. USB Device Tree");
    std::cout << ctrl.list_devices_tree();
}

void demo_hid_list(UsbController& ctrl) {
    print_separator("4. HID Devices");
    std::cout << ctrl.list_hid_devices();
}

void demo_open_hid_by_index(UsbController& ctrl) {
    print_separator("5. Open HID by Index");
    auto& devs = ctrl.devices();
    if (devs.empty()) { std::cout << "No devices.\n"; return; }
    for (size_t i = 0; i < devs.size(); ++i)
        if (devs[i].has_hid_interface()) std::cout << "[" << i << "] " << devs[i].summary() << "\n";
    std::cout << "Enter index: ";
    size_t idx;
    std::cin >> idx;
    if (ctrl.open_hid_device(idx))
        std::cout << "Opened: " << ctrl.hid_device_info() << "\n";
    else
        std::cout << "Failed to open HID device\n";
}

void demo_open_hid_by_vid_pid(UsbController& ctrl) {
    print_separator("6. Open HID by VID:PID");
    uint16_t vid, pid;
    std::cout << "Enter VID (hex): 0x";
    std::cin >> std::hex >> vid >> std::dec;
    std::cout << "Enter PID (hex): 0x";
    std::cin >> std::hex >> pid >> std::dec;
    if (ctrl.open_hid_device_by_vid_pid(vid, pid))
        std::cout << "Opened: " << ctrl.hid_device_info() << "\n";
    else
        std::cout << "No matching HID device found\n";
}

void demo_device_detail(UsbController& ctrl) {
    print_separator("7. Device Detail");
    if (ctrl.device_count() == 0) { std::cout << "No devices.\n"; return; }
    std::cout << "Enter device index (0-" << ctrl.device_count() - 1 << "): ";
    size_t idx;
    std::cin >> idx;
    std::cout << ctrl.device_detail(idx);
}

void demo_hid_read(UsbController& ctrl) {
    print_separator("8. HID Read Report");
    if (!ctrl.is_hid_open()) {
        auto& devs = ctrl.devices();
        bool opened = false;
        for (size_t i = 0; i < devs.size(); ++i) {
            if (devs[i].has_hid_interface() && ctrl.open_hid_device(i)) {
                std::cout << "Auto-opened [" << i << "] " << ctrl.hid_device_info() << "\n";
                opened = true;
                break;
            }
        }
        if (!opened) { std::cout << "Could not auto-open any HID device\n"; return; }
    }
    int len = 64;
    std::cout << "Read length (" << len << "): ";
    std::string input;
    std::cin.ignore();
    std::getline(std::cin, input);
    if (!input.empty()) len = std::stoi(input);

    auto result = ctrl.hid_read(len, 1000);
    print_transfer_result("Read", result);
}

void demo_hid_write(UsbController& ctrl) {
    print_separator("9. HID Write Report");
    if (!ctrl.is_hid_open()) {
        std::cout << "No HID device opened. Use option 5 or 6 first.\n";
        return;
    }
    std::cout << "Enter hex data (e.g. 0001020304): ";
    std::string hex_str;
    std::cin >> hex_str;
    auto data = parse_hex(hex_str);
    auto result = ctrl.hid_write(data, 1000);
    print_transfer_result("Write", result);
}

void demo_hid_get_feature(UsbController& ctrl) {
    print_separator("10. HID Get Feature Report");
    if (!ctrl.is_hid_open()) { std::cout << "No HID device opened.\n"; return; }
    uint8_t report_id = 0;
    int len = 64;
    std::cout << "Report ID (0): ";
    std::string input;
    std::cin.ignore();
    std::getline(std::cin, input);
    if (!input.empty()) report_id = static_cast<uint8_t>(std::stoi(input, nullptr, 16));
    std::cout << "Length (64): ";
    std::getline(std::cin, input);
    if (!input.empty()) len = std::stoi(input);

    auto result = ctrl.hid_get_feature_report(report_id, len, 1000);
    print_transfer_result("GetFeature", result);
}

void demo_hid_send_feature(UsbController& ctrl) {
    print_separator("11. HID Send Feature Report");
    if (!ctrl.is_hid_open()) { std::cout << "No HID device opened.\n"; return; }
    std::cout << "Enter hex data (1st byte=reportID): ";
    std::string hex_str;
    std::cin >> hex_str;
    auto data = parse_hex(hex_str);
    auto result = ctrl.hid_send_feature_report(data, 1000);
    print_transfer_result("SendFeature", result);
}

void demo_async_read(UsbController& ctrl) {
    print_separator("12. HID Async Read (single)");
    if (!ctrl.is_hid_open()) { std::cout << "No HID device opened.\n"; return; }
    std::cout << ctrl.hid_device_info() << "\n\n";

    ctrl.hid_read_async(64, [](const TransferResult& r) {
        std::cout << "  [Async Single] " << format_transfer_result(r, 32) << "\n";
    }, 2000);

    std::cout << "  Submitted async read, waiting 3s...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "  Pending: " << ctrl.async_pending_count() << "\n";
}

void demo_continuous_read(UsbController& ctrl) {
    print_separator("13. HID Continuous Read (5 seconds)");
    if (!ctrl.is_hid_open()) { std::cout << "No HID device opened.\n"; return; }

    std::atomic<int> count{0};
    ctrl.hid_read_continuous(64, [&count](const TransferResult& r) {
        if (r.success && r.bytes_transferred > 0) {
            ++count;
            std::cout << "  [#" << count << "] " << r.bytes_transferred
                      << " bytes: " << transfer::bytes_to_hex(r.data, 32) << "\n";
        }
    }, 500);

    std::cout << "  Reading continuously for 5 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    ctrl.hid_stop_continuous();
    std::cout << "  Stopped. Total reads: " << count << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

void demo_bulk_read(UsbController& ctrl) {
    print_separator("14. Raw Bulk Read");
    if (!ctrl.is_hid_open()) { std::cout << "No device opened.\n"; return; }
    uint8_t ep;
    int len;
    std::cout << "Endpoint (hex): 0x";
    std::cin >> std::hex >> ep >> std::dec;
    std::cout << "Length: ";
    std::cin >> len;
    auto result = ctrl.bulk_read(ep, len, 2000);
    print_transfer_result("BulkRead", result);
}

void demo_bulk_write(UsbController& ctrl) {
    print_separator("15. Raw Bulk Write");
    if (!ctrl.is_hid_open()) { std::cout << "No device opened.\n"; return; }
    uint8_t ep;
    std::string hex_str;
    std::cout << "Endpoint (hex): 0x";
    std::cin >> std::hex >> ep >> std::dec;
    std::cout << "Hex data: ";
    std::cin >> hex_str;
    auto data = parse_hex(hex_str);
    auto result = ctrl.bulk_write(ep, data, 2000);
    print_transfer_result("BulkWrite", result);
}

void demo_find_device(UsbController& ctrl) {
    print_separator("16. Find by VID:PID");
    uint16_t vid, pid;
    std::cout << "VID (hex): 0x";
    std::cin >> std::hex >> vid;
    std::cout << "PID (hex): 0x";
    std::cin >> std::hex >> pid >> std::dec;

    auto devs = ctrl.find_by_vid_pid(vid, pid);
    if (devs.empty()) {
        std::cout << "  No matching devices.\n";
    } else {
        for (size_t i = 0; i < devs.size(); ++i)
            std::cout << "  [" << i << "] " << devs[i].summary() << "\n";
        std::cout << "  Total: " << devs.size() << " device(s)\n";
    }
}

void demo_run_all(UsbController& ctrl) {
    std::cout << "\n  ===== AUTO DEMO MODE =====\n";

    demo_list_devices(ctrl);
    demo_hid_list(ctrl);

    ctrl.async_start();

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

    if (hid_opened) {
        auto result = ctrl.hid_read(64, 1000);
        print_transfer_result("Read", result);

        std::vector<uint8_t> test = {0x00, 0x01, 0x02, 0x03};
        result = ctrl.hid_write(test, 1000);
        print_transfer_result("Write", result);

        ctrl.hid_read_async(64, [](const TransferResult& r) {
            std::cout << "  [Async] " << format_transfer_result(r, 16) << "\n";
        }, 2000);

        std::cout << "  Waiting for async completion...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::atomic<int> count{0};
        ctrl.hid_read_continuous(64, [&count](const TransferResult& r) {
            if (r.success && r.bytes_transferred > 0) {
                ++count;
                std::cout << "  [#" << count << "] " << r.bytes_transferred << " bytes\n";
            }
        }, 500);

        std::cout << "  Continuous read for 3 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
        ctrl.hid_stop_continuous();
        std::cout << "  Continuous reads: " << count << "\n";

        result = ctrl.hid_get_feature_report(0, 64, 1000);
        print_transfer_result("GetFeature", result);

        ctrl.close_hid_device();
    } else {
        std::cout << "  No HID device available for I/O demo\n";
    }

    ctrl.async_stop();
    print_separator("Auto Demo Complete");
}
static const char * _usb_controller_log = R"(
                                                                                      
     _____ _____ _____    _____ _____ _____ _____ _____ _____ __    __    _____ _____ 
    |  |  |   __| __  |  |     |     |   | |_   _| __  |     |  |  |  |  |   __| __  |
    |  |  |__   | __ -|  |   --|  |  | | | | | | |    -|  |  |  |__|  |__|   __|    -|
    |_____|_____|_____|  |_____|_____|_|___| |_| |__|__|_____|_____|_____|_____|__|__|
                                                                                      
)";


int main() {
    try {
        std::cout << _usb_controller_log << "\n";
        std::cout << "       USB Controller Demo v3.0 - Unified API Edition\n\n";

        UsbController ctrl;
        ctrl.set_debug_level(0);
        ctrl.refresh_devices();
        std::cout << "Found " << ctrl.device_count() << " USB device(s)\n";

        int choice;
        while (g_running) {
            print_menu();
            std::cin >> choice;
            if (std::cin.fail()) { std::cin.clear(); std::cin.ignore(10000, '\n'); continue; }

            switch (choice) {
                case 1:  demo_list_devices(ctrl); break;
                case 2:  demo_list_detail(ctrl); break;
                case 3:  demo_tree_view(ctrl); break;
                case 4:  demo_hid_list(ctrl); break;
                case 5:  demo_open_hid_by_index(ctrl); break;
                case 6:  demo_open_hid_by_vid_pid(ctrl); break;
                case 7:  demo_device_detail(ctrl); break;
                case 8:  demo_hid_read(ctrl); break;
                case 9:  demo_hid_write(ctrl); break;
                case 10: demo_hid_get_feature(ctrl); break;
                case 11: demo_hid_send_feature(ctrl); break;
                case 12: demo_async_read(ctrl); break;
                case 13: demo_continuous_read(ctrl); break;
                case 14: demo_bulk_read(ctrl); break;
                case 15: demo_bulk_write(ctrl); break;
                case 16: demo_find_device(ctrl); break;
                case 17: demo_run_all(ctrl); break;
                case 0:  g_running = false; break;
                default: std::cout << "Invalid choice\n"; break;
            }
        }

        ctrl.close_hid_device();
        ctrl.async_stop();
        std::cout << "USB Controller exited.\n";

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
