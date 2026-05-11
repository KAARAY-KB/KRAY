#include "USBInterface.h"
#include "console.h"

#include <assert.h>
#include <memory>
#include <cstring>



USBInterface::USBInterface(libusb_device_handle* handle, int interface, bool try_detach)
    : m_handle(handle)
    , m_interface(interface)
    , m_try_detach(try_detach)
    , m_endpoint()
    , m_isValid(false)
{
    m_isValid = (USBHelper::claim_interface(m_handle, m_interface) == LIBUSB_SUCCESS);
}
USBInterface::~USBInterface() {
    // cancel all transfers
    for (std::map<int, libusb_transfer*>::iterator it = m_endpoint.begin(); it != m_endpoint.end(); ++it) {
        if (it->second) {
            if (libusb_cancel_transfer(it->second) != LIBUSB_SUCCESS) {
                Console::out() << "error canceling transfer: " << it->first << std::endl;
            }
            // wait for cancel to succeed
            int ret = libusb_handle_events(NULL);
            if (ret != LIBUSB_SUCCESS) {
                Console::out() << "libusb_handle_events() failure: " << ret << std::endl;
            }
            // libusb_free_transfer(it->second);
        }
        // cancel_transfer(it->first);
    }
    if (m_endpoint.size()) {
        m_endpoint.clear();
    }
    USBHelper::release_interface(m_handle, m_interface);
}

void USBInterface::cancel_transfer(int endpoint) {
    std::map<int, libusb_transfer*>::iterator it = m_endpoint.find(endpoint);
    if (it != m_endpoint.end()) {
        m_endpoint.erase(it);
        if (it->second) {
            if (libusb_cancel_transfer(it->second) != LIBUSB_SUCCESS) {
                Console::out() << "error canceling transfer: " << it->first << std::endl;
            }
            // wait for cancel to succeed
            int ret = libusb_handle_events(NULL);
            if (ret != LIBUSB_SUCCESS) {
                Console::out() << "libusb_handle_events() failure: " << ret << std::endl;
            }
            // libusb_free_transfer(it->second);
        }
    }
    else {
        Console::out() << "endpoint not found: " << endpoint << std::endl;
    }
}
void USBInterface::cancel_read(int endpoint) {
    cancel_transfer(endpoint | LIBUSB_ENDPOINT_IN);
}
void USBInterface::cancel_write(int endpoint) {
    cancel_transfer(endpoint | LIBUSB_ENDPOINT_OUT);
}
void USBInterface::cancel_control(int endpoint) {
    cancel_transfer(endpoint | LIBUSB_ENDPOINT_OUT);
}

void USBInterface::submit_read(int endpoint, int len, USBTransferCallback::callback_t cb, uint32_t timout) {
    if (m_endpoint.find(endpoint) != m_endpoint.end()) {
        Console::out() << "endpoint already in use: " << endpoint << std::endl;
        return;
    }
    libusb_transfer* transfer = libusb_alloc_transfer(0);
    transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
    endpoint        |= LIBUSB_ENDPOINT_IN;
    uint8_t *data = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * len));

    USBTransferCallback *transfer_cb = new USBTransferCallback(this, cb);
    libusb_fill_interrupt_transfer(transfer, m_handle, endpoint, data, len, &USBInterface::on_read_wrap, transfer_cb, timout);
    int ret = libusb_submit_transfer(transfer);
    if (ret != LIBUSB_SUCCESS) {
        libusb_free_transfer(transfer);
        Console::out() << "submit_read() libusb_submit_transfer(): " << libusb_error_name(ret) << std::endl;
        return;
    }
    m_endpoint[endpoint] = transfer;
}
void USBInterface::submit_write(int endpoint, int len, uint8_t *buf, USBTransferCallback::callback_t cb, uint32_t timout) {
    if (m_endpoint.find(endpoint) != m_endpoint.end()) {
        Console::out() << "endpoint already in use: " << endpoint << std::endl;
        return;
    }
    libusb_transfer* transfer = libusb_alloc_transfer(0);
    transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
    endpoint        |= LIBUSB_ENDPOINT_OUT;
    uint8_t *data = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * len));
    memcpy(data, buf, len);
    
    USBTransferCallback *transfer_cb = new USBTransferCallback(this, cb);
    libusb_fill_interrupt_transfer(transfer, m_handle, endpoint, data, len, &USBInterface::on_write_wrap, transfer_cb, timout);
    int ret = libusb_submit_transfer(transfer);
    if (ret != LIBUSB_SUCCESS) {
        libusb_free_transfer(transfer);
        Console::out() << "submit_write() libusb_submit_transfer(): " << libusb_error_name(ret) << std::endl;
        return;
    }
    m_endpoint[endpoint] = transfer;
}
void USBInterface::submit_control(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t *wBuf, uint16_t wLength, USBTransferCallback::callback_t cb, uint32_t timout) {
    int endpoint = LIBUSB_ENDPOINT_OUT | 0;
    if (m_endpoint.find(endpoint) != m_endpoint.end()) {
        Console::out() << "endpoint already in use: " << endpoint << std::endl;
        return;
    }
    const uint8_t wSetupLen = 8;
    libusb_transfer *transfer = libusb_alloc_transfer(0);
    transfer->flags  |= LIBUSB_TRANSFER_FREE_BUFFER;
    uint8_t *data = static_cast<uint8_t *>(malloc(wLength + wSetupLen));

    libusb_fill_control_setup(data, bmRequestType, bRequest, wValue, wIndex, wLength);
    memcpy(data + wSetupLen, wBuf, wLength);

    USBTransferCallback *transfer_cb = new USBTransferCallback(this, cb);
    libusb_fill_control_transfer(transfer, m_handle, data, &USBInterface::on_control_wrap, transfer_cb, timout);
    int ret = libusb_submit_transfer(transfer);
    if (ret != LIBUSB_SUCCESS) {
        libusb_free_transfer(transfer);
        Console::out() << "submit_control() libusb_submit_transfer(): " << libusb_error_name(ret) << std::endl;
        return;
    }
    m_endpoint[endpoint] = transfer;
}

