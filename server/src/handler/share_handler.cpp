// =============================================================
// server/src/handler/share_handler.cpp
// 第十三章：文件分享业务逻辑
// =============================================================

#include "server/handler/share_handler.h"

#include "common/protocol_codec.h"
#include "server/tcp_connection.h"

#include <arpa/inet.h>
#include <cstring>

namespace cloudvault {

namespace {

constexpr uint8_t kStatusOk = 0;
constexpr uint8_t kStatusInvalid = 1;
constexpr uint8_t kStatusNotFound = 2;
constexpr uint8_t kStatusConflict = 3;
constexpr uint8_t kStatusUnauthorized = 4;

bool readString(const std::vector<uint8_t>& body, size_t& offset, std::string& out) {
    if (offset + 2 > body.size()) {
        return false;
    }

    uint16_t len = 0;
    std::memcpy(&len, body.data() + offset, sizeof(len));
    len = ntohs(len);
    offset += 2;
    if (offset + len > body.size()) {
        return false;
    }

    out.assign(reinterpret_cast<const char*>(body.data() + offset), len);
    offset += len;
    return true;
}

bool readUInt32(const std::vector<uint8_t>& body, size_t& offset, uint32_t& out) {
    if (offset + 4 > body.size()) {
        return false;
    }
    std::memcpy(&out, body.data() + offset, sizeof(out));
    out = ntohl(out);
    offset += 4;
    return true;
}

bool readFixedString32(const std::vector<uint8_t>& body, size_t& offset, std::string& out) {
    constexpr size_t kFixedLen = 32;
    if (offset + kFixedLen > body.size()) {
        return false;
    }

    const char* data = reinterpret_cast<const char*>(body.data() + offset);
    size_t actual_len = 0;
    while (actual_len < kFixedLen && data[actual_len] != '\0') {
        ++actual_len;
    }
    out.assign(data, actual_len);
    offset += kFixedLen;
    return true;
}

std::vector<uint8_t> buildStatus(MessageType type,
                                 uint8_t status,
                                 const std::string& message) {
    return PDUBuilder(type)
        .writeUInt8(status)
        .writeString(message)
        .build();
}

std::vector<uint8_t> buildShareNotify(const std::string& from_user,
                                      const std::string& file_path) {
    return PDUBuilder(MessageType::SHARE_NOTIFY)
        .writeFixedString(from_user, 32)
        .writeString(file_path)
        .build();
}

} // namespace

ShareHandler::ShareHandler(Database& db,
                           SessionManager& sessions,
                           FileStorage& storage)
    : users_(db), friends_(db), sessions_(sessions), storage_(storage) {}

void ShareHandler::handleShareRequest(std::shared_ptr<TcpConnection> conn,
                                      const PDUHeader&,
                                      const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::SHARE_REQUEST, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    uint32_t count = 0;
    std::string file_path;
    if (!readUInt32(body, offset, count)) {
        conn->send(buildStatus(MessageType::SHARE_REQUEST, kStatusInvalid, "请求格式错误"));
        return;
    }

    std::vector<std::string> targets;
    targets.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        std::string username;
        if (!readFixedString32(body, offset, username) || username.empty()) {
            conn->send(buildStatus(MessageType::SHARE_REQUEST, kStatusInvalid, "请求格式错误"));
            return;
        }
        targets.push_back(username);
    }
    if (!readString(body, offset, file_path)) {
        conn->send(buildStatus(MessageType::SHARE_REQUEST, kStatusInvalid, "请求格式错误"));
        return;
    }
    if (targets.empty()) {
        conn->send(buildStatus(MessageType::SHARE_REQUEST, kStatusInvalid, "请选择至少一个好友"));
        return;
    }

