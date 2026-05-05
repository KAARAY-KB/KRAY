#ifndef USBHOTPLUGLINUX_H
#define USBHOTPLUGLINUX_H

#include "USBHelper.h"


class USBHotplugLinux {

private:
    typedef std::function<void (const USBHelper::DevMsg_t)> dev_hotplug_cb_t;

    libusb_hotplug_callback_handle m_hp_handle;
    dev_hotplug_cb_t m_insert_cb;
    dev_hotplug_cb_t m_remove_cb;
    std::vector<USBHelper::DevMsg_t> m_devs_info;

    void get_existing_decice_info(libusb_device *dev, USBHelper::DevMsg_t &info);
  //  std::vector<USBHelper::DevMsg_t> get_existing_decice_info(void);

    static int LIBUSB_CALL hotplug_cb(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data);

public:
    USBHotplugLinux();
    ~USBHotplugLinux();

    bool start_detector();
    void stop_detector();
    void refresh_device_evt();
    std::vector<USBHelper::DevMsg_t> get_decice_info(void) const { return m_devs_info; };

    void register_cb(dev_hotplug_cb_t insert_cb, dev_hotplug_cb_t remove_cb) {
        m_insert_cb = insert_cb;
        m_remove_cb = remove_cb;
    }
    void unregister_cb() {
        m_insert_cb = nullptr;
        m_remove_cb = nullptr;
    }

};


#endif // USBHOTPLUGLINUX_H
