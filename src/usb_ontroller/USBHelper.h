#ifndef USBHELPER_H
#define USBHELPER_H

#include <set>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <functional>
#include "libusb.h"


class USBHelper {
public:
    // typedef enum {
    //     USB_PID_TYPE_KB = 0x00, //Keyboard
    //     USB_PID_TYPE_MS, //Mouse
    //     USB_PID_TYPE_GP, //Gamepad
    //     USB_PID_TYPE_PS, //Portable screen
    //     USB_PID_TYPE_DISK,
    // } PidType;

    typedef enum {
        ENDPOINT_OUT = 0x00, /** Out: host-to-device */
        ENDPOINT_IN = 0x80 /** In: device-to-host */
    } EndpointDir;
    typedef enum {
        DEV_DBG = 0x00,
        DEV_KB_GT64HE,
        DEV_KB_MADE68PRO,
        DEV_KB_WOOTING_60HE,
        DEV_KB_WOOTING_TWOHE,
        DEV_MS_G102,
        DEV_UNKNOWN,
        DEV_MAX = DEV_UNKNOWN,
    } DevType;

    typedef enum {
        FIND_DEV_TYPE_VID = 0x01 << 1,
        FIND_DEV_TYPE_PID = 0x01 << 2,
        FIND_DEV_TYPE_VID_PID = FIND_DEV_TYPE_PID | FIND_DEV_TYPE_VID,

        FIND_DEV_TYPE_MFR = 0x01 << 3,
        FIND_DEV_TYPE_PROD = 0x01 << 4,
        FIND_DEV_TYPE_SN = 0x01 << 5,
        // FIND_DEV_TYPE_BUS = 0x01 << 6,
        // FIND_DEV_TYPE_PORT = 0x01 << 7,
    } DevFindType;

    typedef struct dev_msg_id{
        const char *name;
        DevType ty;
        uint16_t vid;
        uint16_t pid;
        uint16_t bus;
        uint16_t port;
        uint16_t addr;
        bool operator==(const struct dev_msg_id& other) const {
            return  (vid == other.vid) &&
                    (pid == other.pid) &&
                    (bus == other.bus) &&
                    (port == other.port) &&
                    (addr == other.addr);
        }
    } DevMsgId_t;
    typedef struct {
        std::string mfr;
        std::string prod;
        std::string sn;
        std::string path; // bus:port:addr
        std::string user;
    } DevMsgCh_t;
    typedef struct {
        DevMsgId_t id;
        DevMsgCh_t ch;
    } DevMsg_t;

    USBHelper();
    ~USBHelper();

    static std::vector<DevMsg_t> list_device();
    static void list_interface(libusb_device_handle* handle);
    static void list_endpoint(libusb_device_handle* handle, int intf);
    
    static int release_interface(libusb_device_handle* handle, int intf);
    static int claim_interface(libusb_device_handle* handle, int intf, bool try_detach = true);

    static bool dev_id_compare(DevMsg_t dev1, DevMsg_t dev2);

    static libusb_device *find_device(DevMsg_t dev_info);
    static DevMsg_t find_device_support(DevMsg_t dev_info, DevFindType find_type);
//   static void find_controller(libusb_device** dev, DevMsg_t &dev_info, const DevFindType find_type);
//   static bool find_controller_by_path(const std::string& busid, const std::string& devid, libusb_device** xbox_device);
//   static bool find_controller_by_id(int id, int vendor_id, int product_id, libusb_device** xbox_device);
    // static libusb_device* find_device_by_path(uint8_t busnum, uint8_t devnum);


    static uint32_t get_dev_num();
    static DevMsgId_t *get_dev_id(uint16_t idx);
    static void printf_msg(DevMsg_t *const msg, bool show_title);
    static void debug_msg(DevMsg_t &msg);

    // typedef std::function<void (const USBHelper::DevMsg_t)> dev_msg_cb_t;
private:
};

#endif  // USBHELPER_H

