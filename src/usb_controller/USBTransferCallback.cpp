#include "USBTransferCallback.h"

#include <iostream>
#include <assert.h>
#include <memory>


USBTransferCallback::USBTransferCallback(USBInterface *iface, callback_t callback)
    : m_iface(iface)
{
    if (callback == nullptr) {
        m_callback = [](libusb_transfer *transfer) {
            return TRANSFER_ONCE;
        };
    }
    else {
        m_callback = callback;
    }
}
