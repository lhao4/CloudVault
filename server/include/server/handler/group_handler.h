// =============================================================
// server/include/server/handler/group_handler.h
// 群组管理与群聊处理器
// =============================================================

#pragma once

#include "common/protocol.h"
#include "server/db/chat_repository.h"
#include "server/db/group_repository.h"
#include "server/db/user_repository.h"
#include "server/session_manager.h"

#include <memory>
#include <vector>

namespace cloudvault {

class TcpConnection;
class Database;

class GroupHandler {
public:
    GroupHandler(Database& db, SessionManager& sessions);

    void handleCreateGroup(std::shared_ptr<TcpConnection> conn,
                           const PDUHeader& hdr,
                           const std::vector<uint8_t>& body);
    void handleJoinGroup(std::shared_ptr<TcpConnection> conn,
                         const PDUHeader& hdr,
                         const std::vector<uint8_t>& body);
    void handleLeaveGroup(std::shared_ptr<TcpConnection> conn,
                          const PDUHeader& hdr,
                          const std::vector<uint8_t>& body);
    void handleGetGroupList(std::shared_ptr<TcpConnection> conn,
                            const PDUHeader& hdr,
                            const std::vector<uint8_t>& body);
    void handleGroupChat(std::shared_ptr<TcpConnection> conn,
                         const PDUHeader& hdr,
                         const std::vector<uint8_t>& body);

private:
    UserRepository users_;
    GroupRepository groups_;
    ChatRepository messages_;
    SessionManager& sessions_;
};

} // namespace cloudvault
