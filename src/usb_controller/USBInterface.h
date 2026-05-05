#ifndef USBINTERFACE_H
#define USBINTERFACE_H


#include <map>
#include "USBHelper.h"
#include "USBTransferCallback.h"


class USBInterface {
public:
    USBInterface(libusb_device_handle* handle, int interface, bool try_detach = true);
    ~USBInterface();

    void cancel_read(int endpoint);
    void cancel_write(int endpoint);
    void cancel_control(int endpoint = 0);

    void submit_read(int endpoint, int len, USBTransferCallback::callback_t cb, uint32_t timout);
    void submit_write(int endpoint, int len, uint8_t *buf, USBTransferCallback::callback_t cb, uint32_t timout);
    void submit_control(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t *wBuf, uint16_t wLength, USBTransferCallback::callback_t cb, uint32_t timout);

    bool isValid() const { return m_isValid; }

private:
    void cancel_transfer(int endpoint);

private:
    void on_read(libusb_transfer *transfer, USBTransferCallback *transferCallback);
    void on_write(libusb_transfer *transfer, USBTransferCallback *transferCallback);
    void on_control(libusb_transfer *transfer, USBTransferCallback *transferCallback);

    static void on_read_wrap(libusb_transfer *transfer);
    static void on_write_wrap(libusb_transfer *transfer);
    static void on_control_wrap(libusb_transfer* transfer);

private:
    int m_interface;
    bool m_try_detach;
    bool m_isValid;
    libusb_device_handle *m_handle;
    std::map<int, libusb_transfer*> m_endpoint;
    
private:
    USBInterface(const USBInterface&);
    USBInterface& operator=(const USBInterface&);
};

#endif // USBINTERFACE_H