void USBInterface::on_read(libusb_transfer *transfer, USBTransferCallback *transferCallback) {
    USBTransferCallback::TransferAction action = transferCallback->m_callback(transfer);
   // Console::out() << "on_read() action: " << action;
    if (action == USBTransferCallback::TRANSFER_ONCE) {
        libusb_free_transfer(transfer);
        m_endpoint.erase(transfer->endpoint);
        delete transferCallback;
    }
    else if (action == USBTransferCallback::TRANSFER_CONTINUE) {
        int ret = libusb_submit_transfer(transfer);
        if (ret != LIBUSB_SUCCESS) {
            libusb_free_transfer(transfer);
            Console::out() << "libusb_submit_transfer(): " << libusb_error_name(ret) << std::endl;
        }
    }
}
void USBInterface::on_write(libusb_transfer *transfer, USBTransferCallback *transferCallback) {
    USBTransferCallback::TransferAction action = transferCallback->m_callback(transfer);
    if (action == USBTransferCallback::TRANSFER_ONCE) {
        libusb_free_transfer(transfer);
        m_endpoint.erase(transfer->endpoint);
        delete transferCallback;
    }
    else if (action == USBTransferCallback::TRANSFER_CONTINUE) {
        int ret = libusb_submit_transfer(transfer);
        if (ret != LIBUSB_SUCCESS) {
            libusb_free_transfer(transfer);
            Console::out() << "libusb_submit_transfer(): " << libusb_error_name(ret) << std::endl;
        }
    }
}
void USBInterface::on_control(libusb_transfer *transfer, USBTransferCallback *transferCallback) {
    USBTransferCallback::TransferAction action = transferCallback->m_callback(transfer);
    if (action == USBTransferCallback::TRANSFER_ONCE) {
        libusb_free_transfer(transfer);
        m_endpoint.erase(transfer->endpoint);
        delete transferCallback;
    }
    else if (action == USBTransferCallback::TRANSFER_CONTINUE) {
        int ret = libusb_submit_transfer(transfer);
        if (ret != LIBUSB_SUCCESS) {
            libusb_free_transfer(transfer);
            Console::out() << "libusb_submit_transfer(): " << libusb_error_name(ret) << std::endl;
        }
    }
}

void USBInterface::on_read_wrap(libusb_transfer *transfer) {
    USBTransferCallback *transfer_cb = static_cast<USBTransferCallback*>(transfer->user_data);
    transfer_cb->m_iface->on_read(transfer, transfer_cb);
}
void USBInterface::on_write_wrap(libusb_transfer *transfer) {
    USBTransferCallback *transfer_cb = static_cast<USBTransferCallback*>(transfer->user_data);
    transfer_cb->m_iface->on_write(transfer, transfer_cb);
}
void USBInterface::on_control_wrap(libusb_transfer* transfer) {
    USBTransferCallback *transfer_cb = static_cast<USBTransferCallback*>(transfer->user_data);
    transfer_cb->m_iface->on_control(transfer, transfer_cb);
}
