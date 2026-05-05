#ifndef USBCONTROLLER_H
#define USBCONTROLLER_H


#include <set>
#include <map>
#include "USBHelper.h"
#include "USBInterface.h"


class USBController
{

public:
    USBController(libusb_device* dev, std::set<int> _interfaces = std::set<int>());
    virtual ~USBController();

    virtual uint16_t get_vid() const;
    virtual uint16_t get_pid() const;
    virtual std::string get_sn() const;
    virtual std::string get_mfr() const;
    virtual std::string get_prod() const;
    virtual std::string get_path() const;

    virtual bool read_cb(uint8_t *buf, uint16_t len, void *user) = 0;
    virtual bool write_cb(uint8_t *buf, uint16_t len, void *user) = 0;

    void read_async(uint8_t endpoint, int len, uint32_t timeout);
    void write_async(uint8_t endpoint, uint8_t *buf, int len, uint32_t timeout);
    void control_async(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t *wBuf, uint16_t wLength, uint32_t timeout);

    void view_all_endpoint();
    int find_endpoint(int direction, uint8_t if_class, uint8_t if_subclass, uint8_t if_protocol);

    // void add_interface(int interface, bool try_detach);

    libusb_device_handle *m_handle;
    std::map<int, USBInterface *> interfaces;

private:
    void on_read(struct libusb_transfer *xfer);
    void on_write(struct libusb_transfer *xfer);
    void on_control(struct libusb_transfer* xfer);
    static void on_read_wrap(struct libusb_transfer *xfer);
    static void on_write_wrap(struct libusb_transfer *xfer);
    static void on_control_wrap(struct libusb_transfer* xfer);
protected:
    libusb_device *m_dev;
    USBHelper::DevMsg_t m_dev_info;
    std::set<libusb_transfer *> m_transfers;
private:
    USBController(const USBController&) = delete;  //禁止类的拷贝构造和赋值
    USBController& operator=(const USBController&) = delete;
};

#endif // USBCONTROLLER_H
