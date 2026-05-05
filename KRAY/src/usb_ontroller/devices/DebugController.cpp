#include "DebugController.h"


DebugController::DebugController(libusb_device* dev, bool try_detach)
    : USBController(dev)
{
}

DebugController::~DebugController()
{
}

bool DebugController::read_cb(uint8_t *buf, uint16_t len, void *user) { 
    (void)user;
    char buff[64];
    // sprntf(buff, 64, "usb rx[%d]%d,%d,%d\n", len, buf[0], buf[1], buf[len-1]);
    std::snprintf(buff, 64, "usb rx[%d]%d,%d,%d\n", len, buf[0], buf[1], buf[len-1]);
    std::cout << buff << std::endl;
    return false;
}
bool DebugController::write_cb(uint8_t *buf, uint16_t len, void *user) {
    return false;
}
