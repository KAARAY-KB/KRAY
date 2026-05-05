#ifndef USBCONTROLLERFACTORY_H
#define USBCONTROLLERFACTORY_H

#include "USBHelper.h"
#include "USBController.h"


class USBControllerFactory
{
public:
    void create(const USBHelper::DevMsg_t &dev_info, libusb_device* dev/* opt */);
    void create_multiple(const USBHelper::DevMsg_t &dev_info, libusb_device* dev/* opt */);
    


private:
  USBControllerFactory(const USBControllerFactory&);
  USBControllerFactory& operator=(const USBControllerFactory&);
};

#endif // USBCONTROLLERFACTORY_H
