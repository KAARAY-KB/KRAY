#ifndef USBTRANSFERCALLBACK_H
#define USBTRANSFERCALLBACK_H

#include <functional>
#include "libusb.h"


class USBInterface;

class USBTransferCallback {
public:
    typedef enum {
        TRANSFER_TYPE_CONTROL = 0,
        TRANSFER_TYPE_ISOCHRONOUS,
        TRANSFER_TYPE_BULK,
        TRANSFER_TYPE_INTERRUPT,
    } TransferType;
    typedef enum {
        TRANSFER_DIRECTION_READ = 0,
        TRANSFER_DIRECTION_WRITE,
        TRANSFER_DIRECTION_IN = TRANSFER_DIRECTION_READ,
        TRANSFER_DIRECTION_OUT = TRANSFER_DIRECTION_WRITE,
    } TransferDirection;
    typedef enum {
        TRANSFER_ONCE = 0,
        TRANSFER_CONTINUE,
    } TransferAction;
    typedef enum {
        TRANSFER_COMPLETED = 0,
        TRANSFER_ERROR,
        TRANSFER_CANCELLED,
        TRANSFER_STALL,
        TRANSFER_NO_DEVICE,
        TRANSFER_OVERFLOW,
    } TransferStatus;
    
    typedef std::function<TransferAction (libusb_transfer *)> callback_t;
    USBTransferCallback(USBInterface *iface, callback_t callback);

    USBInterface *m_iface;
    callback_t m_callback;

private:
    TransferAction m_cb(libusb_transfer *transfer) { return TRANSFER_ONCE; }
    USBTransferCallback(const USBTransferCallback&);
    USBTransferCallback& operator=(const USBTransferCallback&);
};

#endif // USBTRANSFERCALLBACK_H
