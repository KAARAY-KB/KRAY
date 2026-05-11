#include "USBHelper.h"
#include "console.h"
#include <iomanip>
#include <sstream>
#include <iostream>

static USBHelper::DevMsgId_t g_devs_info[] = {
    { "DEBUG",              USBHelper::DEV_DBG,                 0x5741, 0x4E47, 0x00, 0x00, 0x00 },
    { "GT-64HE",            USBHelper::DEV_KB_GT64HE,           0x9013, 0x2601, 0x00, 0x00, 0x00 },
    { "G102",               USBHelper::DEV_MS_G102,             0x046d, 0xc092, 0x00, 0x00, 0x00 },
    { "MADE68PRO",          USBHelper::DEV_KB_MADE68PRO,        0x0416, 0x0110, 0x00, 0x00, 0x00 },
    { "Wooting-TwoHe",      USBHelper::DEV_KB_WOOTING_TWOHE,    0x31E3, 0x1232, 0x00, 0x00, 0x00 },
};
static uint16_t g_dev_num = sizeof(g_devs_info)/sizeof(USBHelper::DevMsgId_t);

USBHelper::USBHelper()
{
}

USBHelper::~USBHelper()
{
}

uint32_t USBHelper::get_dev_num() {
    return g_dev_num;
}
USBHelper::DevMsgId_t *USBHelper::get_dev_id(uint16_t idx) {
    return g_devs_info + idx;
}
USBHelper::DevMsg_t USBHelper::find_device_support(DevMsg_t dev_info, DevFindType find_type) {
    dev_info.id.ty = DEV_UNKNOWN;
    for (uint32_t i = 0; i < get_dev_num(); ++i) {
        if (find_type == FIND_DEV_TYPE_NONE) {
            return dev_info;
        }
        if ((find_type & FIND_DEV_TYPE_VID) && (get_dev_id(i)->vid != dev_info.id.vid))          continue;
        else if ((find_type & FIND_DEV_TYPE_PID) && (get_dev_id(i)->pid != dev_info.id.pid))     continue;
        
        dev_info.id.ty = get_dev_id(i)->ty;
        dev_info.id.name = get_dev_id(i)->name;
        break;
    }
    return dev_info;
}

std::vector<USBHelper::DevMsg_t> USBHelper::list_device() {
    libusb_device** dev_list;
    ssize_t num_devices = libusb_get_device_list(NULL, &dev_list);
    std::vector<USBHelper::DevMsg_t> devs_info;
    USBHelper::DevMsg_t info = {0};

    int id = 0;
    Console::out() << USBHelper::msg(NULL, true);

    for(ssize_t dev_it = 0; dev_it < num_devices; ++dev_it) {
        uint32_t len = 0;
        char ch[512] = "N/A";
        libusb_device_descriptor desc;

        if (libusb_get_device_descriptor(dev_list[dev_it], &desc) == LIBUSB_SUCCESS) {
            libusb_device_handle *handle = NULL;
            memset(&info, 0, sizeof(info));
            uint8_t mfr[128]  = "N/A";
            uint8_t prod[128] = "N/A";
            uint8_t sn[128]   = "N/A";
            uint16_t size     = 0;

            // vid:pid
            info.id.vid  = desc.idVendor; //vid
            info.id.pid  = desc.idProduct; //pid
            
            // path
            info.id.bus  = libusb_get_bus_number(dev_list[dev_it]); //Hub
            info.id.port = libusb_get_port_number(dev_list[dev_it]); //Port
            info.id.addr = libusb_get_device_address(dev_list[dev_it]); //Addr
            
            // string
            int rc = libusb_open(dev_list[dev_it], &handle);
            if ((rc == 0) && handle) {
                if (desc.iManufacturer) {
                    size = sizeof(mfr);
                    int len = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, mfr, size);
                    if (len > 0) {
                        info.ch.mfr.append((char *)mfr, len);
                    }
                }
                if (desc.iProduct) {
                    size = sizeof(prod);
                    int len = libusb_get_string_descriptor_ascii(handle, desc.iProduct, prod, size);
                    if (len > 0) {
                        info.ch.prod.append((char *)prod, len);
                    }
                }
                if (desc.iSerialNumber) {
                    size = sizeof(sn);
                    int len = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, sn, size);
                    if (len > 0) {
                        info.ch.sn.append((char *)sn, len);
                    }
                }
                libusb_close(handle);
            } 
            else {
                Console::out() << "vid:" << info.id.vid  << "pid:" << info.id.pid << " - [Permission denied or device busy]" << std::endl;
            }

            Console::out() << USBHelper::msg(&info, false);
            devs_info.push_back(info);
        }
    }

    libusb_free_device_list(dev_list, 1 /* unref_devices */);
    return devs_info;
}

