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

/**
 * @brief 文件分享处理器。
 *
 * 负责分享请求发送、分享接收确认以及挂起分享状态管理。
 */
class ShareHandler {
public:
    /**
     * @brief 构造分享处理器。
     * @param db 数据库连接池引用。
     * @param sessions 会话管理器引用。
     * @param storage 文件存储抽象引用。
     */
    ShareHandler(Database& db, SessionManager& sessions, FileStorage& storage);

    /// @brief 处理发起分享请求。
    void handleShareRequest(std::shared_ptr<TcpConnection> conn,
                            const PDUHeader& hdr,
                            const std::vector<uint8_t>& body);

    /// @brief 处理同意分享请求。
    void handleShareAgree(std::shared_ptr<TcpConnection> conn,
                          const PDUHeader& hdr,
                          const std::vector<uint8_t>& body);

    /**
     * @brief 连接关闭时清理挂起分享状态。
     * @param conn 关闭连接。
     */
    void handleConnectionClosed(std::shared_ptr<TcpConnection> conn);

private:
    /**
     * @brief 计算挂起分享键值。
     * @param sender 发送方用户名。
     * @param file_path 文件路径。
     * @return 组合键字符串。
     */
    static std::string pendingKey(const std::string& sender, const std::string& file_path);

    /// @brief 用户仓储。
    UserRepository users_;
    /// @brief 好友关系仓储。
    FriendRepository friends_;
    /// @brief 会话管理器引用。
    SessionManager& sessions_;
    /// @brief 文件存储抽象引用。
    FileStorage& storage_;

    /// @brief 挂起分享状态锁。
    std::mutex pending_mutex_;
    /// @brief 挂起分享集合：receiver -> {sender|path key...}。
    std::unordered_map<std::string, std::unordered_set<std::string>> pending_shares_;
};

} // namespace cloudvault
