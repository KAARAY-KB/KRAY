// ============================================================================
// usb_server.cpp - USB TCP 服务器（实现文件）
//
// TCP 服务器核心逻辑：监听、连接管理、消息分发、通知广播
// ============================================================================

#include "usb_server.hpp"
#include "console.h"
#include <cstring>

namespace usb_srv {

// ============================================================================
// 构造/析构
// ============================================================================

// 构造函数：初始化网络库和请求处理器
UsbServer::UsbServer()
    : _handler(std::make_unique<UsbHandler>()) {
    _net_init();
}

// 析构函数：停止服务并清理网络库
UsbServer::~UsbServer() {
    stop();
    _net_cleanup();
}

// ============================================================================
// 启动/停止
// ============================================================================

// 启动 TCP 服务器
bool UsbServer::start(const std::string& host, uint16_t port) {
    // 防止重复启动
    if (_running.load()) return true;

    // 创建监听套接字
    _listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_fd == INVALID_SOCK) {
        Console::out() << "[UsbServer] socket() failed" << std::endl;
        return false;
    }

    // 允许地址复用（避免重启时端口占用）
    int opt = 1;
    setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    // 绑定地址和端口
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (::bind(_listen_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        Console::out() << "[UsbServer] bind() failed on " << host << ":" << port << std::endl;
        _close_socket(_listen_fd);
        _listen_fd = INVALID_SOCK;
        return false;
    }

    // 开始监听（backlog=5）
    if (::listen(_listen_fd, 5) != 0) {
        Console::out() << "[UsbServer] listen() failed" << std::endl;
        _close_socket(_listen_fd);
        _listen_fd = INVALID_SOCK;
        return false;
    }

    _port = port;
    _running.store(true);

    // 设置热插拔通知回调：广播给所有客户端
    _handler->set_notify_callback([this](const Message& msg) {
        _broadcast(msg);
    });

    // 启动接受连接线程
    _accept_thread = std::thread(&UsbServer::_accept_loop, this);

    Console::out() << "[UsbServer] Started on " << host << ":" << port << std::endl;
    return true;
}

// 停止 TCP 服务器
void UsbServer::stop() {
    if (!_running.load()) return;
    _running.store(false);

    // 关闭监听套接字（使 accept() 返回）
    _close_socket(_listen_fd);
    _listen_fd = INVALID_SOCK;

    // 关闭所有客户端连接
    {
        std::lock_guard<std::mutex> lock(_clients_mutex);
        for (auto fd : _client_fds) {
            _close_socket(fd);
        }
        _client_fds.clear();
    }

    // 等待接受线程退出
    if (_accept_thread.joinable()) {
        _accept_thread.join();
    }

    Console::out() << "[UsbServer] Stopped" << std::endl;
}

// ============================================================================
// 接受连接主循环
// ============================================================================
void UsbServer::_accept_loop() {
    while (_running.load()) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        // 等待客户端连接（阻塞）
        socket_t client_fd = ::accept(
            _listen_fd,
            reinterpret_cast<struct sockaddr*>(&client_addr),
            &addr_len
        );

        if (client_fd == INVALID_SOCK) {
            if (_running.load()) {
                Console::out() << "[UsbServer] accept() failed" << std::endl;
            }
            break;
        }

        // 添加到客户端列表
        {
            std::lock_guard<std::mutex> lock(_clients_mutex);
            _client_fds.push_back(client_fd);
        }

        // 为每个客户端启动处理线程
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        Console::out() << "[UsbServer] Client connected: " << ip_str << std::endl;

        ClientConn conn;
        conn.fd = client_fd;
        std::thread(&UsbServer::_client_loop, this, std::move(conn)).detach();
    }
}

// ============================================================================
// 客户端处理循环
// ============================================================================
void UsbServer::_client_loop(ClientConn client) {
    uint8_t buf[4096]; // 临时读取缓冲区

    while (_running.load()) {
        // 接收数据
        int n = ::recv(client.fd, reinterpret_cast<char*>(buf), sizeof(buf), 0);

        if (n <= 0) {
            // 连接关闭或出错
            break;
        }

        // 追加到接收缓冲区
        client.recv_buf.insert(client.recv_buf.end(), buf, buf + n);

        // 协议解码（处理粘包/拆包）
        std::vector<Message> msgs;
        decode(client.recv_buf, msgs);

        // 处理每条消息
        for (const auto& req : msgs) {
            Message rsp = _handler->handle(req);
            _send_msg(client.fd, rsp);
        }
    }

    // 客户端断开，从列表移除
    {
        std::lock_guard<std::mutex> lock(_clients_mutex);
        _client_fds.erase(
            std::remove(_client_fds.begin(), _client_fds.end(), client.fd),
            _client_fds.end()
        );
    }
    _close_socket(client.fd);
    Console::out() << "[UsbServer] Client disconnected" << std::endl;
}

// ============================================================================
// 消息发送
// ============================================================================

// 向指定客户端发送消息
void UsbServer::_send_msg(socket_t fd, const Message& msg) {
    auto data = encode(msg);
    size_t total = data.size();
    size_t sent = 0;

    // 循环发送，确保完整发送
    while (sent < total) {
        int n = ::send(fd, reinterpret_cast<const char*>(data.data() + sent),
                       static_cast<int>(total - sent), 0);
        if (n <= 0) break; // 发送失败
        sent += static_cast<size_t>(n);
    }
}

// 向所有客户端广播通知消息
void UsbServer::_broadcast(const Message& msg) {
    std::lock_guard<std::mutex> lock(_clients_mutex);
    for (auto fd : _client_fds) {
        _send_msg(fd, msg);
    }
}

// ============================================================================
// 平台辅助
// ============================================================================

// 关闭套接字（跨平台）
void UsbServer::_close_socket(socket_t fd) {
    if (fd == INVALID_SOCK) return;
#ifdef _WIN32
    ::closesocket(fd);
#else
    ::close(fd);
#endif
}

// 初始化网络库（Windows 需要 WSAStartup）
void UsbServer::_net_init() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

// 清理网络库（Windows 需要 WSACleanup）
void UsbServer::_net_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

} // namespace usb_srv