void USBHelper::debug_msg(DevMsg_t &msg) {
    msg.id.ty = DEV_DBG;
    msg.id.name = "DEBUG";
    msg.id.vid = 0xDEDE;
    msg.id.pid = 0xDEDE;
    msg.id.bus = 0xDEDE;
    msg.id.port = 0xDEDE;
    msg.id.addr = 0xDEDE;
    msg.ch.mfr = "debug";
    msg.ch.prod = "debug";
    msg.ch.sn = "debug";
}
std::string USBHelper::msg(DevMsg_t *const msg, bool show_title) {
    std::string str = "";
    std::string end = "\r\n";

    if (show_title) {
        str += "---------------------------------------------------------------------------------------------------" + end;
        str += "|   vid    |   pid    |   bus    |   port   |   addr   |  manufacturer, product name, serial number" + end;
        str += "|----------|----------|----------|----------|----------|-------------------------------------------" + end;
    }

    if (msg != NULL) {
        std::string mfr  = msg->ch.mfr.empty()  ? "NULL" : msg->ch.mfr;
        std::string prod = msg->ch.prod.empty() ? "NULL" : msg->ch.prod;
        std::string sn   = msg->ch.sn.empty()   ? "NULL" : msg->ch.sn;
        
        auto hex4 = [](uint16_t val) -> std::string {
            char buf[8];
            snprintf(buf, sizeof(buf), "0x%04x", val);
            return std::string(buf);
        };
        
        auto pad_right = [](const std::string& s, size_t width, const char fill_char = ' ') -> std::string {
            return s.length() >= width ? s : s + std::string(width - s.length(), fill_char);
        };
        
        str += ("|  " + hex4(msg->id.vid) 
            + "  |  " + hex4(msg->id.pid) 
            + "  |  " + hex4(msg->id.bus) 
            + "  |  " + hex4(msg->id.port) 
            + "  |  " + hex4(msg->id.addr) 
            + "  |  " 
            + pad_right(mfr,  12, ' ') + ", " 
            + pad_right(prod, 12, ' ') + ", " 
            + pad_right(sn,   13, ' ') 
            + end);
    }

    return str;
}

libusb_device * USBHelper::find_device(DevMsg_t dev_info) {
    libusb_device *m_dev = nullptr;
    libusb_device **dev_list;
    ssize_t num_devices = libusb_get_device_list(nullptr, &dev_list);

    for(ssize_t dev_it = 0; dev_it < num_devices; ++dev_it) {
        libusb_device *dev = dev_list[dev_it];
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) == LIBUSB_SUCCESS) {
            uint8_t bus  = libusb_get_bus_number(dev);
            uint8_t port = libusb_get_port_number(dev);
            // uint8_t addr = libusb_get_device_address(dev);

            if ((dev_info.id.vid == desc.idVendor) && 
                (dev_info.id.pid == desc.idProduct) && 
                (dev_info.id.bus == bus) && 
                (dev_info.id.port == port)) {
                m_dev = dev;
                libusb_ref_device(m_dev);
                break;
            }
        }
    }

    libusb_free_device_list(dev_list, 1 /* unref_devices */);
    return m_dev;
}

bool USBHelper::dev_id_compare(DevMsg_t dev1, DevMsg_t dev2) {
    return dev1.id == dev2.id;
}

int USBHelper::release_interface(libusb_device_handle* handle, int intf) {
    int ret = libusb_release_interface(handle, intf); //释放接口
    if (ret != LIBUSB_SUCCESS) {
        Console::out() << "error releasing interface: " << intf << ": " << libusb_error_name(ret) << std::endl;
    }
    return ret;
}

int USBHelper::claim_interface(libusb_device_handle* handle, int intf, bool try_detach) {
    int ret = libusb_claim_interface(handle, intf); //申请接口
    if (ret == LIBUSB_ERROR_BUSY && try_detach) {
        ret = libusb_detach_kernel_driver(handle, intf); //卸载接口的内核驱动程序
        if (ret == LIBUSB_SUCCESS) {
            ret = libusb_claim_interface(handle, intf);
            if (ret != LIBUSB_SUCCESS) {
                Console::out() << "error claiming interface: " << intf << ": " << libusb_error_name(ret) << std::endl;
            }
        }
        else {
            Console::out() <<  "error detaching kernel driver: "<< intf << ": " << libusb_error_name(ret) << std::endl; 
        }
    }
    else if (ret != LIBUSB_SUCCESS) {
        Console::out() << "error claiming interface: " << intf << ": " << libusb_error_name(ret) << std::endl;
    }
    return ret;
}

