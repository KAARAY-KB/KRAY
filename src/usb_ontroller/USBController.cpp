#include "USBController.h"

#include <iostream>
#include <assert.h>
#include <memory>
#include <cstring>


USBController::USBController(libusb_device* dev, std::set<int> _interfaces)
    : m_dev(dev)
    , m_handle(nullptr)
    , m_transfers()
    , m_dev_info()
    , interfaces()
{
    char ch[1024];
    int len, ret;
    uint8_t bus, addr;
    struct libusb_device_descriptor desc;

    ret = libusb_open(dev, &m_handle);
    std::cout << "libusb_open() " << libusb_strerror(ret) << std::endl;
    if (ret != LIBUSB_SUCCESS) {
        return;
    }

    if (_interfaces.size() > 0) {
        for (auto i : _interfaces) {
            USBInterface *intf = new USBInterface(m_handle, i);
            if (intf->isValid()) {
                interfaces[i] = intf;
                std::cout << "interface " << i << " is valid" << std::endl;
            }
            else {
                delete intf;
                std::cout << "interface " << i << " is not valid" << std::endl;
            }
        }
    }
    
    // "%03d:%03d"
    bus  = libusb_get_bus_number(dev);
    addr = libusb_get_device_address(dev);
    ret  = libusb_get_device_descriptor(dev, &desc);
    if (ret != LIBUSB_SUCCESS) {
        std::cout << "libusb_get_device_descriptor() failed: " << libusb_strerror(ret) << std::endl;
        return;
    }

    len = libusb_get_string_descriptor_ascii(m_handle, desc.iManufacturer, reinterpret_cast<unsigned char *>(ch), sizeof(ch));
    if (len > 0) {
        m_dev_info.ch.mfr.append(ch, len);
    }

    len = libusb_get_string_descriptor_ascii(m_handle, desc.iProduct, reinterpret_cast<unsigned char *>(ch), sizeof(ch));
    if (len > 0) {
        m_dev_info.ch.prod.append(ch, len);
    }

    len = libusb_get_string_descriptor_ascii(m_handle, desc.iSerialNumber, reinterpret_cast<unsigned char *>(ch), sizeof(ch));
    if (len > 0) {
        m_dev_info.ch.sn.append(ch, len);
    }
    
    // "%04x:%04x"
    m_dev_info.id.vid = desc.idVendor;
    m_dev_info.id.pid = desc.idProduct;
    
    std::cout << "USBController: " << std::endl;
    USBHelper::printf_msg(&m_dev_info, true);
}

USBController::~USBController() {
    for (std::map<int, USBInterface *>::iterator it = interfaces.begin(); it != interfaces.end(); ++it) {
        if (it->second) {
            delete it->second;
        }
        interfaces.erase(it);
    }
    if (interfaces.size()) {
        interfaces.clear();
    }
    
    // // cancel all transfers
    // for (std::set<libusb_transfer *>::iterator it = m_transfers.begin(); it != m_transfers.end(); ++it) {
    //     libusb_cancel_transfer(*it);
    // }

    // // wait for cancel to succeed
    // while (!m_transfers.empty()) {
    //     int ret = libusb_handle_events(NULL);
    //     if (ret != LIBUSB_SUCCESS) {
    //         std::cout << "libusb_handle_events() failure: " << ret << std::endl;
    //     }
    // }

    // // release all claimed interfaces
    // for (std::set<int>::iterator it = interfaces.begin(); it != interfaces.end(); ++it) {
    //     libusb_release_interface(m_handle, *it);
    // }

    // read and write transfers might still be going on and might need to be canceled
    libusb_close(m_handle);
}

