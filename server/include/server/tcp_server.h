// =============================================================
// server/include/server/tcp_server.h
// TCP 监听服务器
//
// 职责：创建监听 socket，accept 新连接，创建 TcpConnection
// =============================================================

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace cloudvault {

class EventLoop;
class TcpConnection;

/**
 * @brief TCP 监听服务器。
 *
 * 负责创建监听 socket，接受新连接并生成 TcpConnection 对象。
 */
class TcpServer {
public:
    /**
     * @brief 新连接回调签名。
     */
    using NewConnectionCallback =
        std::function<void(std::shared_ptr<TcpConnection>)>;

    /**
     * @brief 构造监听服务器。
     * @param loop 事件循环指针。
     * @param host 监听地址。
     * @param port 监听端口。
     */
    TcpServer(EventLoop* loop, const std::string& host, int port);
    /**
     * @brief 析构监听服务器并释放资源。
     */
    ~TcpServer();

    /**
     * @brief 设置新连接建立后的回调。
     * @param cb 回调函数。
     */
    void setNewConnectionCallback(NewConnectionCallback cb);

    /**
     * @brief 启动监听并注册到 EventLoop。
     */
    void start();

private:
    /**
     * @brief 处理 accept 事件。
     */
    void onAccept();

    /// @brief 事件循环指针。
    EventLoop*  loop_;
    /// @brief 监听 socket fd。
    int         listen_fd_{-1};
    /// @brief 监听地址。
    std::string host_;
    /// @brief 监听端口。
    int         port_;

    /// @brief 新连接回调。
    NewConnectionCallback new_conn_cb_;
    /// @brief 当前活跃连接表（fd -> TcpConnection）。
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;
};

} // namespace cloudvault
