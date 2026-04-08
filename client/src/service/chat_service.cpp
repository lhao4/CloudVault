// =============================================================
// client/src/service/chat_service.cpp
// 私聊消息协议封装
// =============================================================

#include "chat_service.h"

#include "common/protocol_codec.h"

#include <cstring>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

namespace cloudvault {

namespace {

constexpr uint8_t kStatusOk = 0;

bool readString(const std::vector<uint8_t>& body, size_t& offset, std::string& out) {
    if (offset + 2 > body.size()) return false;

    uint16_t len = 0;
    std::memcpy(&len, body.data() + offset, sizeof(len));
    len = ntohs(len);
    offset += 2;

    if (offset + len > body.size()) return false;
    out.assign(reinterpret_cast<const char*>(body.data() + offset), len);
    offset += len;
    return true;
}

QString readMessage(const std::vector<uint8_t>& body, size_t offset) {
    std::string message;
    return readString(body, offset, message)
        ? QString::fromStdString(message)
        : QStringLiteral("服务器返回格式错误");
}

} // namespace

ChatService::ChatService(TcpClient& client,
                         ResponseRouter& router,
                         QObject* parent)
    : QObject(parent), client_(client), router_(router) {
    qRegisterMetaType<cloudvault::ChatMessage>("cloudvault::ChatMessage");
    qRegisterMetaType<QList<cloudvault::ChatMessage>>("QList<cloudvault::ChatMessage>");

    router_.registerHandler(
        MessageType::CHAT,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onChatPush(hdr, body);
        });
    router_.registerHandler(
        MessageType::GET_HISTORY,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onHistoryResponse(hdr, body);
        });
}

void ChatService::sendMessage(const QString& target, const QString& content) {
    const auto trimmed_target = target.trimmed();
    auto pdu = PDUBuilder(MessageType::CHAT)
        .writeString(trimmed_target.toStdString())
        .writeString(content.toStdString())
        .build();
    client_.send(std::move(pdu));
}

void ChatService::loadHistory(const QString& peer) {
    pending_history_peer_ = peer.trimmed();
    auto pdu = PDUBuilder(MessageType::GET_HISTORY)
        .writeString(pending_history_peer_.toStdString())
        .build();
    client_.send(std::move(pdu));
}

void ChatService::loadGroupHistory(int groupId) {
    Q_UNUSED(groupId);
}

void ChatService::onChatPush(const PDUHeader&, const std::vector<uint8_t>& body) {
    size_t offset = 0;
    std::string from;
    std::string to;
    std::string content;
    std::string timestamp;
    if (!readString(body, offset, from) ||
        !readString(body, offset, to) ||
        !readString(body, offset, content) ||
        !readString(body, offset, timestamp)) {
        return;
    }

    emit messageReceived(ChatMessage{
        0,
        QString::fromStdString(from),
        QString::fromStdString(to),
        QString::fromStdString(content),
        QString::fromStdString(timestamp),
    });
}

void ChatService::onHistoryResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit historyLoadFailed(pending_history_peer_, QStringLiteral("服务器返回空响应"));
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    std::string peer;
    if (!readString(body, offset, peer)) {
        emit historyLoadFailed(pending_history_peer_, QStringLiteral("历史消息格式错误"));
        return;
    }

    const QString peer_name = QString::fromStdString(peer);
    if (status != kStatusOk) {
        emit historyLoadFailed(peer_name.isEmpty() ? pending_history_peer_ : peer_name,
                               readMessage(body, offset));
        return;
    }

    if (offset + 2 > body.size()) {
        emit historyLoadFailed(peer_name, QStringLiteral("历史消息格式错误"));
        return;
    }

    uint16_t count = 0;
    std::memcpy(&count, body.data() + offset, sizeof(count));
    count = ntohs(count);
    offset += 2;

    QList<ChatMessage> messages;
    for (uint16_t i = 0; i < count; ++i) {
        std::string from;
        std::string to;
        std::string content;
        std::string timestamp;
        if (!readString(body, offset, from) ||
            !readString(body, offset, to) ||
            !readString(body, offset, content) ||
            !readString(body, offset, timestamp)) {
            emit historyLoadFailed(peer_name, QStringLiteral("历史消息格式错误"));
            return;
        }
        messages.append(ChatMessage{
            0,
            QString::fromStdString(from),
            QString::fromStdString(to),
            QString::fromStdString(content),
            QString::fromStdString(timestamp),
        });
    }

    emit historyLoaded(peer_name, messages);
}

} // namespace cloudvault
