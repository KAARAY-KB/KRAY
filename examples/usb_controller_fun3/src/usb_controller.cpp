#include "usb_controller.hpp"

namespace usb_ctrl {

UsbController::UsbController() {
    _ctx = std::make_unique<UsbContext>();
    _manager = std::make_unique<UsbDeviceManager>(*_ctx);
}

UsbController::~UsbController() {
    close_hid_device();
    async_stop();
}

void UsbController::set_debug_level(int level) { _ctx->set_debug_level(level); }
void UsbController::refresh_devices() { _manager->refresh(); }

std::string UsbController::list_devices() const { return _manager->list_summary(); }
std::string UsbController::list_devices_detail() const { return _manager->list_detail(); }
std::string UsbController::list_devices_tree() const { return _manager->list_tree(); }
std::string UsbController::list_hid_devices() const { return _manager->list_hid_summary(); }

size_t UsbController::device_count() const { return _manager->count(); }
std::string UsbController::device_detail(size_t index) const {
    auto& devs = _manager->devices();
    if (index >= devs.size()) return "Invalid device index";
    return devs[index].detail();
}

std::vector<UsbDevice> UsbController::find_by_vid_pid(uint16_t vid, uint16_t pid) {
    return _manager->find_by_vid_pid(vid, pid);
}
std::vector<UsbDevice> UsbController::find_hid_devices() {
    return _manager->find_hid_devices();
}

bool UsbController::open_hid_device(size_t index) {
    close_hid_device();
    auto& devs = _manager->devices();
    if (index >= devs.size()) return false;
    auto hid = std::make_unique<HidDevice>(devs[index]);
    if (!hid->open()) return false;
    _hid_device = std::move(hid);
    _sync_transfer = std::make_unique<SyncTransfer>(_hid_device->transfer().handle());
    return true;
}

bool UsbController::open_hid_device_by_vid_pid(uint16_t vid, uint16_t pid) {
    auto devs = _manager->find_by_vid_pid(vid, pid);
    for (auto& dev : devs) {
        if (dev.has_hid_interface()) {
            auto hid = std::make_unique<HidDevice>(dev);
            if (hid->open()) {
                close_hid_device();
                _hid_device = std::move(hid);
                _sync_transfer = std::make_unique<SyncTransfer>(_hid_device->transfer().handle());
                return true;
            }
        }
    }
    return false;
}

void UsbController::close_hid_device() {
    _async_hid.reset();
    _sync_transfer.reset();
    _hid_device.reset();
}

bool UsbController::is_hid_open() const { return _hid_device && _hid_device->is_open(); }

std::string UsbController::hid_device_info() const {
    if (!_hid_device) return "No HID device opened";
    return _hid_device->device_summary();
}

TransferResult UsbController::hid_read(int length, unsigned int timeout_ms) {
    if (!_hid_device) return {false, 0, 0, "No HID device opened", {}};
    return _hid_device->read_report(length, timeout_ms);
}

TransferResult UsbController::hid_write(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_hid_device) return {false, 0, 0, "No HID device opened", {}};
    return _hid_device->write_report(data, timeout_ms);
}

TransferResult UsbController::hid_get_feature_report(uint8_t report_id, int length, unsigned int timeout_ms) {
    if (!_hid_device) return {false, 0, 0, "No HID device opened", {}};
    return _hid_device->get_feature_report(report_id, length, timeout_ms);
}

TransferResult UsbController::hid_send_feature_report(const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_hid_device) return {false, 0, 0, "No HID device opened", {}};
    return _hid_device->send_feature_report(data, timeout_ms);
}

TransferResult UsbController::bulk_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    if (!_sync_transfer) return {false, 0, 0, "No sync transfer available", {}};
    return _sync_transfer->bulk_read(endpoint, length, timeout_ms);
}

TransferResult UsbController::bulk_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_sync_transfer) return {false, 0, 0, "No sync transfer available", {}};
    return _sync_transfer->bulk_write(endpoint, data, timeout_ms);
}

TransferResult UsbController::interrupt_read(uint8_t endpoint, int length, unsigned int timeout_ms) {
    if (!_sync_transfer) return {false, 0, 0, "No sync transfer available", {}};
    return _sync_transfer->interrupt_read(endpoint, length, timeout_ms);
}

TransferResult UsbController::interrupt_write(uint8_t endpoint, const std::vector<uint8_t>& data, unsigned int timeout_ms) {
    if (!_sync_transfer) return {false, 0, 0, "No sync transfer available", {}};
    return _sync_transfer->interrupt_write(endpoint, data, timeout_ms);
}

void UsbController::async_start() {
    if (!_async_engine) {
        _async_engine = std::make_unique<AsyncTransferEngine>(_ctx->handle());
        _async_engine->start();
    }
}

void UsbController::async_stop() {
    _async_hid.reset();
    if (_async_engine) {
        _async_engine->stop();
        _async_engine.reset();
    }
}

void UsbController::hid_read_async(int length, AsyncCallback callback, unsigned int timeout_ms) {
    if (!_hid_device || !_hid_device->is_open()) return;
    async_start();
    if (!_async_hid)
        _async_hid = std::make_unique<AsyncHidTransfer>(*_async_engine, _hid_device->handle(),
                                                         _hid_device->in_endpoint().address,
                                                         _hid_device->out_endpoint().address);
    _async_hid->read_async(length, std::move(callback), timeout_ms);
}

void UsbController::hid_write_async(const std::vector<uint8_t>& data, AsyncCallback callback, unsigned int timeout_ms) {
    if (!_hid_device || !_hid_device->is_open()) return;
    async_start();
    if (!_async_hid)
        _async_hid = std::make_unique<AsyncHidTransfer>(*_async_engine, _hid_device->handle(),
                                                         _hid_device->in_endpoint().address,
                                                         _hid_device->out_endpoint().address);
    _async_hid->write_async(data, std::move(callback), timeout_ms);
}

void UsbController::hid_read_continuous(int length, AsyncCallback callback, unsigned int timeout_ms) {
    if (!_hid_device || !_hid_device->is_open()) return;
    async_start();
    if (!_async_hid)
        _async_hid = std::make_unique<AsyncHidTransfer>(*_async_engine, _hid_device->handle(),
                                                         _hid_device->in_endpoint().address,
                                                         _hid_device->out_endpoint().address);
    _async_hid->read_continuous(length, std::move(callback), timeout_ms);
}

void UsbController::hid_stop_continuous() {
    if (_async_hid) _async_hid->stop_continuous();
}

size_t UsbController::async_pending_count() const {
    return _async_engine ? _async_engine->pending_count() : 0;
}

} // namespace usb_ctrl
