// =============================================================
// server/include/server/handler/friend_handler.h
// 好友系统业务逻辑
// =============================================================

#pragma once

#include "common/protocol.h"
#include "server/db/friend_repository.h"
#include "server/db/user_repository.h"
#include "server/session_manager.h"

#include <mutex>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cloudvault {

class TcpConnection;

class FriendHandler {
public:
    FriendHandler(Database& db, SessionManager& sessions);

    void handleFindUser(std::shared_ptr<TcpConnection> conn,
                        const PDUHeader& hdr,
                        const std::vector<uint8_t>& body);
    void handleAddFriend(std::shared_ptr<TcpConnection> conn,
                         const PDUHeader& hdr,
                         const std::vector<uint8_t>& body);
    void handleAgreeFriend(std::shared_ptr<TcpConnection> conn,
                           const PDUHeader& hdr,
                           const std::vector<uint8_t>& body);
    void handleFlushFriends(std::shared_ptr<TcpConnection> conn,
                            const PDUHeader& hdr,
                            const std::vector<uint8_t>& body);
    void handleDeleteFriend(std::shared_ptr<TcpConnection> conn,
                            const PDUHeader& hdr,
                            const std::vector<uint8_t>& body);

private:
    bool addPendingRequest(int target_user_id, int requester_user_id);
    bool consumePendingRequest(int target_user_id, int requester_user_id);

    UserRepository   users_;
    FriendRepository friends_;
    SessionManager&  sessions_;
    std::mutex pending_mutex_;
    std::unordered_map<int, std::unordered_set<int>> pending_requests_;
};

} // namespace cloudvault
