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

class TcpServer {
public:
    using NewConnectionCallback =
        std::function<void(std::shared_ptr<TcpConnection>)>;

    TcpServer(EventLoop* loop, const std::string& host, int port);
    ~TcpServer();

    void setNewConnectionCallback(NewConnectionCallback cb);

    // 向 EventLoop 注册监听 fd，开始接受连接
    void start();

private:
    void onAccept();

    EventLoop*  loop_;
    int         listen_fd_{-1};
    std::string host_;
    int         port_;

    NewConnectionCallback new_conn_cb_;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;
};

} // namespace cloudvault
