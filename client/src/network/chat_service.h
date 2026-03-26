// =============================================================
// client/src/network/chat_service.h
// 私聊消息协议封装
// =============================================================

#pragma once

#include "network/response_router.h"
#include "network/tcp_client.h"

#include <QList>
#include <QObject>
#include <QString>

namespace cloudvault {

struct ChatMessage {
    QString from;
    QString to;
    QString content;
    QString timestamp;
};

class ChatService : public QObject {
    Q_OBJECT

public:
    explicit ChatService(TcpClient& client,
                         ResponseRouter& router,
                         QObject* parent = nullptr);

    void sendMessage(const QString& target, const QString& content);
    void loadHistory(const QString& peer);

signals:
    void messageReceived(const cloudvault::ChatMessage& message);
    void historyLoaded(const QString& peer, const QList<cloudvault::ChatMessage>& messages);
    void historyLoadFailed(const QString& peer, const QString& reason);

private:
    void onChatPush(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onHistoryResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);

    TcpClient& client_;
    ResponseRouter& router_;
    QString pending_history_peer_;
};

} // namespace cloudvault

Q_DECLARE_METATYPE(cloudvault::ChatMessage)
