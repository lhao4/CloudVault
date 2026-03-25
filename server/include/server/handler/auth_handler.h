// =============================================================
// server/include/server/handler/auth_handler.h
// 用户认证业务逻辑处理器
//
// 处理的 MessageType：
//   REGISTER_REQUEST → 注册（格式校验 → 检查重名 → 存库）
//   LOGIN_REQUEST    → 登录（查用户 → 验密码 → 防重复 → 注册会话）
//   LOGOUT           → 登出（移除会话 → 更新 online 状态）
// =============================================================

#pragma once

#include "server/db/database.h"
#include "server/db/user_repository.h"
#include "server/session_manager.h"
#include "common/protocol.h"

#include <memory>
#include <vector>

namespace cloudvault {

class TcpConnection;

class AuthHandler {
public:
    AuthHandler(Database& db, SessionManager& sessions);

    void handleRegister(std::shared_ptr<TcpConnection>  conn,
                        const PDUHeader&                hdr,
                        const std::vector<uint8_t>&     body);

    void handleLogin(std::shared_ptr<TcpConnection>  conn,
                     const PDUHeader&                hdr,
                     const std::vector<uint8_t>&     body);

    void handleLogout(std::shared_ptr<TcpConnection>  conn,
                      const PDUHeader&                hdr,
                      const std::vector<uint8_t>&     body);

private:
    UserRepository users_;
    SessionManager& sessions_;
};

} // namespace cloudvault
