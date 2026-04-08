// =============================================================
// server/include/server/handler/auth_handler.h
// 用户认证业务逻辑处理器
//
// 处理的 MessageType：
//   REGISTER_REQUEST        → 注册（格式校验 → 检查重名 → 存库）
//   LOGIN_REQUEST           → 登录（查用户 → 验密码 → 防重复 → 注册会话）
//   LOGOUT                  → 登出（移除会话 → 更新 online 状态）
//   UPDATE_PROFILE_REQUEST  → 更新个人资料（昵称 + 签名）
// =============================================================

#pragma once

#include "server/db/database.h"
#include "server/db/chat_repository.h"
#include "server/db/user_repository.h"
#include "server/session_manager.h"
#include "common/protocol.h"

#include <memory>
#include <vector>

namespace cloudvault {

class TcpConnection;

/**
 * @brief 认证处理器。
 *
 * 负责注册、登录、登出请求处理，以及登录后离线消息投递。
 */
class AuthHandler {
public:
    /**
     * @brief 构造认证处理器。
     * @param db 数据库连接池引用。
     * @param sessions 会话管理器引用。
     */
    AuthHandler(Database& db, SessionManager& sessions);

    /**
     * @brief 处理注册请求。
     * @param conn 请求连接。
     * @param hdr 协议头。
     * @param body 协议体。
     */
    void handleRegister(std::shared_ptr<TcpConnection>  conn,
                        const PDUHeader&                hdr,
                        const std::vector<uint8_t>&     body);

    /**
     * @brief 处理登录请求。
     * @param conn 请求连接。
     * @param hdr 协议头。
     * @param body 协议体。
     */
    void handleLogin(std::shared_ptr<TcpConnection>  conn,
                     const PDUHeader&                hdr,
                     const std::vector<uint8_t>&     body);

    /**
     * @brief 处理登出请求。
     * @param conn 请求连接。
     * @param hdr 协议头。
     * @param body 协议体。
     */
    void handleLogout(std::shared_ptr<TcpConnection>  conn,
                      const PDUHeader&                hdr,
                      const std::vector<uint8_t>&     body);

    /**
     * @brief 处理更新个人资料请求。
     * @param conn 请求连接。
     * @param hdr 协议头。
     * @param body 协议体。
     */
    void handleUpdateProfile(std::shared_ptr<TcpConnection>  conn,
                             const PDUHeader&                hdr,
                             const std::vector<uint8_t>&     body);

private:
    /**
     * @brief 向刚登录用户投递离线消息。
     * @param conn 用户连接。
     * @param user_id 用户 ID。
     * @param username 用户名。
     */
    void deliverOfflineMessages(std::shared_ptr<TcpConnection> conn,
                                int user_id,
                                const std::string& username);

    /// @brief 用户仓储。
    UserRepository users_;
    /// @brief 聊天仓储。
    ChatRepository messages_;
    /// @brief 会话管理器引用。
    SessionManager& sessions_;
};

} // namespace cloudvault
