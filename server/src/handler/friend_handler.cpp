// =============================================================
// server/src/handler/friend_handler.cpp
// 好友系统业务逻辑
// =============================================================

#include "server/handler/friend_handler.h"

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
constexpr uint8_t kStatusOffline = 3;
constexpr uint8_t kStatusAlreadyFriends = 4;
constexpr uint8_t kStatusInternal = 5;
constexpr uint8_t kStatusNotFriends = 6;
constexpr uint8_t kStatusUnauthorized = 7;

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

std::vector<uint8_t> buildStatusMessage(MessageType type, uint8_t status,
                                        const std::string& message) {
    return PDUBuilder(type)
        .writeUInt8(status)
        .writeString(message)
        .build();
}

std::vector<uint8_t> buildFriendRequestPush(const std::string& from_user) {
    return PDUBuilder(MessageType::FRIEND_REQUEST_PUSH)
        .writeString(from_user)
        .build();
}

std::vector<uint8_t> buildFriendAddedPush(const std::string& username) {
    return PDUBuilder(MessageType::FRIEND_ADDED_PUSH)
        .writeString(username)
        .build();
}

std::vector<uint8_t> buildFriendDeletedPush(const std::string& username) {
    return PDUBuilder(MessageType::FRIEND_DELETED_PUSH)
        .writeString(username)
        .build();
}

} // namespace

FriendHandler::FriendHandler(Database& db, SessionManager& sessions)
    : users_(db), friends_(db), sessions_(sessions) {}

bool FriendHandler::addPendingRequest(int target_user_id, int requester_user_id) {
    std::lock_guard lock(pending_mutex_);
    return pending_requests_[target_user_id].insert(requester_user_id).second;
}

bool FriendHandler::consumePendingRequest(int target_user_id, int requester_user_id) {
    std::lock_guard lock(pending_mutex_);
    auto it = pending_requests_.find(target_user_id);
    if (it == pending_requests_.end()) {
        return false;
    }

    const auto erased = it->second.erase(requester_user_id);
    if (it->second.empty()) {
        pending_requests_.erase(it);
    }
    return erased > 0;
}

void FriendHandler::handleFindUser(std::shared_ptr<TcpConnection> conn,
                                   const PDUHeader&,
                                   const std::vector<uint8_t>& body) {
    size_t offset = 0;
    std::string target;
    if (!readString(body, offset, target)) {
        conn->send(buildStatusMessage(MessageType::FIND_USER_RESPONSE,
                                      kStatusInvalid, "请求格式错误"));
        return;
    }

    const auto user = users_.findByName(target);
    if (!user) {
        conn->send(buildStatusMessage(MessageType::FIND_USER_RESPONSE,
                                      kStatusNotFound, "用户不存在"));
        return;
    }

    auto response = PDUBuilder(MessageType::FIND_USER_RESPONSE)
        .writeUInt8(kStatusOk)
        .writeString(user->username)
        .writeUInt8(sessions_.isOnline(user->username) ? 1 : 0)
        .build();
    conn->send(std::move(response));
}