void USBController::read_async(uint8_t endpoint, int len, uint32_t timeout) {
    libusb_transfer* xfer = libusb_alloc_transfer(0);
    uint8_t *data = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * len));
    xfer->flags  |= LIBUSB_TRANSFER_FREE_BUFFER;
    endpoint     &= ~LIBUSB_ENDPOINT_DIR_MASK; //clear dir
    endpoint     |= LIBUSB_ENDPOINT_IN;

    libusb_fill_interrupt_transfer(xfer, m_handle, endpoint, data, len, &USBController::on_read_wrap, this, timeout);
    int ret = libusb_submit_transfer(xfer);
    if (ret != LIBUSB_SUCCESS) {
        libusb_free_transfer(xfer);
        std::cout << "libusb_submit_transfer(): " << libusb_error_name(ret) << std::endl;
        return;
    }
    m_transfers.insert(xfer);
}
void USBController::write_async(uint8_t endpoint, uint8_t *buf, int len, uint32_t timeout) {
    libusb_transfer *xfer = libusb_alloc_transfer(0);
    uint8_t *data = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * len));
    xfer->flags  |= LIBUSB_TRANSFER_FREE_BUFFER;
    endpoint     &= ~LIBUSB_ENDPOINT_DIR_MASK; //clear dir
    endpoint     |= LIBUSB_ENDPOINT_OUT;
    memcpy(data, buf, len);

    libusb_fill_interrupt_transfer(xfer, m_handle, endpoint, data, len, &USBController::on_write_wrap, this, timeout);
    int ret = libusb_submit_transfer(xfer);
    if (ret != LIBUSB_SUCCESS) {
        libusb_free_transfer(xfer);
        std::cout << "libusb_submit_transfer(): " << libusb_error_name(ret) << std::endl;
        return;
    }
    m_transfers.insert(xfer);
}
void USBController::control_async(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t *wBuf, uint16_t wLength, uint32_t timeout) {
    const uint8_t wSetupLen = 8;
    libusb_transfer *xfer = libusb_alloc_transfer(0);
    uint8_t *data = static_cast<uint8_t *>(malloc(wLength + wSetupLen));
    xfer->flags  |= LIBUSB_TRANSFER_FREE_BUFFER;
    
    libusb_fill_control_setup(data, bmRequestType, bRequest, wValue, wIndex, wLength);
    memcpy(data + wSetupLen, wBuf, wLength);

    libusb_fill_control_transfer(xfer, m_handle, data, &USBController::on_control_wrap, this, timeout);
    int ret = libusb_submit_transfer(xfer);
    if (ret != LIBUSB_SUCCESS) {
        libusb_free_transfer(xfer);
        std::cout << "libusb_submit_transfer(): " << libusb_error_name(ret) << std::endl;
        return;
    }
    m_transfers.insert(xfer);
}

void USBController::on_control_wrap(struct libusb_transfer* xfer) {
    static_cast<USBController*>(xfer->user_data)->on_control(xfer);
}
void USBController::on_control(libusb_transfer *xfer) {
    std::cout << "control xfer" << std::endl;
    m_transfers.erase(xfer);
    libusb_free_transfer(xfer);
}

void USBController::on_write_wrap(struct libusb_transfer *xfer) {
    static_cast<USBController*>(xfer->user_data)->on_write(xfer);
}
void USBController::on_write(libusb_transfer *xfer) {
    switch (xfer->status) {
        case LIBUSB_TRANSFER_COMPLETED:
            write_cb(xfer->buffer, xfer->actual_length, NULL);
            std::cout << "write len:" << xfer->length << std::endl;
            break;
        case LIBUSB_TRANSFER_CANCELLED: std::cout << "transfer was cancelled" << std::endl;     break;
        case LIBUSB_TRANSFER_NO_DEVICE: std::cout << "device was disconnected" << std::endl;    break;
        default: std::cout << "write failure: " << libusb_error_name(xfer->status) << "len: " << xfer->length << std::endl; break;
    }
    m_transfers.erase(xfer);
    libusb_free_transfer(xfer);
}

void USBController::on_read_wrap(struct libusb_transfer *xfer) {
    static_cast<USBController*>(xfer->user_data)->on_read(xfer);
}
void USBController::on_read(libusb_transfer *xfer) {
    int ret;
    switch(xfer->status) {
        case LIBUSB_TRANSFER_COMPLETED:
            read_cb(xfer->buffer, xfer->actual_length, NULL);
            ret = libusb_submit_transfer(xfer);
            if (ret != LIBUSB_SUCCESS) {
                std::cout << "failed to resubmit USB xfer: " << libusb_error_name(ret) << std::endl;
                m_transfers.erase(xfer);
                libusb_free_transfer(xfer);
                if (ret == LIBUSB_ERROR_NO_DEVICE) {
                    std::cout << "device was disconnected" << std::endl;
                }
            }
            break;
        default:
            m_transfers.erase(xfer);
            libusb_free_transfer(xfer);
            if (xfer->status == LIBUSB_TRANSFER_CANCELLED) std::cout << "transfer was cancelled" << std::endl;
            else if (xfer->status == LIBUSB_TRANSFER_NO_DEVICE) std::cout << "device was disconnected" << std::endl;
            else std::cout << "read failure: " << libusb_error_name(xfer->status) << "len: " << xfer->length << std::endl;
            break;
    }
}



