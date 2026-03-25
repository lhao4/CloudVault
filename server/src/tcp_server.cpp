// =============================================================
// server/src/tcp_server.cpp
// TcpServer 实现：创建监听 socket，accept 新连接
// =============================================================

#include "server/tcp_server.h"
#include "server/event_loop.h"
#include "server/tcp_connection.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace cloudvault {

TcpServer::TcpServer(EventLoop* loop, const std::string& host, int port)
    : loop_(loop), host_(host), port_(port)
{
    // ── 创建监听 socket ────────────────────────────────────
    // SOCK_STREAM   ：TCP 流式套接字
    // SOCK_NONBLOCK ：设置非阻塞，accept/read/write 不会阻塞
    // SOCK_CLOEXEC  ：fork 时自动关闭，防止 fd 泄漏
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (listen_fd_ < 0) {
        throw std::runtime_error(
            std::string("socket() failed: ") + strerror(errno));
    }

    // SO_REUSEADDR：允许服务器重启后立即绑定同一端口
    // （否则可能因 TIME_WAIT 状态导致 bind 失败）
    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // ── 绑定地址 ──────────────────────────────────────────
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<uint16_t>(port_));

    if (host_.empty() || host_ == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
            ::close(listen_fd_);
            throw std::runtime_error("inet_pton failed for host: " + host_);
        }
    }

    if (::bind(listen_fd_,
               reinterpret_cast<sockaddr*>(&addr),
               sizeof(addr)) < 0) {
        ::close(listen_fd_);
        throw std::runtime_error(
            std::string("bind() failed: ") + strerror(errno));
    }

    // ── 开始监听 ──────────────────────────────────────────
    // SOMAXCONN：系统允许的最大 backlog（通常 128）
    if (::listen(listen_fd_, SOMAXCONN) < 0) {
        ::close(listen_fd_);
        throw std::runtime_error(
            std::string("listen() failed: ") + strerror(errno));
    }

    spdlog::info("TcpServer listening on {}:{}", host_.empty() ? "0.0.0.0" : host_, port_);
}

TcpServer::~TcpServer() {
    if (listen_fd_ >= 0) ::close(listen_fd_);
}

void TcpServer::setNewConnectionCallback(NewConnectionCallback cb) {
    new_conn_cb_ = std::move(cb);
}

void TcpServer::start() {
    // 向 EventLoop 注册监听 fd，有新连接时调用 onAccept()
    loop_->addFd(listen_fd_, EPOLLIN | EPOLLET,
                 [this](uint32_t) { onAccept(); });
}

void TcpServer::onAccept() {
    // 边缘触发（ET）模式：一次 EPOLLIN 可能代表多个连接，需循环 accept
    while (true) {
        sockaddr_in peer_addr{};
        socklen_t   peer_len = sizeof(peer_addr);

        // accept4：比 accept + fcntl 更原子
        // SOCK_NONBLOCK | SOCK_CLOEXEC 直接在新 fd 上设置标志
        int conn_fd = ::accept4(listen_fd_,
                                reinterpret_cast<sockaddr*>(&peer_addr),
                                &peer_len,
                                SOCK_NONBLOCK | SOCK_CLOEXEC);

        if (conn_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // 没有更多待 accept 的连接
            }
            if (errno == EINTR) {
                continue;  // 被信号打断，重试
            }
            spdlog::warn("accept4 failed: {}", strerror(errno));
            break;
        }

        // 格式化对端地址字符串 "IP:PORT"
        char ip_buf[INET_ADDRSTRLEN] = {};
        ::inet_ntop(AF_INET, &peer_addr.sin_addr, ip_buf, sizeof(ip_buf));
        std::string peer_str =
            std::string(ip_buf) + ":" + std::to_string(ntohs(peer_addr.sin_port));

        spdlog::info("New connection from {} (fd={})", peer_str, conn_fd);

        // 创建 TcpConnection 并注册到 EventLoop
        auto conn = std::make_shared<TcpConnection>(loop_, conn_fd, peer_str);
        connections_[conn_fd] = conn;

        // TcpConnection 关闭时从 connections_ 移除
        conn->setCloseCallback([this](std::shared_ptr<TcpConnection> c) {
            connections_.erase(c->fd());
        });

        // 注册可读事件（EPOLLET 边缘触发）
        loop_->addFd(conn_fd,
                     EPOLLIN | EPOLLET,
                     [conn](uint32_t ev) {
                         if (ev & EPOLLIN)  conn->onReadable();
                         if (ev & EPOLLOUT) conn->onWritable();
                     });

        if (new_conn_cb_) {
            new_conn_cb_(conn);
        }
    }
}

} // namespace cloudvault