void FriendHandler::handleAddFriend(std::shared_ptr<TcpConnection> conn,
                                    const PDUHeader&,
                                    const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatusMessage(MessageType::ADD_FRIEND_RESPONSE,
                                      kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string target_name;
    if (!readString(body, offset, target_name)) {
        conn->send(buildStatusMessage(MessageType::ADD_FRIEND_RESPONSE,
                                      kStatusInvalid, "请求格式错误"));
        return;
    }

    if (target_name == current->username) {
        conn->send(buildStatusMessage(MessageType::ADD_FRIEND_RESPONSE,
                                      kStatusInvalid, "不能添加自己为好友"));
        return;
    }

    const auto target = users_.findByName(target_name);
    if (!target) {
        conn->send(buildStatusMessage(MessageType::ADD_FRIEND_RESPONSE,
                                      kStatusNotFound, "用户不存在"));
        return;
    }

    if (friends_.areFriends(current->user_id, target->user_id)) {
        conn->send(buildStatusMessage(MessageType::ADD_FRIEND_RESPONSE,
                                      kStatusAlreadyFriends, "你们已经是好友"));
        return;
    }

    if (!sessions_.isOnline(target->username)) {
        conn->send(buildStatusMessage(MessageType::ADD_FRIEND_RESPONSE,
                                      kStatusOffline, "对方当前不在线"));
        return;
    }

    if (!addPendingRequest(target->user_id, current->user_id)) {
        conn->send(buildStatusMessage(MessageType::ADD_FRIEND_RESPONSE,
                                      kStatusInvalid, "好友请求已发送，请等待对方处理"));
        return;
    }

    sessions_.sendTo(target->username, buildFriendRequestPush(current->username));
    conn->send(buildStatusMessage(MessageType::ADD_FRIEND_RESPONSE,
                                  kStatusOk, "好友请求已发送"));
}

void FriendHandler::handleAgreeFriend(std::shared_ptr<TcpConnection> conn,
                                      const PDUHeader&,
                                      const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatusMessage(MessageType::AGREE_FRIEND_RESPONSE,
                                      kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string requester_name;
    if (!readString(body, offset, requester_name)) {
        conn->send(buildStatusMessage(MessageType::AGREE_FRIEND_RESPONSE,
                                      kStatusInvalid, "请求格式错误"));
        return;
    }

    const auto requester = users_.findByName(requester_name);
    if (!requester) {
        conn->send(buildStatusMessage(MessageType::AGREE_FRIEND_RESPONSE,
                                      kStatusNotFound, "请求方不存在"));
        return;
    }

    if (requester->user_id == current->user_id) {
        conn->send(buildStatusMessage(MessageType::AGREE_FRIEND_RESPONSE,
                                      kStatusInvalid, "不能添加自己为好友"));
        return;
    }

    if (friends_.areFriends(current->user_id, requester->user_id)) {
        conn->send(buildStatusMessage(MessageType::AGREE_FRIEND_RESPONSE,
                                      kStatusAlreadyFriends, "你们已经是好友"));
        return;
    }

    if (!consumePendingRequest(current->user_id, requester->user_id)) {
        conn->send(buildStatusMessage(MessageType::AGREE_FRIEND_RESPONSE,
                                      kStatusInvalid, "没有待处理的好友请求"));
        return;
    }

    if (!friends_.addFriendPair(current->user_id, requester->user_id)) {
        conn->send(buildStatusMessage(MessageType::AGREE_FRIEND_RESPONSE,
                                      kStatusInternal, "添加好友失败"));
        return;
    }

    conn->send(buildStatusMessage(MessageType::AGREE_FRIEND_RESPONSE,
                                  kStatusOk, "已同意好友请求"));
    if (sessions_.isOnline(requester->username)) {
        sessions_.sendTo(requester->username, buildFriendAddedPush(current->username));
    }
}

void FriendHandler::handleFlushFriends(std::shared_ptr<TcpConnection> conn,
                                       const PDUHeader&,
                                       const std::vector<uint8_t>&) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        spdlog::warn("FLUSH_FRIENDS: unauthorized request from {}", conn->peerAddr());
        conn->send(buildStatusMessage(MessageType::FLUSH_FRIENDS_RESPONSE,
                                      kStatusUnauthorized, "未登录"));
        return;
    }

    spdlog::info("FLUSH_FRIENDS: user={} uid={}", current->username, current->user_id);
    const auto friends = friends_.listFriends(current->user_id);
    spdlog::info("FLUSH_FRIENDS: returning {} friends", friends.size());
    auto response = PDUBuilder(MessageType::FLUSH_FRIENDS_RESPONSE)
        .writeUInt8(kStatusOk)
        .writeUInt16(static_cast<uint16_t>(friends.size()));
    for (const auto& item : friends) {
        response.writeString(item.username)
            .writeUInt8(sessions_.isOnline(item.username) ? 1 : 0);
    }
    conn->send(response.build());
}

void FriendHandler::handleDeleteFriend(std::shared_ptr<TcpConnection> conn,
                                       const PDUHeader&,
                                       const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatusMessage(MessageType::DELETE_FRIEND_RESPONSE,
                                      kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string target_name;
    if (!readString(body, offset, target_name)) {
        conn->send(buildStatusMessage(MessageType::DELETE_FRIEND_RESPONSE,
                                      kStatusInvalid, "请求格式错误"));
        return;
    }

    const auto target = users_.findByName(target_name);
    if (!target) {
        conn->send(buildStatusMessage(MessageType::DELETE_FRIEND_RESPONSE,
                                      kStatusNotFound, "用户不存在"));
        return;
    }

    if (!friends_.areFriends(current->user_id, target->user_id)) {
        conn->send(buildStatusMessage(MessageType::DELETE_FRIEND_RESPONSE,
                                      kStatusNotFriends, "你们还不是好友"));
        return;
    }

    if (!friends_.removeFriendPair(current->user_id, target->user_id)) {
        conn->send(buildStatusMessage(MessageType::DELETE_FRIEND_RESPONSE,
                                      kStatusInternal, "删除好友失败"));
        return;
    }

    conn->send(buildStatusMessage(MessageType::DELETE_FRIEND_RESPONSE,
                                  kStatusOk, "好友已删除"));
    if (sessions_.isOnline(target->username)) {
        sessions_.sendTo(target->username, buildFriendDeletedPush(current->username));
    }
}

} // namespace cloudvault
