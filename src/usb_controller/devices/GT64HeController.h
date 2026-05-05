#ifndef GT64HECONTROLLER_H
#define GT64HECONTROLLER_H

#include "USBHelper.h"
#include "USBController.h"
#include "USBInterface.h"
#include "USBTransferCallback.h"

class GT64HeController : public USBController {
public:
    GT64HeController(libusb_device* dev, bool try_detach = true);
    virtual ~GT64HeController();

    enum interface {
        INTERFACE_KEYBOARD = 0,
        INTERFACE_STANDARD,
        INTERFACE_CUSTOM,
        INTERFACE_LAMP,
        INTERFACE_MAX,
        // INTERFACE_MOUSE,
        // INTERFACE_JOYSTICK,
    };

    enum enpoint {
        EP_CUSTOM_IN   = 0x03 | USBHelper::ENDPOINT_IN,
        EP_CUSTOM_OUT  = 0x03 | USBHelper::ENDPOINT_OUT,
        EP_CUSTOM_SIZE = 0x0100,
        // EP_CUSTOM_RPT_ID = 0x00,
    };
private:
    bool read_cb(uint8_t *buf, uint16_t len, void *user) override;
    bool write_cb(uint8_t *buf, uint16_t len, void *user) override;

private:
    GT64HeController(const GT64HeController&) = delete;  //禁止类的拷贝构造和赋值
    GT64HeController& operator=(const GT64HeController&) = delete;
};


#endif // GT64HECONTROLLER_H