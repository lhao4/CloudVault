// =============================================================
// server/include/server/tcp_connection.h
// 单个 TCP 连接的状态管理
//
// 职责：
//   - 接收缓冲区 + PDUParser（处理粘包/半包）
//   - 发送队列（mutex 保护，工作线程安全入队）
//   - 通过 EventLoop::runInLoop 将写操作调度回 IO 线程
// =============================================================

#pragma once

#include "common/protocol.h"
#include "common/protocol_codec.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace cloudvault {

class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using MessageCallback = std::function<void(
        std::shared_ptr<TcpConnection>,
        const PDUHeader&,
        std::vector<uint8_t>)>;
    using SendInterceptor =
        std::function<void(const std::vector<uint8_t>&)>;
    using CloseCallback =
        std::function<void(std::shared_ptr<TcpConnection>)>;

    TcpConnection(EventLoop* loop, int fd, std::string peer_addr);
    ~TcpConnection();

    int                fd()       const { return fd_; }
    const std::string& peerAddr() const { return peer_addr_; }

    // 线程安全：可从工作线程调用
    void send(std::vector<uint8_t> data);
    void close();

    void setMessageCallback(MessageCallback cb) { msg_cb_ = std::move(cb); }
    void setSendInterceptor(SendInterceptor cb) { send_interceptor_ = std::move(cb); }
    void setCloseCallback  (CloseCallback   cb) { close_cbs_.push_back(std::move(cb)); }

    // 由 EventLoop 回调，在 IO 线程执行
    void onReadable();
    void onWritable();

private:
    void doClose();
    void flushSendBuf();

    EventLoop*  loop_;
    int         fd_;
    std::string peer_addr_;
    std::atomic<bool> closed_{false};

    PDUParser parser_;

    std::mutex           send_mutex_;
    std::vector<uint8_t> send_buf_;

    MessageCallback msg_cb_;
    SendInterceptor send_interceptor_;
    std::vector<CloseCallback> close_cbs_;
};

} // namespace cloudvault
