#include "USBHotplug.h"


USBHotplug::USBHotplug() 
{
#if defined(__linux__)
    handle = new USBHotplugLinux();
#elif defined(__WIN32__)
    handle = new USBHotplugWindows();
#elif defined(__APPLE__)
    handle = new USBHotplugMac();
#endif

}

USBHotplug::~USBHotplug() 
{
    if (handle) {
        delete handle;
    }
}
