#include "USBHelper.h"
#include <cassert>
#include <cstdlib>

#include <iostream>
#include <iomanip>
#include <memory>
#include <map>


void handle_error(int r, char const* msg)
{
    std::cerr << msg << " - " << libusb_error_name(r) << "\n";
}
// 产生指定个数的缩进符
std::string _t(int depth)
{
    std::string _tab = "";

    for (int i = 0; i < depth; i++)
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            _tab += " ";
        }
    }
    return _tab;
}


// BCD版本号转字符串
std::string bcd_to_version_string(uint16_t bcd) 
{
    // 0x XXXX
    uint8_t major = (bcd >> 8) & 0xFF; // 主版本号，在第三个字节
    uint8_t minor = (bcd >> 4) & 0x0F; // 次版本号，在第二个字节
    uint8_t patch = bcd & 0x0F; // 补丁号
    
    std::ostringstream oss;
    oss << static_cast<int>(major) << "." 
        << static_cast<int>(minor);
    
    if (patch != 0) 
    {
        oss << "." << static_cast<int>(patch);
    }
    
    return oss.str().c_str();
}

// USB 设备分类的枚举值，转成中文说明
std::string const& class_code_to_string(uint8_t code)
{
    static std::string const unknown_class = "未知类型";
    static std::map<uint8_t, std::string> const m =
    {
        {LIBUSB_CLASS_PER_INTERFACE, "接口层面定义"},
        {LIBUSB_CLASS_AUDIO, "音频设备(扬声器、麦克风、MIDI输入等)"},
        {LIBUSB_CLASS_COMM, "通信设备(调制解调器，网络适配器等)"},
        {LIBUSB_CLASS_HID, "人机接口设备(键盘、鼠标、手柄等)"},
        {LIBUSB_CLASS_PHYSICAL, "物理设备(力反馈、运动传感器等)"},
        {LIBUSB_CLASS_IMAGE, "图像设备(数码相机、扫描仪等)"},
        {LIBUSB_CLASS_PRINTER, "打印机"},
        {LIBUSB_CLASS_MASS_STORAGE, "大容量存储类(U盘、移动硬盘、SD读卡器等)"},
        {LIBUSB_CLASS_HUB, "集线器"},
        {LIBUSB_CLASS_DATA, "数据类(通信设备的配套接口)"},
        {LIBUSB_CLASS_SMART_CARD, "智能卡(安全令牌、读卡器等)"},
        {LIBUSB_CLASS_CONTENT_SECURITY, "数据版权管理设备(内容安全)"},
        {LIBUSB_CLASS_VIDEO, "视频设备(摄像头、视频采集卡等)"},
        {LIBUSB_CLASS_PERSONAL_HEALTHCARE, "个人健康设备(心率监测器、血糖仪等)"},
        {LIBUSB_CLASS_DIAGNOSTIC_DEVICE, "诊断设备(USB调试工具、协议分析仪等)"},
        {LIBUSB_CLASS_WIRELESS, "无线控制器(蓝牙适配器、Wi-Fi网卡等)"},
        {LIBUSB_CLASS_MISCELLANEOUS, "复合功能设备(多功能键盘等)"},
        {LIBUSB_CLASS_APPLICATION, "特定应用设备(固件更新模式、设备测试模式等)"},
        {LIBUSB_CLASS_VENDOR_SPEC, "厂商自定义设备(非标准设备，需专用驱动)"},
    };

    auto it = m.find(code);
    return (it != m.cend() ? it->second : unknown_class);
}

// 传输类型的名称
std::string transfer_type_to_string(uint8_t transfer_type)
{
    switch(transfer_type) 
    {
        case LIBUSB_TRANSFER_TYPE_CONTROL:
            return "控制传输";
        case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
            return "等时传输";
        case LIBUSB_TRANSFER_TYPE_BULK:
            return "批量传输";
        case LIBUSB_TRANSFER_TYPE_INTERRUPT:
            return "中断传输";
        default:
            return "未知传输类型";
    }
}

// 打印端点描述信息
void print_endpoint_descriptor(libusb_endpoint_descriptor const& desc, int depth)
{
    // 传输类型 desc.bmAttributes & 0x03 (LIBUSB_TRANSFER_TYPE_MASK)
    uint8_t transfer_type = desc.bmAttributes 
		& LIBUSB_TRANSFER_TYPE_MASK; 

    std::cout << _t(depth) << "端点地址: 0x" << std::hex << std::setw(2) 
	<< static_cast<int>(desc.bEndpointAddress)
        << " - " << transfer_type_to_string(transfer_type) 
        << "(" << ((desc.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)? "IN" : "OUT") 
	<< ")\n";
}

// 打印接口信息
void print_interface_descriptor(libusb_interface_descriptor const& desc, int depth)
{
    // 每个接口有自己的设备类型:
    std::cout << _t(depth) << "设备类型: " 
	<< class_code_to_string(desc.bInterfaceClass) << "\n";

    std::cout << std::hex;
    std::cout << _t(depth) << "设备子类型: 0x" << std::setw(2) 
	<< static_cast<int>(desc.bInterfaceSubClass) << "\n";
    std::cout << _t(depth) << "接口协议: 0x" << std::setw(2) 
	<< static_cast<int>(desc.bInterfaceProtocol) << "\n";
    std::cout << _t(depth) << "包含端点个数: " << std::setw(4) << std::dec 
	<< static_cast<int>(desc.bNumEndpoints) << "\n";

    for (int i=0; i<desc.bNumEndpoints; ++i)
    {
        std::cout << _t(depth) << "#" << std::dec << i << ":\n";
        print_endpoint_descriptor(desc.endpoint[i], depth+1);
    }
}

