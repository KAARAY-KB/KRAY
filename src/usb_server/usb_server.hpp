// ============================================================================
// usb_server.hpp - USB TCP 服务器（头文件）
//
// 在 KRAY 进程内运行的 TCP 服务器，提供 USB 控制能力。
// 客户端通过自定义二进制协议连接后可执行设备枚举、HID 操作、热插拔监听等。
//
// 使用方式：
//   UsbServer server;
//   server.start("127.0.0.1", 9120);  // 启动服务
//   // ... 运行中 ...
//   server.stop();                      // 停止服务
// ============================================================================

#pragma once

// 平台 Socket 头文件（必须在 windows.h 之前包含）
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include "usb_handler.hpp"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

#ifdef _WIN32
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using socket_t = int;
    constexpr socket_t INVALID_SOCK = -1;
#endif

namespace usb_srv {

// ============================================================================
// ClientConn - 客户端连接信息
// ============================================================================
struct ClientConn {
    socket_t fd = INVALID_SOCK;             // 套接字描述符
    std::vector<uint8_t> recv_buf;          // 接收缓冲区（处理粘包/拆包）
};

// ============================================================================
// UsbServer - USB TCP 服务器
//
// 工作方式：
//   1. start() 创建监听线程，接受客户端连接
//   2. 每个客户端连接在独立线程中处理
//   3. 接收数据 → 协议解码 → UsbHandler 处理 → 协议编码 → 发送响应
//   4. 热插拔事件通过通知回调主动推送给所有客户端
//   5. stop() 关闭所有连接并等待线程退出
// ============================================================================
class UsbServer {
public:
    // 构造函数
    UsbServer();

    // 析构函数：自动停止服务
    ~UsbServer();

    // 禁止拷贝
    UsbServer(const UsbServer&) = delete;
    UsbServer& operator=(const UsbServer&) = delete;

    // 启动 TCP 服务器
    // @param host 监听地址（如 "127.0.0.1"）
    // @param port 监听端口
    // @return true 启动成功
    bool start(const std::string& host = "127.0.0.1", uint16_t port = 9120);

    // 停止 TCP 服务器
    void stop();

    // 检查服务器是否正在运行
    bool is_running() const { return _running.load(std::memory_order_acquire); }

    // 获取监听端口
    uint16_t port() const { return _port; }

private:
    // 接受连接的主循环（在独立线程中运行）
    void _accept_loop();

    // 处理单个客户端连接（在独立线程中运行）
    // @param client 客户端连接信息
    void _client_loop(ClientConn client);

    // 向指定客户端发送消息
    // @param fd  客户端套接字
    // @param msg 要发送的消息
    void _send_msg(socket_t fd, const Message& msg);

    // 向所有客户端广播通知消息
    // @param msg 通知消息
    void _broadcast(const Message& msg);

    // 关闭套接字（跨平台）
    void _close_socket(socket_t fd);

    // 初始化/清理网络库（Windows 需要 WSA）
    static void _net_init();
    static void _net_cleanup();

    socket_t _listen_fd = INVALID_SOCK;     // 监听套接字
    std::atomic<bool> _running{false};      // 运行标志
    uint16_t _port = 0;                     // 监听端口

    std::thread _accept_thread;             // 接受连接线程

    std::mutex _clients_mutex;              // 客户端列表互斥锁
    std::vector<socket_t> _client_fds;      // 所有客户端套接字

    std::unique_ptr<UsbHandler> _handler;   // USB 请求处理器
};

} // namespace usb_srv
