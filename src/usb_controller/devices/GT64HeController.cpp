#include "GT64HeController.h"
#include "console.h"

GT64HeController::GT64HeController(libusb_device* dev, bool try_detach)
    : USBController(dev, std::set<int>{INTERFACE_CUSTOM})
{
}

GT64HeController::~GT64HeController()
{
}

bool GT64HeController::read_cb(uint8_t *buf, uint16_t len, void *user) { 
    (void)user;
    char buff[64];
    // sprntf(buff, 64, "usb rx[%d]%d,%d,%d\n", len, buf[0], buf[1], buf[len-1]);
    std::snprintf(buff, 64, "usb rx[%d]%d,%d,%d\n", len, buf[0], buf[1], buf[len-1]);
    Console::out() << buff << std::endl;
    return false;
}
bool GT64HeController::write_cb(uint8_t *buf, uint16_t len, void *user) {
    return false;
}