void USBController::view_all_endpoint() {
    libusb_config_descriptor *config = nullptr;
    if (LIBUSB_SUCCESS == libusb_get_config_descriptor(m_dev, 0, &config)) {
        const libusb_interface *interface = nullptr;
        const libusb_interface_descriptor *altsetting = nullptr;
        const libusb_endpoint_descriptor *endpoint = nullptr;
        for (interface = config->interface; interface != (config->interface + config->bNumInterfaces); ++interface) {
            for (altsetting = interface->altsetting; altsetting != (interface->altsetting + interface->num_altsetting); ++altsetting) {
                std::cout << "Interface: " << static_cast<int>(altsetting->bInterfaceNumber) << std::endl;
                for (endpoint = altsetting->endpoint; endpoint != (altsetting->endpoint + altsetting->bNumEndpoints); ++endpoint) {
                    std::cout << "    Endpoint: " << int(endpoint->bEndpointAddress & LIBUSB_ENDPOINT_ADDRESS_MASK) 
                                << "(" << ((endpoint->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) ? "IN" : "OUT") << ")" 
                                << "if_class" << altsetting->bInterfaceClass
                                << "if_subclass" << altsetting->bInterfaceSubClass
                                << "if_protocol" << altsetting->bInterfaceProtocol
                                << std::endl;
                }
            }
        }
        libusb_free_config_descriptor(config);
    }
}

int USBController::find_endpoint(int direction, uint8_t if_class, uint8_t if_subclass, uint8_t if_protocol) {
    libusb_config_descriptor *config;
    int ret = libusb_get_config_descriptor(m_dev, 0, &config);
    if (ret != LIBUSB_SUCCESS) {
        std::cout << "libusb_get_config_descriptor() failed: " << libusb_error_name(ret) << std::endl;
        return ret;
    }

    ret = -1;
    const libusb_interface *interface = nullptr;
    const libusb_interface_descriptor *altsetting = nullptr;
    const libusb_endpoint_descriptor *endpoint = nullptr;
    // FIXME: no need to search all interfaces, could just check the one we acutally use
    for (interface = config->interface; interface != (config->interface + config->bNumInterfaces); ++interface) {
        for (altsetting = interface->altsetting; altsetting != (interface->altsetting + interface->num_altsetting); ++altsetting) {
            std::cout << "Interface: " << static_cast<int>(altsetting->bInterfaceNumber) << std::endl;
            for (endpoint = altsetting->endpoint; endpoint != (altsetting->endpoint + altsetting->bNumEndpoints); ++endpoint) {
                std::cout << "    Endpoint: " << int(endpoint->bEndpointAddress & LIBUSB_ENDPOINT_ADDRESS_MASK) 
                          << "(" << ((endpoint->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) ? "IN" : "OUT") << ")" 
                          << "if_class" << altsetting->bInterfaceClass
                          << "if_subclass" << altsetting->bInterfaceSubClass
                          << "if_protocol" << altsetting->bInterfaceProtocol
                          << std::endl;

                if ((endpoint->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == direction &&
                    altsetting->bInterfaceClass == if_class &&
                    altsetting->bInterfaceSubClass == if_subclass &&
                    altsetting->bInterfaceProtocol == if_protocol) {
                    ret = static_cast<int>(endpoint->bEndpointAddress & LIBUSB_ENDPOINT_ADDRESS_MASK);
                }
            }
        }
    }

    libusb_free_config_descriptor(config);
    if (ret < 0) {
        std::cout << "Error couldn't find matching endpoint" << std::endl;
    }
    return ret;
}

uint16_t USBController::get_vid() const {
    return m_dev_info.id.vid;
}

uint16_t USBController::get_pid() const {
    return m_dev_info.id.pid;
}

std::string USBController::get_sn() const {
    return m_dev_info.ch.sn;
}

std::string USBController::get_mfr() const {
    return m_dev_info.ch.mfr;
}

std::string USBController::get_path() const {
    return m_dev_info.ch.path;
}

std::string USBController::get_prod() const {
    return m_dev_info.ch.prod;
}
