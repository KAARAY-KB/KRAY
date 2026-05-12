#include "core/UsbContext.h"
#include "core/UsbDevice.h"
#include "device/UsbDeviceInfo.h"
#include "hid/HidInterface.h"
#include "async/AsyncTransfer.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>
#include <unistd.h>

using namespace std;

// 全局运行标志，用于控制主循环
atomic<bool> g_running(true);

// 打印菜单
// 显示所有可用的操作选项
void printMenu()
{
    cout << "\n========== USB Controller Menu ==========" << endl;
    cout << "1. List all USB devices" << endl;
    cout << "2. Open device by VID/PID" << endl;
    cout << "3. Print device info" << endl;
    cout << "4. Start async read on endpoint" << endl;
    cout << "5. Send data to endpoint" << endl;
    cout << "6. Stop async read" << endl;
    cout << "0. Exit" << endl;
    cout << "=========================================\n" << endl;
}

// 打印十六进制数据
// 参数: data - 要打印的数据向量
void printHex(const vector<uint8_t>& data)
{
    for (size_t i = 0; i < data.size(); ++i) {
        cout << hex << setw(2) << setfill('0') << static_cast<int>(data[i]);
        // 每16字节换行
        if ((i + 1) % 16 == 0) {
            cout << endl;
        } else {
            cout << " ";
        }
    }
    cout << dec << endl;
}

