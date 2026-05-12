#include "async/AsyncTransfer.h"
#include <iostream>

// 传输完成回调包装函数
// libusb异步传输完成时调用此函数
// 参数: transfer - libusb传输结构指针
static void LIBUSB_CALL transferCallbackWrapper(libusb_transfer* transfer)
{
    // 从transfer的user_data中获取AsyncTransfer对象指针
    AsyncTransfer* async = static_cast<AsyncTransfer*>(transfer->user_data);
    // 调用AsyncTransfer的处理函数
    async->handleTransferComplete(transfer);
}

// 构造函数：初始化成员变量
// 参数: context - USB上下文引用, device - USB设备引用
AsyncTransfer::AsyncTransfer(UsbContext& context, UsbDevice& device)
    : context_(context)    // 保存USB上下文引用
    , device_(device)      // 保存USB设备引用
    , running_(false)      // 运行标志初始化为false
{
}

// 析构函数：停止事件循环
AsyncTransfer::~AsyncTransfer()
{
    stopEventLoop();
}

// 启动异步读取
// 参数: endpoint - 端点地址, max_packet - 最大包大小
//      type - 传输类型, callback - 数据回调函数
// 返回: true成功, false失败
bool AsyncTransfer::startRead(uint8_t endpoint, uint16_t max_packet,
                               TransferType type, DataCallback callback)
{
    std::lock_guard<std::mutex> lock(read_mutex_);  // 加锁保护

    // 分配libusb传输结构
    libusb_transfer* transfer = libusb_alloc_transfer(0);
    if (!transfer) {
        std::cerr << "Failed to allocate transfer" << std::endl;
        return false;
    }

    // 分配数据缓冲区
    uint8_t* buffer = new uint8_t[max_packet];

    // 根据传输类型填充传输结构
    if (type == TransferType::BULK) {
        // 批量传输
        libusb_fill_bulk_transfer(transfer, device_.getHandle(),
                                   endpoint, buffer, max_packet,
                                   transferCallbackWrapper, this, 1000);
    } else {
        // 中断传输
        libusb_fill_interrupt_transfer(transfer, device_.getHandle(),
                                        endpoint, buffer, max_packet,
                                        transferCallbackWrapper, this, 1000);
    }

    // 设置自动释放缓冲区标志（读端点不需要自动释放transfer，因为要重新提交）
    transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;

    // 创建读取端点信息
    ReadEndpoint ep;
    ep.address = endpoint;           // 保存端点地址
    ep.max_packet = max_packet;      // 保存包大小
    ep.type = type;                  // 保存传输类型
    ep.callback = callback;          // 保存回调函数
    ep.transfer = transfer;          // 保存传输结构指针
    ep.stopping = false;             // 初始化停止标志

    read_endpoints_.push_back(ep);  // 添加到读取端点列表

    // 打印调试信息
    std::cout << "Submitting transfer: endpoint=0x" << std::hex << static_cast<int>(endpoint)
              << ", max_packet=" << std::dec << max_packet
              << ", type=" << (type == TransferType::BULK ? "Bulk" : "Interrupt")
              << std::endl;

    // 提交传输请求
    int ret = libusb_submit_transfer(transfer);
    if (ret != LIBUSB_SUCCESS) {
        std::cerr << "Failed to submit transfer: " << libusb_error_name(ret) << std::endl;
        libusb_free_transfer(transfer);  // 失败则释放资源
        read_endpoints_.pop_back();      // 从列表中移除
        return false;
    }

    return true;
}

// 停止指定端点的异步读取
// 参数: endpoint - 端点地址
// 返回: true成功, false失败
bool AsyncTransfer::stopRead(uint8_t endpoint)
{
    std::lock_guard<std::mutex> lock(read_mutex_);  // 加锁保护

    // 查找匹配的端点
    for (auto& ep : read_endpoints_) {
        if (ep.address == endpoint) {
            ep.stopping = true;  // 设置停止标志
            libusb_cancel_transfer(ep.transfer);  // 取消传输
            return true;
        }
    }
    return false;  // 未找到
}

