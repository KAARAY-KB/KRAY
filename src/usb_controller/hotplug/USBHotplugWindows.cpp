#include "USBHotplugWindows.h"


#include <iostream>
// #include <sstream>
// #include <devguid.h>
// #include <regstr.h>
// #include <regex>
#include <usbioctl.h>
#include <usbscan.h>


USBHotplugWindows::USBHotplugWindows() 
    : m_hWnd(nullptr)
    , m_hDevNotify(nullptr)
    , m_insert_cb(nullptr)
    , m_remove_cb(nullptr)
{
    m_hiddenWindowClassName = "usb detector";
    m_devs_info = get_existing_decice_info(); //update device info
}

USBHotplugWindows::~USBHotplugWindows()
{
    m_devs_info.clear();
    stop_detector();
    unregister_cb();
}


void USBHotplugWindows::refresh_device_evt(void)
{
    for (auto &dev : m_devs_info) {
        if (m_insert_cb) {
            m_insert_cb(dev);
        }
    }
}

bool USBHotplugWindows::start_detector()
{
    // 注册隐藏窗口类
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = m_hiddenWindowClassName.c_str();
    if (!RegisterClassA(&wc)) {
        std::cerr << "RegisterClassA failed" << std::endl;
        return false;
    }

    // 创建隐藏窗口
    m_hWnd = CreateWindowA(m_hiddenWindowClassName.c_str(), "", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, this);
    if (!m_hWnd) {
        std::cerr << "CreateWindowA failed" << std::endl;
        return false;
    }

    // 设置窗口用户数据为当前实例指针
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // 注册设备通知
    DEV_BROADCAST_DEVICEINTERFACE dbi = {};
    dbi.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    dbi.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;  // USB设备接口类GUID

    m_hDevNotify = RegisterDeviceNotification(m_hWnd, &dbi, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (!m_hDevNotify) {
        std::cerr << "RegisterDeviceNotification failed" << std::endl;
        return false;
    }

    return true;
}

void USBHotplugWindows::stop_detector()
{
    if (m_hDevNotify) {
        UnregisterDeviceNotification(m_hDevNotify);
        m_hDevNotify = nullptr;
    }

    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }

    UnregisterClassA(m_hiddenWindowClassName.c_str(), GetModuleHandleA(nullptr));
}

