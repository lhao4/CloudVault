// =============================================================
// server/src/handler/chat_handler.cpp
// 私聊与历史查询
// =============================================================

#include "server/handler/chat_handler.h"

#include "common/protocol_codec.h"
#include "server/tcp_connection.h"

#include <arpa/inet.h>
#include <cstring>
#include <spdlog/spdlog.h>

namespace cloudvault {

namespace {

constexpr uint8_t kStatusOk = 0;
constexpr uint8_t kStatusInvalid = 1;
constexpr uint8_t kStatusNotFound = 2;
constexpr uint8_t kStatusNotFriends = 6;
constexpr uint8_t kStatusUnauthorized = 7;
constexpr size_t  kMaxMessageBytes = 4096;

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

std::vector<uint8_t> buildChatPush(const std::string& from,
                                   const std::string& to,
                                   const std::string& content,
                                   const std::string& created_at) {
    return PDUBuilder(MessageType::CHAT)
        .writeString(from)
        .writeString(to)
        .writeString(content)
        .writeString(created_at)
        .build();
}

std::vector<uint8_t> buildHistoryFailure(const std::string& peer,
                                         uint8_t status,
                                         const std::string& message) {
    return PDUBuilder(MessageType::GET_HISTORY)
        .writeUInt8(status)
        .writeString(peer)
        .writeString(message)
        .build();
}

} // namespace

ChatHandler::ChatHandler(Database& db, SessionManager& sessions)
    : users_(db), friends_(db), messages_(db), sessions_(sessions) {}

void ChatHandler::handleChat(std::shared_ptr<TcpConnection> conn,
                             const PDUHeader&,
                             const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        spdlog::warn("CHAT: unauthorized request from {}", conn->peerAddr());
        return;
    }

    size_t offset = 0;
    std::string target_name;
    std::string content;
    if (!readString(body, offset, target_name) || !readString(body, offset, content)) {
        spdlog::warn("CHAT: malformed body from {}", current->username);
        return;
    }

    if (target_name.empty() || content.empty() || content.size() > kMaxMessageBytes) {
        spdlog::warn("CHAT: invalid content from {}", current->username);
        return;
    }

    const auto target = users_.findByName(target_name);
    if (!target) {
        spdlog::warn("CHAT: target '{}' not found", target_name);
        return;
    }

    if (!friends_.areFriends(current->user_id, target->user_id)) {
        spdlog::warn("CHAT: '{}' attempted to message non-friend '{}'",
                     current->username, target_name);
        return;
    }

    const std::string created_at =
        messages_.storePrivateMessage(current->user_id, target->user_id, content);
    if (created_at.empty()) {
        spdlog::error("CHAT: failed to persist message {} -> {}", current->username, target_name);
        return;
    }

    auto push = buildChatPush(current->username, target->username, content, created_at);
    if (sessions_.isOnline(target->username)) {
        sessions_.sendTo(target->username, std::move(push));
        spdlog::info("CHAT delivered: {} -> {}", current->username, target->username);
        return;
    }

    if (!messages_.queueOfflineMessage(current->user_id,
                                       target->user_id,
                                       static_cast<uint32_t>(MessageType::CHAT),
                                       content,
                                       created_at)) {
        spdlog::error("CHAT: failed to queue offline message {} -> {}",
                      current->username, target->username);
        return;
    }

    spdlog::info("CHAT queued offline: {} -> {}", current->username, target->username);
}

void ChatHandler::handleGetHistory(std::shared_ptr<TcpConnection> conn,
                                   const PDUHeader&,
                                   const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildHistoryFailure("", kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string peer_name;
    if (!readString(body, offset, peer_name) || peer_name.empty()) {
        conn->send(buildHistoryFailure("", kStatusInvalid, "请求格式错误"));
        return;
    }

    const auto peer = users_.findByName(peer_name);
    if (!peer) {
        conn->send(buildHistoryFailure(peer_name, kStatusNotFound, "用户不存在"));
        return;
    }

    if (!friends_.areFriends(current->user_id, peer->user_id)) {
        conn->send(buildHistoryFailure(peer_name, kStatusNotFriends, "你们还不是好友"));
        return;
    }

    const auto history = messages_.fetchPrivateHistory(current->user_id, peer->user_id);
    auto response = PDUBuilder(MessageType::GET_HISTORY)
        .writeUInt8(kStatusOk)
        .writeString(peer_name)
        .writeUInt16(static_cast<uint16_t>(history.size()));
    for (const auto& item : history) {
        response.writeString(item.sender_username)
            .writeString(item.receiver_username)
            .writeString(item.content)
            .writeString(item.created_at);
    }
    conn->send(response.build());
}

} // namespace cloudvault
