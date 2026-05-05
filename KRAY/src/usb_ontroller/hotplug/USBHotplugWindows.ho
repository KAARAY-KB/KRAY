#ifndef USBHOTPLUGWINDOWS_H
#define USBHOTPLUGWINDOWS_H


#include <windows.h>
#include <dbt.h>
#include <string>
#include <vector>
#include <setupapi.h>
#include <functional>
#include "USBHelper.h"

class USBHotplugWindows
{
private:
    typedef std::function<void (const USBHelper::DevMsg_t)> dev_detector_cb_t;

    typedef enum {
        DEV_CONNECT = 0,
        DEV_DISCONNECT,
    } dev_state_t;

    dev_detector_cb_t m_insert_cb;
    dev_detector_cb_t m_remove_cb;
    std::vector<USBHelper::DevMsg_t> m_devs_info;

    HWND m_hWnd;  // 隐藏窗口句柄
    HDEVNOTIFY m_hDevNotify;  // 设备通知句柄
    std::string m_hiddenWindowClassName;  // 隐藏窗口类名
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam); // 隐藏窗口的窗口过程函数

    // 获取USB设备信息
    std::vector<USBHelper::DevMsg_t> get_existing_decice_info(void);
    // std::vector<USBHelper::DevMsg_t> get_dev_info(const DEV_BROADCAST_DEVICEINTERFACE &dbi);
    std::pair<uint16_t, uint16_t> parse_location_info(const std::string &locationInfo);
    std::tuple<uint16_t, uint16_t, std::string> parse_dev_path(const std::string &devicePath);
    USBHelper::DevMsg_t dev_info_compare(std::vector<USBHelper::DevMsg_t> &devs_info, dev_state_t state);
public:
    USBHotplugWindows();
    ~USBHotplugWindows();

    bool start_detector();
    void stop_detector();
    void refresh_device_evt(void);
    std::vector<USBHelper::DevMsg_t> get_decice_info(void) const { return m_devs_info; };

    void register_cb(dev_detector_cb_t insert_cb, dev_detector_cb_t remove_cb) {
        m_insert_cb = insert_cb;
        m_remove_cb = remove_cb;
    }
    void unregister_cb() {
        m_insert_cb = nullptr;
        m_remove_cb = nullptr;
    }

};

#endif // USBHOTPLUGWINDOWS_H