LRESULT CALLBACK USBHotplugWindows::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    USBHotplugWindows* pDetector = reinterpret_cast<USBHotplugWindows*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if ((uMsg == WM_DEVICECHANGE) && (pDetector) && (lParam)) {
        DEV_BROADCAST_DEVICEINTERFACE *dbi = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE *>(lParam);
        if ((dbi) && (dbi->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE)) {
            // dbi->dbcc_name; // current device path
            std::vector<USBHelper::DevMsg_t> devs_info = pDetector->get_existing_decice_info();
            if ((wParam == DBT_DEVICEARRIVAL) && (pDetector->m_insert_cb)) {
                USBHelper::DevMsg_t action_dev_info = USBHelper::find_device_support(pDetector->dev_info_compare(devs_info, DEV_CONNECT), 
                                                                                     USBHelper::DevFindType::FIND_DEV_TYPE_VID_PID);
                if (action_dev_info.id.ty != USBHelper::DevType::DEV_UNKNOWN) {
                    pDetector->m_insert_cb(action_dev_info);
                }
                else {
                    pDetector->m_insert_cb(action_dev_info);
                }
            }
            else if ((wParam == DBT_DEVICEREMOVECOMPLETE) && (pDetector->m_remove_cb)) {
                USBHelper::DevMsg_t action_dev_info = USBHelper::find_device_support(pDetector->dev_info_compare(devs_info, DEV_DISCONNECT), 
                                                                                     USBHelper::DevFindType::FIND_DEV_TYPE_VID_PID);
                if (action_dev_info.id.ty != USBHelper::DevType::DEV_UNKNOWN) {
                    pDetector->m_remove_cb(action_dev_info);
                }
                else {
                    pDetector->m_remove_cb(action_dev_info);
                }
            }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

std::vector<USBHelper::DevMsg_t> USBHotplugWindows::get_existing_decice_info(void)
{
    // parse_dev_path(dbi.dbcc_name);
    std::vector<USBHelper::DevMsg_t> devs_info;
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE); //存在的设备
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        std::cerr << "SetupDiGetClassDevs failed" << std::endl;
        return devs_info;
    }
    
    SP_DEVICE_INTERFACE_DATA devInterfaceData = {};
    devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    // devInterfaceData = *reinterpret_cast<const SP_DEVICE_INTERFACE_DATA *>(&dbi);
    for (DWORD idx = 0; ; ++idx) {
        // 获取设备接口信息
        if (!SetupDiEnumDeviceInterfaces(hDevInfo, nullptr, &GUID_DEVINTERFACE_USB_DEVICE, idx, &devInterfaceData)) {
            if (GetLastError() != ERROR_NO_MORE_ITEMS) {
                std::cerr << "SetupDiEnumDeviceInterfaces failed: " << idx << "-" << GetLastError() << std::endl;
            }
            break;
        }

        //获取设备接口的详细信息
        DWORD requiredSize = 0;
        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, nullptr, 0, &requiredSize, nullptr)) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                std::cerr << "SetupDiGetDeviceInterfaceDetail failed: " << idx << "-" << GetLastError() << std::endl;
                break;
            }
        }
        PSP_DEVICE_INTERFACE_DETAIL_DATA pDevInterfaceDetailData = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(requiredSize));
        pDevInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, pDevInterfaceDetailData, requiredSize, &requiredSize, nullptr)) {
            std::cerr << "SetupDiGetDeviceInterfaceDetail failed: " << idx << "-" << GetLastError() << std::endl;
            free(pDevInterfaceDetailData);
            break;
        }

        USBHelper::DevMsg_t info = {0};
        
        // Qt6 默认定义了 UNICODE 宏，导致 Windows API 使用宽字符版本
#ifdef UNICODE
        int len = WideCharToMultiByte(CP_ACP, 0, pDevInterfaceDetailData->DevicePath, -1, nullptr, 0, nullptr, nullptr);
        std::string devPathA(len, 0);
        WideCharToMultiByte(CP_ACP, 0, pDevInterfaceDetailData->DevicePath, -1, &devPathA[0], len, nullptr, nullptr);
        devPathA.pop_back();
#else
        std::string devPathA(pDevInterfaceDetailData->DevicePath);
#endif
        std::tie(info.id.vid, info.id.pid, info.ch.sn) = parse_dev_path(devPathA);

        // 获取设备信息
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiEnumDeviceInfo(hDevInfo, idx, &devInfoData)) {
            std::cerr << "SetupDiEnumDeviceInfo failed: " << idx << "-" << GetLastError() << std::endl;
        }
        else {
            DWORD size = 0;
            SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_LOCATION_INFORMATION, nullptr, nullptr, 0, &size);
            if (size > 0) {
                BYTE* buffer = new BYTE[size];
                SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_LOCATION_INFORMATION, nullptr, buffer, size, nullptr);
                
                std::tie(info.id.bus, info.id.port) = parse_location_info(reinterpret_cast<const char*>(buffer));
                // info.ch.user.append(std::to_string(idx)+": ");
                info.ch.user.append(devPathA);
                info.ch.user.append("\n");
                info.ch.user.append(reinterpret_cast<const char*>(buffer), size);

                delete[] buffer;
            }
        }
        devs_info.push_back(info);

        free(pDevInterfaceDetailData);
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return devs_info;
}