    try {
        const auto shared = storage_.inspectPath(current->username, file_path);
        if (shared.is_dir) {
            conn->send(buildStatus(MessageType::SHARE_REQUEST, kStatusInvalid, "当前只支持分享普通文件"));
            return;
        }
    } catch (const std::runtime_error& error) {
        conn->send(buildStatus(MessageType::SHARE_REQUEST, kStatusNotFound, error.what()));
        return;
    }

    size_t delivered = 0;
    for (const auto& target_name : targets) {
        if (target_name == current->username) {
            continue;
        }

        const auto target_user = users_.findByName(target_name);
        if (!target_user) {
            continue;
        }
        if (!friends_.areFriends(current->user_id, target_user->user_id)) {
            continue;
        }

        auto target_conn = sessions_.getConnection(target_name);
        if (!target_conn) {
            continue;
        }

        target_conn->send(buildShareNotify(current->username, file_path));
        {
            std::lock_guard lock(pending_mutex_);
            pending_shares_[target_name].insert(pendingKey(current->username, file_path));
        }
        ++delivered;
    }

    if (delivered == 0) {
        conn->send(buildStatus(MessageType::SHARE_REQUEST,
                               kStatusConflict,
                               "没有可接收分享的在线好友"));
        return;
    }

    conn->send(buildStatus(MessageType::SHARE_REQUEST,
                           kStatusOk,
                           "已向 " + std::to_string(delivered) + " 位好友发送分享请求"));
}

void ShareHandler::handleShareAgree(std::shared_ptr<TcpConnection> conn,
                                    const PDUHeader&,
                                    const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::SHARE_AGREE_RESPOND, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string sender_name;
    std::string source_path;
    if (!readFixedString32(body, offset, sender_name) ||
        !readString(body, offset, source_path) ||
        sender_name.empty()) {
        conn->send(buildStatus(MessageType::SHARE_AGREE_RESPOND, kStatusInvalid, "请求格式错误"));
        return;
    }

    const auto sender_user = users_.findByName(sender_name);
    if (!sender_user) {
        conn->send(buildStatus(MessageType::SHARE_AGREE_RESPOND, kStatusNotFound, "分享来源用户不存在"));
        return;
    }
    if (!friends_.areFriends(current->user_id, sender_user->user_id)) {
        conn->send(buildStatus(MessageType::SHARE_AGREE_RESPOND, kStatusUnauthorized, "你们还不是好友"));
        return;
    }

    {
        std::lock_guard lock(pending_mutex_);
        auto receiver_it = pending_shares_.find(current->username);
        if (receiver_it == pending_shares_.end() ||
            receiver_it->second.count(pendingKey(sender_name, source_path)) == 0) {
            conn->send(buildStatus(MessageType::SHARE_AGREE_RESPOND,
                                   kStatusInvalid,
                                   "没有待处理的分享请求"));
            return;
        }
    }

    try {
        const std::string copied_path =
            storage_.copyFileToUser(sender_name, source_path, current->username);
        {
            std::lock_guard lock(pending_mutex_);
            auto receiver_it = pending_shares_.find(current->username);
            if (receiver_it != pending_shares_.end()) {
                receiver_it->second.erase(pendingKey(sender_name, source_path));
                if (receiver_it->second.empty()) {
                    pending_shares_.erase(receiver_it);
                }
            }
        }
        conn->send(buildStatus(MessageType::SHARE_AGREE_RESPOND,
                               kStatusOk,
                               "已接收分享，文件已复制到 " + copied_path));
    } catch (const std::runtime_error& error) {
        conn->send(buildStatus(MessageType::SHARE_AGREE_RESPOND,
                               kStatusConflict,
                               error.what()));
    }
}

void ShareHandler::handleConnectionClosed(std::shared_ptr<TcpConnection> conn) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        return;
    }

    std::lock_guard lock(pending_mutex_);
    pending_shares_.erase(current->username);
}

std::string ShareHandler::pendingKey(const std::string& sender, const std::string& file_path) {
    return sender + '\n' + file_path;
}

} // namespace cloudvault
