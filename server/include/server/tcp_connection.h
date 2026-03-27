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

/**
 * @brief 单个 TCP 连接对象。
 *
 * 负责连接级收发缓存、PDU 拆包与发送队列，
 * 并通过 EventLoop 回调实现 IO 线程内读写。
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    /**
     * @brief 收到完整消息后的回调签名。
     */
    using MessageCallback = std::function<void(
        std::shared_ptr<TcpConnection>,
        const PDUHeader&,
        std::vector<uint8_t>)>;
    /**
     * @brief 发送拦截器（测试场景使用）。
     */
    using SendInterceptor =
        std::function<void(const std::vector<uint8_t>&)>;
    /**
     * @brief 连接关闭回调签名。
     */
    using CloseCallback =
        std::function<void(std::shared_ptr<TcpConnection>)>;

    /**
     * @brief 构造连接对象。
     * @param loop 事件循环指针。
     * @param fd 已接受连接 fd。
     * @param peer_addr 对端地址文本。
     */
    TcpConnection(EventLoop* loop, int fd, std::string peer_addr);
    /**
     * @brief 析构连接对象。
     */
    ~TcpConnection();

    /**
     * @brief 获取连接 fd。
     * @return 文件描述符。
     */
    int                fd()       const { return fd_; }
    /**
     * @brief 获取对端地址。
     * @return 对端地址字符串。
     */
    const std::string& peerAddr() const { return peer_addr_; }

    /**
     * @brief 线程安全发送数据。
     * @param data 已编码 PDU 字节。
     */
    void send(std::vector<uint8_t> data);
    /**
     * @brief 请求关闭连接。
     */
    void close();

    /**
     * @brief 设置消息回调。
     * @param cb 回调函数。
     */
    void setMessageCallback(MessageCallback cb) { msg_cb_ = std::move(cb); }
    /**
     * @brief 设置发送拦截器。
     * @param cb 拦截器函数。
     */
    void setSendInterceptor(SendInterceptor cb) { send_interceptor_ = std::move(cb); }
    /**
     * @brief 添加关闭回调。
     * @param cb 回调函数。
     */
    void setCloseCallback  (CloseCallback   cb) { close_cbs_.push_back(std::move(cb)); }

    /**
     * @brief IO 线程读事件回调。
     */
    void onReadable();
    /**
     * @brief IO 线程写事件回调。
     */
    void onWritable();

private:
    /**
     * @brief 执行底层关闭逻辑。
     */
    void doClose();
    /**
     * @brief 刷新发送缓冲区到 socket。
     */
    void flushSendBuf();

    /// @brief 事件循环指针。
    EventLoop*  loop_;
    /// @brief 连接 fd。
    int         fd_;
    /// @brief 对端地址。
    std::string peer_addr_;
    /// @brief 连接是否已关闭。
    std::atomic<bool> closed_{false};

    /// @brief PDU 解析器，处理粘包/半包。
    PDUParser parser_;

    /// @brief 发送缓存互斥锁。
    std::mutex           send_mutex_;
    /// @brief 待发送字节缓存。
    std::vector<uint8_t> send_buf_;

    /// @brief 完整消息回调。
    MessageCallback msg_cb_;
    /// @brief 发送拦截器（可为空）。
    SendInterceptor send_interceptor_;
    /// @brief 关闭回调列表。
    std::vector<CloseCallback> close_cbs_;
};

} // namespace cloudvault
