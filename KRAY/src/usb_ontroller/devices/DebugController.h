#ifndef DEBUGCONTROLLER_H
#define DEBUGCONTROLLER_H

#include "USBHelper.h"
#include "USBController.h"
#include "USBInterface.h"
#include "USBTransferCallback.h"

class DebugController : public USBController {
public:
    DebugController(libusb_device* dev, bool try_detach = true);
    virtual ~DebugController();

    enum interface {
        INTERFACE_KEYBOARD = 0,
        INTERFACE_CUSTOM,
        INTERFACE_MAX,
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
    DebugController(const DebugController&) = delete;  //禁止类的拷贝构造和赋值
    DebugController& operator=(const DebugController&) = delete;
};


#endif // DEBUGCONTROLLER_H