// 提交异步写入请求
// 参数: endpoint - 端点地址, data - 要发送的数据
//      type - 传输类型, callback - 完成回调函数
// 返回: true成功, false失败
bool AsyncTransfer::submitWrite(uint8_t endpoint, const std::vector<uint8_t>& data,
                                 TransferType type, DataCallback callback)
{
    // 分配libusb传输结构
    libusb_transfer* transfer = libusb_alloc_transfer(0);
    if (!transfer) {
        std::cerr << "Failed to allocate transfer" << std::endl;
        return false;
    }

    // 分配并复制数据到缓冲区
    uint8_t* buffer = new uint8_t[data.size()];
    std::copy(data.begin(), data.end(), buffer);

    // 根据传输类型填充传输结构
    if (type == TransferType::BULK) {
        libusb_fill_bulk_transfer(transfer, device_.getHandle(),
                                   endpoint, buffer, static_cast<int>(data.size()),
                                   transferCallbackWrapper, this, 0);
    } else {
        libusb_fill_interrupt_transfer(transfer, device_.getHandle(),
                                        endpoint, buffer, static_cast<int>(data.size()),
                                        transferCallbackWrapper, this, 0);
    }

    // 设置自动释放缓冲区标志
    transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;

    // 提交传输请求
    int ret = libusb_submit_transfer(transfer);
    if (ret != LIBUSB_SUCCESS) {
        std::cerr << "Failed to submit write transfer: " << libusb_error_name(ret) << std::endl;
        libusb_free_transfer(transfer);  // 失败则释放资源
        delete[] buffer;
        return false;
    }

    return true;
}

// 运行事件循环（在独立线程中）
// 持续处理libusb事件，直到running_被设为false
void AsyncTransfer::runEventLoop()
{
    running_ = true;  // 设置运行标志
    while (running_) {
        struct timeval tv = {0, 100000};  // 超时时间：100ms
        // 处理libusb事件（非阻塞）
        int ret = libusb_handle_events_timeout(context_.getContext(), &tv);
        if (ret != LIBUSB_SUCCESS) {
            std::cerr << "libusb handle events error: " << libusb_error_name(ret) << std::endl;
        }
    }
}

// 停止事件循环
void AsyncTransfer::stopEventLoop()
{
    running_ = false;  // 设置停止标志
    // 等待线程结束
    if (event_thread_.joinable()) {
        event_thread_.join();
    }
}

// 检查事件循环是否正在运行
// 返回: true运行中, false未运行
bool AsyncTransfer::isEventLoopRunning() const
{
    return running_ && event_thread_.joinable();
}

// 启动事件循环线程
void AsyncTransfer::startEventLoopThread()
{
    // 如果线程未运行，则创建新线程
    if (!event_thread_.joinable()) {
        event_thread_ = std::thread(&AsyncTransfer::runEventLoop, this);
    }
}

// 处理传输完成事件
// 参数: transfer - libusb传输结构指针
void AsyncTransfer::handleTransferComplete(libusb_transfer* transfer)
{
    TransferResult result;
    result.status = transfer->status;         // 保存传输状态
    result.actual_length = transfer->actual_length;  // 保存实际传输长度

    // 如果传输成功，复制数据
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
        result.data.assign(transfer->buffer, transfer->buffer + transfer->actual_length);
    }

    std::lock_guard<std::mutex> lock(read_mutex_);  // 加锁保护

    // 查找匹配的读取端点
    for (auto it = read_endpoints_.begin(); it != read_endpoints_.end(); ++it) {
        if (it->transfer == transfer) {
            // 调用回调函数处理结果
            if (it->callback) {
                it->callback(result);
            }

            // 如果正在停止，不重新提交，清理资源
            if (it->stopping) {
                // 注意：不要手动释放buffer和transfer
                // libusb会自动处理，因为flags设置了LIBUSB_TRANSFER_FREE_BUFFER
                read_endpoints_.erase(it);  // 从列表中移除
                return;
            }

            // 如果仍在运行，重新提交传输（持续读取）
            if (running_) {
                int ret = libusb_submit_transfer(transfer);
                if (ret != LIBUSB_SUCCESS) {
                    std::cerr << "Failed to resubmit transfer: "
                              << libusb_error_name(ret) << std::endl;
                }
            }
            return;
        }
    }

    // 如果不是读取端点（是写入端点），释放资源
    if (transfer->flags & LIBUSB_TRANSFER_FREE_BUFFER) {
        delete[] transfer->buffer;
    }
    libusb_free_transfer(transfer);
}
