// =============================================================
// client/src/service/chat_service.h
// 私聊消息协议封装
// =============================================================

#pragma once

#include "network/response_router.h"
#include "network/tcp_client.h"

#include <QList>
#include <QObject>
#include <QString>

namespace cloudvault {

/**
 * @brief 聊天消息结构。
 */
struct ChatMessage {
    /// @brief 发送方用户名。
    QString from;
    /// @brief 接收方用户名。
    QString to;
    /// @brief 消息正文。
    QString content;
    /// @brief 消息时间戳文本。
    QString timestamp;
};

/**
 * @brief 私聊服务。
 *
 * 封装发送消息与拉取历史消息的协议交互，
 * 并将消息事件通过信号抛给 UI。
 */
class ChatService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造聊天服务。
     * @param client TCP 客户端引用。
     * @param router 响应路由器引用。
     * @param parent Qt 父对象。
     */
    explicit ChatService(TcpClient& client,
                         ResponseRouter& router,
                         QObject* parent = nullptr);

    /**
     * @brief 发送私聊消息。
     * @param target 目标用户名。
     * @param content 消息内容。
     */
    void sendMessage(const QString& target, const QString& content);
    /**
     * @brief 拉取与指定联系人的历史消息。
     * @param peer 联系人用户名。
     */
    void loadHistory(const QString& peer);

signals:
    /// @brief 收到实时消息推送。
    void messageReceived(const cloudvault::ChatMessage& message);
    /// @brief 历史消息加载成功。
    void historyLoaded(const QString& peer, const QList<cloudvault::ChatMessage>& messages);
    /// @brief 历史消息加载失败。
    void historyLoadFailed(const QString& peer, const QString& reason);

private:
    /**
     * @brief 处理聊天推送消息。
     * @param hdr 响应头。
     * @param body 响应体。
     */
    void onChatPush(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /**
     * @brief 处理历史消息响应。
     * @param hdr 响应头。
     * @param body 响应体。
     */
    void onHistoryResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);

    /// @brief TCP 客户端引用。
    TcpClient& client_;
    /// @brief 响应路由器引用。
    ResponseRouter& router_;
    /// @brief 当前挂起的历史请求对象，用于结果归因。
    QString pending_history_peer_;
};

} // namespace cloudvault

Q_DECLARE_METATYPE(cloudvault::ChatMessage)
