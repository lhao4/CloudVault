// =============================================================
// server/include/server/handler/friend_handler.h
// 好友系统业务逻辑
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
 * @brief 好友处理器。
 *
 * 负责用户查找、好友申请、同意申请、好友列表刷新与删除好友流程。
 * 好友申请状态通过 friend 表的 status 字段持久化（0=待处理, 1=已添加），
 * 支持离线好友申请。
 */
class FriendHandler {
public:
    /**
     * @brief 构造好友处理器。
     * @param db 数据库连接池引用。
     * @param sessions 会话管理器引用。
     */
    FriendHandler(Database& db, SessionManager& sessions);

    /// @brief 处理查找用户请求。
    void handleFindUser(std::shared_ptr<TcpConnection> conn,
                        const PDUHeader& hdr,
                        const std::vector<uint8_t>& body);
    /// @brief 处理添加好友请求。
    void handleAddFriend(std::shared_ptr<TcpConnection> conn,
                         const PDUHeader& hdr,
                         const std::vector<uint8_t>& body);
    /// @brief 处理同意好友请求。
    void handleAgreeFriend(std::shared_ptr<TcpConnection> conn,
                           const PDUHeader& hdr,
                           const std::vector<uint8_t>& body);
    /// @brief 处理刷新好友列表请求。
    void handleFlushFriends(std::shared_ptr<TcpConnection> conn,
                            const PDUHeader& hdr,
                            const std::vector<uint8_t>& body);
    /// @brief 处理删除好友请求。
    void handleDeleteFriend(std::shared_ptr<TcpConnection> conn,
                            const PDUHeader& hdr,
                            const std::vector<uint8_t>& body);

private:
    /// @brief 用户仓储。
    UserRepository   users_;
    /// @brief 好友关系仓储。
    FriendRepository friends_;
    /// @brief 聊天仓储（用于离线好友申请入队）。
    ChatRepository   messages_;
    /// @brief 会话管理器引用。
    SessionManager&  sessions_;
};

} // namespace cloudvault