// 打印设备配置信息
void print_config_descriptor(libusb_config_descriptor const& desc, int depth)
{
    // labmda : 设备功耗  =  电流 * 电压 (固定 5 伏)
    auto toV = [](unsigned int V) -> double { return 5.0; };
    auto toA = [](unsigned int mA) -> double { return mA / 1000.0; };
    auto toW = [](unsigned int V, unsigned int A) -> double { return V * A; };

    // 最大功耗: xV/xA 0.5 瓦(high-speed-mode),2瓦(super-speed-mode)
    std::cout << _t(depth) << "最大功耗:\n" 
            << _t(depth+1) << "high-speed-mode:  "
            << std::dec << toV(5) << "V" << "/" << toA(desc.MaxPower * 2) << "A "
            << std::setprecision(2) << toW(toV(5), toA(desc.MaxPower * 2)) << "W\n" 
            << _t(depth+1) << "super-speed-mode: "
            << std::dec << toV(5) << "V" << "/" << toA(desc.MaxPower * 8) << "A "
            << std::setprecision(2)<< toW(toV(5), toA(desc.MaxPower * 8)) << "W\n";   

    std::cout << _t(depth) << "包含接口个数: " << std::setw(4) << std::dec 
	<< static_cast<int>(desc.bNumInterfaces) << "\n";

    for (int i=0; i<desc.bNumInterfaces; ++i)
    {
        std::cout << std::dec << _t(depth) << "接口 #" << i; // 接口从 0 起

        // 取接口:
        libusb_interface const* interface = &(desc.interface[i]);
        
        if (interface->num_altsetting <= 0)
        {
            std::cout << "(无备用配置)\n";
            continue;
        }

        if (interface->num_altsetting > 1)
        {
            std::cout << "(有 " << std::dec << interface->num_altsetting << " 个备用配置)\n";
        }
        else
        {
            std::cout << "\n";
        }

        // 取首选配置:
        libusb_interface_descriptor const* interface_desc 
		= &(interface->altsetting[0]); // 第0个->首选！

        // 打印接口描述(设置)信息
        print_interface_descriptor(*interface_desc, depth + 1);
    }
}

void print_device_descriptor(libusb_device* dev, libusb_device_descriptor const& desc, int depth = 1)
{
    std::cout << _t(depth) << "USB 版本: " << bcd_to_version_string(desc.bcdUSB) << "\n";
    std::cout << _t(depth) << "设备类型: " << class_code_to_string(desc.bDeviceClass) << "\n";
    std::cout << _t(depth) << "设备版本: " << bcd_to_version_string(desc.bcdDevice) << "\n";

    std::cout << std::hex;
    std::cout << _t(depth) << "厂商 ID: 0x" << std::setw(4) << desc.idVendor << "\n";
    std::cout << _t(depth) << "产品 ID: 0x" << std::setw(4) << desc.idProduct << "\n";

    std::cout << _t(depth) << "包含配置数: " << std::dec << std::setw(2) 
	<< static_cast<int>(desc.bNumConfigurations) << "\n";

    libusb_config_descriptor *config_desc = nullptr; // 配置描述

    // 只取活动的配置
    if (int r = libusb_get_active_config_descriptor(dev, &config_desc); r != LIBUSB_SUCCESS)
    {
        handle_error(r, "获取设备的活动配置失败！");
        return;
    }

    assert(config_desc != nullptr);

    // 安排哨兵
    std::unique_ptr<libusb_config_descriptor, void(*)(libusb_config_descriptor *)> 	
	guard(config_desc, libusb_free_config_descriptor);

    // 打印配置描述
    print_config_descriptor(*config_desc, depth+1);
}

int ex_main()
{
    std::system("chcp 65001 >nul"); // 设置程序运行的控制台编码为 UTF-8

    // RAII的一种使用(自定义类型，非智能指针) 
    struct AutoPauser  { ~AutoPauser() { std::system("pause"); }} auto_pauser;

    // 初始化 libusb 
    libusb_context *ctx = nullptr;
    if (int r = libusb_init(&ctx); r != LIBUSB_SUCCESS)
    {
        handle_error(r, "初始化失败！");
        return -1;
    }

    // 定义哨兵-1
    std::unique_ptr<libusb_context, void(*) (libusb_context*)> guard_1 (ctx, libusb_exit);

    // 获取设备列表
    libusb_device** devices; // 设备列表
    ssize_t count = libusb_get_device_list(ctx, &devices);

    if (count < 0)
    {
        handle_error(count, "获取设备列表失败！");
        return -1;
    }

    std::cout << "发现 " << count << " 个 USB 设备。\n";

    // 定义哨兵-2
    std::unique_ptr<libusb_device*, void(*) (libusb_device**)> guard_2(devices
            , [](libusb_device** p) { libusb_free_device_list(p, 1); });

    std::cout << std::setfill('0');

    for (int i=0; i<count; ++i)
    {
        std::cout << "设备 " << std::dec << std::setw(2) << i + 1 << ":\n";

        libusb_device_descriptor desc; // 设备描述数据
        if (int r = libusb_get_device_descriptor(devices[i], &desc); r != LIBUSB_SUCCESS)
        {
            handle_error(r, "获取设备描述失败！");
            continue;
        }

        print_device_descriptor(devices[i], desc);
        std::cout << "-----------------------------------------------------------------\n";
    }
}
