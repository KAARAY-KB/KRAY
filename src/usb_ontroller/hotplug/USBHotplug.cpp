#include "USBHotplug.h"


USBHotplug::USBHotplug() 
{
    handle = new USBHotplugLinux();
}

USBHotplug::~USBHotplug() 
{
    if (handle) {
        delete handle;
    }
}
