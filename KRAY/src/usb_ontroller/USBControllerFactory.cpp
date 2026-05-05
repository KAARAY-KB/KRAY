#include "USBControllerFactory.h"


void USBControllerFactory::create(const USBHelper::DevMsg_t &dev_info, libusb_device* dev)
{
    switch (dev_info.id.ty)
    {
        case USBHelper::DevType::DEV_KB_GT64HE:
            break;
        default:
            break;
    }
    
}
