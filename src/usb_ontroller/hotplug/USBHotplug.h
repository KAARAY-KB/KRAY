#ifndef USBHOTPLUG_H
#define USBHOTPLUG_H


#include <iostream>
#include <functional>
#include "libusb.h"
#include "USBHelper.h"

#if OS_LINUX
#include "USBHotplugLinux.h"
#define USB_HOTPLUG_OS(x)     x##Linux
#elif OS_WINDOWS
//#include "USBHotplugWindows.h"
#define USB_HOTPLUG_OS(x)     x##Windows
#elif OS_MAC
#include "USBHotplugMac.h"
#define USB_HOTPLUG_OS(x)     x##Mac
#endif

#include "USBHotplugLinux.h"
#define USB_HOTPLUG_OS(x)     x##Linux


class USBHotplug {
public:
    USBHotplug();
    ~USBHotplug();
    USBHotplugLinux *handle = nullptr;
};


#endif // USBHOTPLUG_H
