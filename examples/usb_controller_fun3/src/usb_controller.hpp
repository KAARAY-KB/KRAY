#pragma once

#include "usb_core/usb_context.hpp"
#include "usb_core/usb_device.hpp"
#include "usb_core/usb_device_manager.hpp"
#include "usb_transfer/usb_transfer.hpp"
#include "usb_transfer/hid_device.hpp"
#include "usb_transfer/async_transfer.hpp"

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace usb_ctrl {

using core::UsbContext;
using core::UsbDevice;
using core::UsbDeviceManager;
using core::DeviceInfo;
using core::EndpointInfo;
using transfer::TransferResult;
using transfer::SyncTransfer;
using transfer::HidDevice;
using transfer::AsyncTransferEngine;
using transfer::AsyncHidTransfer;
using transfer::AsyncCallback;

class UsbController {
public:
    UsbController();
    ~UsbController();

    UsbController(const UsbController&) = delete;
    UsbController& operator=(const UsbController&) = delete;

    void set_debug_level(int level);
    void refresh_devices();

    std::string list_devices() const;
    std::string list_devices_detail() const;
    std::string list_devices_tree() const;
    std::string list_hid_devices() const;

    size_t device_count() const;
    std::string device_detail(size_t index) const;

    std::vector<UsbDevice>& devices() { return _manager->devices(); }
    const std::vector<UsbDevice>& devices() const { return _manager->devices(); }

    std::vector<UsbDevice> find_by_vid_pid(uint16_t vid, uint16_t pid);
    std::vector<UsbDevice> find_hid_devices();

    bool open_hid_device(size_t index);
    bool open_hid_device_by_vid_pid(uint16_t vid, uint16_t pid);
    void close_hid_device();
    bool is_hid_open() const;
    std::string hid_device_info() const;

    TransferResult hid_read(int length, unsigned int timeout_ms = 1000);
    TransferResult hid_write(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);
    TransferResult hid_get_feature_report(uint8_t report_id, int length, unsigned int timeout_ms = 1000);
    TransferResult hid_send_feature_report(const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    TransferResult bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);
    TransferResult bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);
    TransferResult interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms = 1000);
    TransferResult interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms = 1000);

    void async_start();
    void async_stop();

    void hid_read_async(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);
    void hid_write_async(const std::vector<uint8_t>& data, AsyncCallback callback, unsigned int timeout_ms = 1000);
    void hid_read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms = 1000);
    void hid_stop_continuous();

    size_t async_pending_count() const;

    HidDevice* hid_device() { return _hid_device.get(); }
    SyncTransfer* sync_transfer() { return _sync_transfer.get(); }
    AsyncTransferEngine* async_engine() { return _async_engine.get(); }

private:
    std::unique_ptr<UsbContext> _ctx;
    std::unique_ptr<UsbDeviceManager> _manager;
    std::unique_ptr<HidDevice> _hid_device;
    std::unique_ptr<SyncTransfer> _sync_transfer;
    std::unique_ptr<AsyncTransferEngine> _async_engine;
    std::unique_ptr<AsyncHidTransfer> _async_hid;
};

} // namespace usb_ctrl
