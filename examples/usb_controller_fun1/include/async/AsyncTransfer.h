#ifndef ASYNC_TRANSFER_H
#define ASYNC_TRANSFER_H

#include "core/UsbContext.h"
#include "core/UsbDevice.h"
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <cstdint>

// 传输类型枚举
// 用于指定USB传输的方式
enum class TransferType {
    BULK,       // 批量传输：适合大量数据传输
    INTERRUPT   // 中断传输：适合HID等定时数据传输
};

// 传输结果结构体
// 存储异步传输完成后的结果信息
struct TransferResult {
    std::vector<uint8_t> data;  // 接收或发送的数据
    int status;                 // 传输状态码
    int actual_length;          // 实际传输的字节数
};

// 异步传输管理类
// 负责管理USB异步传输操作，支持多线程事件循环
class AsyncTransfer {
public:
    // 数据回调函数类型
    // 当异步传输完成时调用此函数处理结果
    using DataCallback = std::function<void(const TransferResult&)>;

    // 构造函数
    // 参数: context - USB上下文引用, device - USB设备引用
    AsyncTransfer(UsbContext& context, UsbDevice& device);
    
    // 析构函数：停止事件循环
    ~AsyncTransfer();

    // 启动异步读取
    // 参数: endpoint - 端点地址, max_packet - 最大包大小
    //      type - 传输类型, callback - 数据回调函数
    // 返回: true成功, false失败
    bool startRead(uint8_t endpoint, uint16_t max_packet,
                   TransferType type, DataCallback callback);
    
    // 停止指定端点的异步读取
    // 参数: endpoint - 端点地址
    // 返回: true成功, false失败
    bool stopRead(uint8_t endpoint);

    // 提交异步写入请求
    // 参数: endpoint - 端点地址, data - 要发送的数据
    //      type - 传输类型, callback - 完成回调函数
    // 返回: true成功, false失败
    bool submitWrite(uint8_t endpoint, const std::vector<uint8_t>& data,
                     TransferType type, DataCallback callback);

    // 运行事件循环（在独立线程中）
    // 处理所有异步传输的完成事件
    void runEventLoop();
    
    // 停止事件循环
    void stopEventLoop();
    
    // 检查事件循环是否正在运行
    // 返回: true运行中, false未运行
    bool isEventLoopRunning() const;
    
    // 启动事件循环线程
    void startEventLoopThread();

    // 处理传输完成事件（供回调函数调用）
    // 参数: transfer - libusb传输结构指针
    void handleTransferComplete(libusb_transfer* transfer);

private:

    UsbContext& context_;              // USB上下文引用
    UsbDevice& device_;                // USB设备引用
    std::atomic<bool> running_;        // 运行状态标志
    std::thread event_thread_;         // 事件循环线程

    // 读取端点信息结构体
    // 存储每个读取端点的配置和状态
    struct ReadEndpoint {
        uint8_t address;               // 端点地址
        uint16_t max_packet;           // 最大包大小
        TransferType type;             // 传输类型
        DataCallback callback;         // 数据回调函数
        libusb_transfer* transfer;     // libusb传输结构指针
        bool stopping;                 // 停止标志（防止重新提交）
    };

    std::vector<ReadEndpoint> read_endpoints_;  // 读取端点列表
    std::mutex read_mutex_;                     // 读取端点列表互斥锁
};

#endif
