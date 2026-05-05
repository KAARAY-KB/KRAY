#ifndef USBHOTPLUG_H
#define USBHOTPLUG_H


#include <iostream>
#include <functional>
#include "libusb.h"
#include "USBHelper.h"



#if defined(__linux__)
    #include "USBHotplugLinux.h"
#elif defined(__WIN32__)
    #include "USBHotplugWindows.h"
#elif defined(__APPLE__)
    #include "USBHotplugMac.h"
#endif



class USBHotplug {
public:
    USBHotplug();
    ~USBHotplug();

#if defined(__linux__)
    USBHotplugLinux *handle = nullptr;
#elif defined(__WIN32__)
    USBHotplugWindows *handle = nullptr;
#elif defined(__APPLE__)
    USBHotplugMac *handle = nullptr;
#endif

};


#endif // USBHOTPLUG_H
