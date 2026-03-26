// =============================================================
// server/include/server/handler/share_handler.h
// 第十三章：文件分享业务逻辑
// =============================================================

#pragma once

#include "common/protocol.h"
#include "server/db/database.h"
#include "server/db/friend_repository.h"
#include "server/db/user_repository.h"
#include "server/file_storage.h"
#include "server/session_manager.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cloudvault {

class TcpConnection;

class ShareHandler {
public:
    ShareHandler(Database& db, SessionManager& sessions, FileStorage& storage);

    void handleShareRequest(std::shared_ptr<TcpConnection> conn,
                            const PDUHeader& hdr,
                            const std::vector<uint8_t>& body);

    void handleShareAgree(std::shared_ptr<TcpConnection> conn,
                          const PDUHeader& hdr,
                          const std::vector<uint8_t>& body);

    void handleConnectionClosed(std::shared_ptr<TcpConnection> conn);

private:
    static std::string pendingKey(const std::string& sender, const std::string& file_path);

    UserRepository users_;
    FriendRepository friends_;
    SessionManager& sessions_;
    FileStorage& storage_;

    std::mutex pending_mutex_;
    std::unordered_map<std::string, std::unordered_set<std::string>> pending_shares_;
};

} // namespace cloudvault
