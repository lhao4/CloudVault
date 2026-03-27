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

/**
 * @brief 私聊处理器。
 *
 * 负责私聊消息发送与历史记录查询。
 */
class ChatHandler {
public:
    /**
     * @brief 构造聊天处理器。
     * @param db 数据库连接池引用。
     * @param sessions 会话管理器引用。
     */
    ChatHandler(Database& db, SessionManager& sessions);

    /**
     * @brief 处理私聊发送请求。
     * @param conn 请求连接。
     * @param hdr 协议头。
     * @param body 协议体。
     */
    void handleChat(std::shared_ptr<TcpConnection> conn,
                    const PDUHeader& hdr,
                    const std::vector<uint8_t>& body);

    /**
     * @brief 处理历史记录拉取请求。
     * @param conn 请求连接。
     * @param hdr 协议头。
     * @param body 协议体。
     */
    void handleGetHistory(std::shared_ptr<TcpConnection> conn,
                          const PDUHeader& hdr,
                          const std::vector<uint8_t>& body);

private:
    /// @brief 用户仓储。
    UserRepository   users_;
    /// @brief 好友关系仓储。
    FriendRepository friends_;
    /// @brief 聊天仓储。
    ChatRepository   messages_;
    /// @brief 会话管理器引用。
    SessionManager&  sessions_;
};

} // namespace cloudvault