std::tuple<uint16_t, uint16_t, std::string> USBHotplugWindows::parse_dev_path(const std::string &devicePath) {
    // "\\?\USB#VID_0001&PID_0002#abcdefg#{a5dcbf10-6530-11d2-901f-00c04fb951ed}"
    uint16_t vid, pid;
    size_t start_pos, end_pos;
    std::string vid_str, pid_str, sn_str;

    start_pos = devicePath.find("usb#") + 4;
    end_pos = devicePath.find("{", start_pos) + 1;
    std::string input = devicePath.substr(start_pos, end_pos - start_pos); // VID_0001&PID_0002#abcdefg#{

    start_pos = input.find("vid_") + 4; //vid
    end_pos = input.find("&pid_", start_pos);
    vid_str = input.substr(start_pos, end_pos - start_pos);
    vid = std::stoi(vid_str, nullptr, 16);

    start_pos = input.find("pid_") + 4; //pid
    end_pos = input.find("#", start_pos);
    pid_str = input.substr(start_pos, end_pos - start_pos);
    pid = std::stoi(pid_str, nullptr, 16);

    start_pos = input.find("#") + 1; //sn
    end_pos = input.find("#{", start_pos);
    sn_str = input.substr(start_pos, end_pos - start_pos);

    // char ch[128];
    // sprintf(ch, "vid:0x%04x, pid:0x%04x, sn:%s\n", vid, pid, sn_str.c_str());
    // std::cout << ch;

    return {vid, pid, sn_str};
}

std::pair<uint16_t, uint16_t> USBHotplugWindows::parse_location_info(const std::string &locationInfo) {
    // "Port_#0014.Hub_#0001"
    size_t start_pos, end_pos;

    //port
    start_pos = locationInfo.find("Port_#") + 6;
    end_pos = locationInfo.find(".", start_pos);
    std::string port_srt = locationInfo.substr(start_pos, end_pos - start_pos);
    uint16_t port = std::stoi(port_srt);

    //hub
    start_pos = locationInfo.find("Hub_#") + 5;
    end_pos = locationInfo.size();
    std::string hub_srt = locationInfo.substr(start_pos, end_pos - start_pos);
    uint16_t hub = std::stoi(hub_srt);

    // char ch[128];
    // sprintf(ch, "hub:%d, port:%d\n", hub, port);
    // std::cout << ch;

    return {hub, port};
}

USBHelper::DevMsg_t USBHotplugWindows::dev_info_compare(std::vector<USBHelper::DevMsg_t> &devs_info, dev_state_t state)
{
    USBHelper::DevMsg_t a_dev_info;
    size_t t_size = devs_info.size();
    size_t m_size = m_devs_info.size();
    
    if (state == DEV_CONNECT) {
        for (size_t i = 0; i < t_size; i++) {
            bool is_find = false;
            for (size_t j = 0; j < m_size; j++) {
                if ((devs_info[i].id.vid  == m_devs_info[j].id.vid) && 
                    (devs_info[i].id.pid  == m_devs_info[j].id.pid) && 
                    (devs_info[i].id.bus  == m_devs_info[j].id.bus) && 
                    (devs_info[i].id.port == m_devs_info[j].id.port)) {
                    is_find = true;
                    continue;
                }
            }
            if (!is_find) {
                a_dev_info = devs_info[i]; //action
                break;
            }
        }
    }
    else {
        for (size_t i = 0; i < m_size; i++) {
            bool is_find = false;
            for (size_t j = 0; j < t_size; j++) {
                if ((m_devs_info[i].id.vid  == devs_info[j].id.vid) && 
                    (m_devs_info[i].id.pid  == devs_info[j].id.pid) && 
                    (m_devs_info[i].id.bus  == devs_info[j].id.bus) && 
                    (m_devs_info[i].id.port == devs_info[j].id.port)) {
                    is_find = true;
                    continue;
                }
            }
            if (!is_find) {
                a_dev_info = m_devs_info[i]; //action
                break;
            }
        }
    }
    m_devs_info.clear();
    m_devs_info = devs_info; // //update device info
    return a_dev_info;
}
