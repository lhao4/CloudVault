// =============================================================
// server/src/tcp_connection.cpp
// 单个 TCP 连接的状态管理实现
// =============================================================

#include "server/tcp_connection.h"
#include "server/event_loop.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include <spdlog/spdlog.h>

namespace cloudvault {

TcpConnection::TcpConnection(EventLoop* loop, int fd, std::string peer_addr)
    : loop_(loop), fd_(fd), peer_addr_(std::move(peer_addr))
{}

TcpConnection::~TcpConnection() {
    if (fd_ >= 0) ::close(fd_);
}

// ── 线程安全发送 ──────────────────────────────────────────
// 可从任意线程调用（工作线程 / IO 线程均可）
void TcpConnection::send(std::vector<uint8_t> data) {
    if (closed_.load()) return;

    {
        std::lock_guard lock(send_mutex_);
        send_buf_.insert(send_buf_.end(), data.begin(), data.end());
    }

    // 将实际写操作调度回 IO 线程，避免多线程并发写 fd
    auto self = shared_from_this();
    loop_->runInLoop([self] {
        self->flushSendBuf();
    });
}

// ── 关闭连接（线程安全）──────────────────────────────────
void TcpConnection::close() {
    if (closed_.exchange(true)) return;  // 已关闭，幂等

    auto self = shared_from_this();
    loop_->runInLoop([self] {
        self->doClose();
    });
}

// ── IO 线程回调：可读 ─────────────────────────────────────
void TcpConnection::onReadable() {
    if (closed_.load()) return;

    // ET 模式：必须循环读直到 EAGAIN
    char buf[4096];
    while (true) {
        ssize_t n = ::recv(fd_, buf, sizeof(buf), 0);
        if (n > 0) {
            parser_.feed(buf, static_cast<size_t>(n));

            // 尝试解析完整 PDU，可能有多个（粘包）
            PDUHeader              hdr;
            std::vector<uint8_t>   body;
            while (parser_.tryParse(hdr, body)) {
                if (msg_cb_) {
                    msg_cb_(shared_from_this(), hdr, std::move(body));
                }
            }
        } else if (n == 0) {
            // 对端正常关闭
            spdlog::info("Connection closed by peer: {}", peer_addr_);
            doClose();
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // 已读完所有数据
            }
            if (errno == EINTR) {
                continue;  // 被信号打断，重试
            }
            spdlog::warn("recv error on {}: {}", peer_addr_, strerror(errno));
            doClose();
            return;
        }
    }
}

// ── IO 线程回调：可写 ─────────────────────────────────────
void TcpConnection::onWritable() {
    if (closed_.load()) return;
    flushSendBuf();
}

// ── 在 IO 线程中执行实际写操作 ────────────────────────────
void TcpConnection::flushSendBuf() {
    if (closed_.load()) return;

    std::vector<uint8_t> buf;
    {
        std::lock_guard lock(send_mutex_);
        if (send_buf_.empty()) return;
        buf.swap(send_buf_);
    }

    size_t offset = 0;
    while (offset < buf.size()) {
        ssize_t n = ::send(fd_,
                           buf.data() + offset,
                           buf.size() - offset,
                           MSG_NOSIGNAL);  // 不触发 SIGPIPE
        if (n > 0) {
            offset += static_cast<size_t>(n);
        } else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 发送缓冲区满：将剩余数据放回 send_buf_，注册 EPOLLOUT
                std::lock_guard lock(send_mutex_);
                send_buf_.insert(send_buf_.begin(),
                                 buf.begin() + static_cast<long>(offset),
                                 buf.end());
                loop_->modFd(fd_, EPOLLIN | EPOLLOUT | EPOLLET);
                return;
            }
            if (errno == EINTR) continue;

            spdlog::warn("send error on {}: {}", peer_addr_, strerror(errno));
            doClose();
            return;
        }
    }

    // 数据全部发送完毕，取消监听 EPOLLOUT（避免 busy loop）
    loop_->modFd(fd_, EPOLLIN | EPOLLET);
}

// ── 实际关闭（在 IO 线程执行）────────────────────────────
void TcpConnection::doClose() {
    if (!closed_.exchange(true)) {
        // 首次调用
    }
    loop_->removeFd(fd_);
    ::close(fd_);
    fd_ = -1;

    if (close_cb_) {
        close_cb_(shared_from_this());
    }
}

} // namespace cloudvault