// 主函数：USB控制器程序入口
int main()
{
    UsbContext context;              // USB上下文管理器
    UsbDevice device;                // USB设备操作对象
    unique_ptr<AsyncTransfer> async_transfer;  // 异步传输管理器
    unique_ptr<HidInterface> hid_iface;        // HID接口操作对象

    // 初始化USB上下文
    if (!context.init()) {
        cerr << "Failed to initialize USB context" << endl;
        return -1;
    }

    cout << "USB Controller initialized" << endl;

    int choice;
    // 主循环：处理用户输入
    while (g_running) {
        printMenu();
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {
            // 选项1：列出所有USB设备
            case 1: {
                auto devices = context.enumerateDevices();  // 枚举设备
                cout << "\nFound " << devices.size() << " device(s):\n" << endl;

                // 遍历并打印每个设备信息
                for (size_t i = 0; i < devices.size(); ++i) {
                    libusb_device* dev = devices[i];
                    libusb_device_descriptor desc;
                    int ret = libusb_get_device_descriptor(dev, &desc);

                    cout << "[" << i << "] Bus " << static_cast<int>(libusb_get_bus_number(dev))
                         << ", Device " << static_cast<int>(libusb_get_device_address(dev));

                    // 如果成功获取描述符，打印VID/PID
                    if (ret == LIBUSB_SUCCESS) {
                        cout << " VID:PID = " << hex << setw(4) << setfill('0') << desc.idVendor
                             << ":" << setw(4) << desc.idProduct << dec;
                    }
                    cout << endl;
                }
                break;
            }

            // 选项2：按VID/PID打开设备
            case 2: {
                uint16_t vid = 0x1915, pid = 0x520c;
                cout << " VID (hex): " << hex << vid << endl;
                //cin >> hex >> vid;
                cout << " PID (hex): " << hex << pid << endl;
                //cin >> hex >> pid;
                cout << dec;

                auto devices = context.enumerateDevices();  // 枚举设备
                bool found = false;

                // 查找匹配VID/PID的设备
                for (auto dev : devices) {
                    libusb_device_descriptor desc;
                    int ret = libusb_get_device_descriptor(dev, &desc);

                    // 检查VID/PID是否匹配
                    if (ret == LIBUSB_SUCCESS && desc.idVendor == vid && desc.idProduct == pid) {
                        if (device.open(dev)) {  // 打开设备
                            // 设置设备配置（使用默认配置1）
                            device.setConfiguration(1);
                            cout << "Device opened successfully" << endl;
                            found = true;
                        } else {
                            cerr << "Failed to open device" << endl;
                        }
                        break;
                    }
                }

                if (!found) {
                    cerr << "Device not found" << endl;
                }
                break;
            }

            // 选项3：打印设备详细信息
            case 3: {
                if (!device.isOpen()) {
                    cerr << "No device opened" << endl;
                    break;
                }

                // 解析并打印设备信息
                libusb_device* dev = libusb_get_device(device.getHandle());
                DeviceInfo info = UsbDeviceInfo::parse(dev, device.getHandle());
                UsbDeviceInfo::print(info);
                break;
            }

            // 选项4：启动异步读取
            case 4: {
                if (!device.isOpen()) {
                    cerr << "No device opened" << endl;
                    break;
                }

                uint8_t ep;           // 端点地址
                uint16_t max_pkt;     // 最大包大小
                int type;             // 传输类型
                int iface_num;        // 接口编号

                cout << "Enter interface number: ";
                //cin >> dec >> iface_num;
                iface_num = 0;
                cout << "Enter endpoint address (hex, IN endpoint e.g. 0x81): ";
                //cin >> hex >> ep;
                ep = 0x81;
                cout << "Enter max packet size: ";
                //cin >> dec >> max_pkt;
                max_pkt = 64;
                cout << "Max packet size: " << max_pkt << endl;
                cout << "Transfer type (0=Bulk, 1=Interrupt): ";
                //cin >> type;
                type = 1;

                // 声明接口（必须先声明接口才能使用端点）
                // 先分离内核驱动（Linux下需要）
                device.detachKernelDriver(iface_num);
                if (!device.claimInterface(iface_num)) {
                    cerr << "Failed to claim interface " << iface_num << endl;
                    break;
                }
                cout << "Interface " << iface_num << " claimed" << endl;

                // 创建异步传输管理器（如果不存在）
                if (!async_transfer) {
                    async_transfer = make_unique<AsyncTransfer>(context, device);
                }

                // 确定传输类型
                TransferType ttype = (type == 0) ? TransferType::BULK : TransferType::INTERRUPT;

                // 定义数据接收回调函数
                auto callback = [](const TransferResult& result) {
                    if (result.status == LIBUSB_TRANSFER_COMPLETED) {
                        static uint8_t cnt = 0;
                        if (cnt > 0) {
                       //     return;
                        }
                        cnt++;
                        cout << "\n[Async RX] Received " << result.actual_length << " bytes: ";
                        printHex(result.data);
                        cout << "> ";
                        cout.flush();
                    } else {
                        cout << "\n[Async RX] Transfer status: " << result.status << endl;
                        cout << "> ";
                        cout.flush();
                    }
                };

                // 启动异步读取
                if (async_transfer->startRead(ep, max_pkt, ttype, callback)) {
                    cout << "Async read started on endpoint 0x" << hex << static_cast<int>(ep) << dec << endl;
                    // 启动事件循环线程
                    async_transfer->startEventLoopThread();
                } else {
                    cerr << "Failed to start async read" << endl;
                    device.releaseInterface(iface_num);
                }
                break;
            }

            // 选项5：发送数据到端点
            case 5: {
                if (!device.isOpen()) {
                    cerr << "No device opened" << endl;
                    break;
                }

                uint8_t ep = 0x01;           // 端点地址
                int type = 1;             // 传输类型
                string hex_str = "01020304";       // 十六进制数据字符串

                cout << " endpoint address (hex, OUT endpoint e.g. 0x01): " << hex << ep << dec << endl;
                //cin >> hex >> ep;
                //cout << dec;
                cout << "Transfer type (0=Bulk, 1=Interrupt): " << type << endl;
                //cin >> type;
                type = 1;
                cout << " data (hex, e.g. 01020304): " << hex_str << endl;
                //cin >> hex_str;



                // 解析十六进制字符串为字节数据
                vector<uint8_t> data;
                for (size_t i = 0; i < hex_str.length(); i += 2) {
                    string byte = hex_str.substr(i, 2);
                    data.push_back(static_cast<uint8_t>(stoi(byte, nullptr, 16)));
                }
                cout << "Data to send: ";
                printHex(data);
                cout << endl;

                // 确定传输类型
                TransferType ttype = (type == 0) ? TransferType::BULK : TransferType::INTERRUPT;

                int transferred = 0;
                int ret;

                // 根据传输类型调用不同的传输函数
                if (ttype == TransferType::BULK) {
                    ret = device.bulkTransfer(ep, data.data(), data.size(), &transferred, 1000);
                } else {
                    ret = device.interruptTransfer(ep, data.data(), data.size(), &transferred, 1000);
                }

                // 打印传输结果
                if (ret == LIBUSB_SUCCESS) {
                    cout << "Sent " << transferred << " bytes" << endl;
                } else {
                    cerr << "Send failed: " << libusb_error_name(ret) << endl;
                }
                break;
            }

            // 选项6：停止异步读取
            case 6: {
                if (async_transfer) {
                    uint8_t ep = 0x81;
                    cout << "endpoint address (hex, IN endpoint e.g. 0x81): " << hex << static_cast<int>(ep) << dec << endl;
                    //cin >> hex >> ep;
                    //cout << dec;

                    if (async_transfer->stopRead(ep)) {
                        cout << "Async read stopped on endpoint 0x" << hex << static_cast<int>(ep) << dec << endl;
                        // 释放接口
                        device.releaseInterface(0);
                    } else {
                        cerr << "Failed to stop async read on endpoint 0x" << hex << static_cast<int>(ep) << dec << endl;
                    }
                } else {
                    cerr << "No async transfer active" << endl;
                }
                break;
            }

            // 选项0：退出程序
            case 0: {
                g_running = false;
                cout << "Exiting..." << endl;
                break;
            }

            // 默认：无效选项
            default: {
                cout << "Invalid choice" << endl;
                break;
            }
        }
    }

    // 清理资源
    if (async_transfer) {
        async_transfer->stopEventLoop();  // 停止事件循环
    }

    if (device.isOpen()) {
        device.close();  // 关闭设备
    }

    context.exit();  // 退出USB上下文
    cout << "USB Controller exited" << endl;

    return 0;
}
