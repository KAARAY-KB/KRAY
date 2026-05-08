#include "USBHotplugLinux.h"
#include "console.h"


USBHotplugLinux::USBHotplugLinux() 
    : m_hp_handle(0)
    , m_insert_cb(nullptr)
    , m_remove_cb(nullptr)
{
    //m_devs_info = get_existing_decice_info(); //update device info
    m_devs_info = USBHelper::list_device();
}

USBHotplugLinux::~USBHotplugLinux() 
{
    stop_detector();
    unregister_cb();
}

bool USBHotplugLinux::start_detector()
{
    if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        Console::out() << "Hotplug capabilities are not supported on this platform." << std::endl;
        return false;
    }

    int ret = libusb_hotplug_register_callback(nullptr, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                                                LIBUSB_HOTPLUG_NO_FLAGS, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                                LIBUSB_HOTPLUG_MATCH_ANY, hotplug_cb, this, &m_hp_handle);
    if (LIBUSB_SUCCESS != ret) {
        Console::out() << "Failed to register hotplug callback." << libusb_strerror(ret) << std::endl;
        return false;
    }
    return true;
}
void USBHotplugLinux::stop_detector()
{
    libusb_hotplug_deregister_callback(nullptr, m_hp_handle);
    m_hp_handle = 0;
    unregister_cb();
}

void USBHotplugLinux::get_existing_decice_info(libusb_device *dev, USBHelper::DevMsg_t &info) {
    char ch[1024];
    int len, ret;
    struct libusb_device_handle *handle;
    struct libusb_device_descriptor desc;
    
    // "%02d:%02d:%02d"
    info.id.bus  = libusb_get_bus_number(dev); //Hub
    info.id.port = libusb_get_port_number(dev); //Port
    info.id.addr = libusb_get_device_address(dev); //Addr

    ret  = libusb_get_device_descriptor(dev, &desc);
    if (ret != LIBUSB_SUCCESS) {
        Console::out() << "libusb_get_device_descriptor() failed: " << libusb_strerror(ret) << std::endl;
        return;
    }
    // "%04x:%04x"
    info.id.vid = desc.idVendor;
    info.id.pid = desc.idProduct;

    ret = libusb_open(dev, &handle);
    if (ret != LIBUSB_SUCCESS) {
        Console::out() << "libusb_open() failed: " << libusb_strerror(ret) << std::endl;
        return;
    }
    len = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, reinterpret_cast<unsigned char *>(ch), sizeof(ch));
    if (len > 0) {
        info.ch.mfr.append(ch, len);
    }

    len = libusb_get_string_descriptor_ascii(handle, desc.iProduct, reinterpret_cast<unsigned char *>(ch), sizeof(ch));
    if (len > 0) {
        info.ch.prod.append(ch, len);
    }

    len = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, reinterpret_cast<unsigned char *>(ch), sizeof(ch));
    if (len > 0) {
        info.ch.sn.append(ch, len);
    }
    
    libusb_close(handle);
}

int LIBUSB_CALL USBHotplugLinux::hotplug_cb(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
    USBHelper::DevMsg_t dev_info;
    USBHotplugLinux *self = static_cast<USBHotplugLinux*>(user_data);
    self->get_existing_decice_info(dev, dev_info);
    // 移除
    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
        Console::out() << "device remove: " << std::hex 
                    << "vid: "  << dev_info.id.vid 
                    << "pid: "  << dev_info.id.pid 
                    << "bus: "  << dev_info.id.bus
                    << "port: " << dev_info.id.port
                    << std::endl;

        // 从当前设备列表中移除该唯一标识设备
        auto it = std::find_if(self->m_devs_info.begin(), self->m_devs_info.end(),
            [&dev_info](const USBHelper::DevMsg_t& d) {
                return USBHelper::dev_id_compare(d, dev_info);
            });
        if (it != self->m_devs_info.end()) {
            self->m_devs_info.erase(it);
        }

        if (self->m_remove_cb) {
            self->m_remove_cb(dev_info);
        }
    }
    // 插入
    else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        Console::out() << "device insert: " << std::hex 
                    << "vid: "  << dev_info.id.vid 
                    << "pid: "  << dev_info.id.pid 
                    << "bus: "  << dev_info.id.bus
                    << "port: " << dev_info.id.port
                    << std::endl;

        // 避免重复插入：检查是否已在列表中
        auto it = std::find_if(self->m_devs_info.begin(), self->m_devs_info.end(),
            [&dev_info](const USBHelper::DevMsg_t& d) {
                return USBHelper::dev_id_compare(d, dev_info);
            });
        if (it == self->m_devs_info.end()) {
            self->m_devs_info.push_back(dev_info);
        }

        if (self->m_insert_cb) {
            self->m_insert_cb(dev_info);
        }
    }
    return 0;
}
void USBHotplugLinux::refresh_device_evt(void)
{
    for (auto &dev : m_devs_info) {
        if (m_insert_cb) {
            m_insert_cb(dev);
        }
    }
}

