// =============================================================
// server/include/server/handler/chat_handler.h
// 私聊业务逻辑
// =============================================================

#pragma once

#include "common/protocol.h"
#include "server/db/chat_repository.h"
#include "server/db/friend_repository.h"
#include "server/db/user_repository.h"
#include "server/session_manager.h"

#include <memory>
#include <vector>

namespace cloudvault {

class TcpConnection;

class ChatHandler {
public:
    ChatHandler(Database& db, SessionManager& sessions);

    void handleChat(std::shared_ptr<TcpConnection> conn,
                    const PDUHeader& hdr,
                    const std::vector<uint8_t>& body);

    void handleGetHistory(std::shared_ptr<TcpConnection> conn,
                          const PDUHeader& hdr,
                          const std::vector<uint8_t>& body);

private:
    UserRepository   users_;
    FriendRepository friends_;
    ChatRepository   messages_;
    SessionManager&  sessions_;
};

} // namespace cloudvault
