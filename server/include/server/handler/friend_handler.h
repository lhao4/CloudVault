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

/**
 * @brief 好友处理器。
 *
 * 负责用户查找、好友申请、同意申请、好友列表刷新与删除好友流程。
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
    /**
     * @brief 记录挂起好友请求。
     * @param target_user_id 目标用户 ID。
     * @param requester_user_id 申请方用户 ID。
     * @return true 表示记录成功。
     */
    bool addPendingRequest(int target_user_id, int requester_user_id);
    /**
     * @brief 消费挂起好友请求。
     * @param target_user_id 目标用户 ID。
     * @param requester_user_id 申请方用户 ID。
     * @return true 表示存在并已消费。
     */
    bool consumePendingRequest(int target_user_id, int requester_user_id);

    /// @brief 用户仓储。
    UserRepository   users_;
    /// @brief 好友关系仓储。
    FriendRepository friends_;
    /// @brief 会话管理器引用。
    SessionManager&  sessions_;
    /// @brief 挂起请求集合互斥锁。
    std::mutex pending_mutex_;
    /// @brief 挂起请求表：target_user_id -> requester_id 集合。
    std::unordered_map<int, std::unordered_set<int>> pending_requests_;
};

} // namespace cloudvault
