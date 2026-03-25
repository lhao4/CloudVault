// =============================================================
// server/include/server/session_manager.h
// 在线会话管理（线程安全）
//
// 职责：
//   - 维护 user_id ↔ TcpConnection 映射
//   - 登录时注册，断开时移除
//   - shared_mutex：允许多个工作线程并发查询，写入互斥
// =============================================================

#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace cloudvault {

class TcpConnection;

class SessionManager {
public:
    // 注册登录会话
    void addSession(int user_id,
                    const std::string& username,
                    std::shared_ptr<TcpConnection> conn);

    // 按 user_id 移除（登出时调用）
    void removeSession(int user_id);

    // 按连接对象移除（连接断开时调用，user_id 未知）
    void removeByConnection(std::shared_ptr<TcpConnection> conn);

    // 查询用户是否在线
    bool isOnline(const std::string& username) const;

    // 获取用户的连接（不存在时返回 nullptr）
    std::shared_ptr<TcpConnection> getConnection(const std::string& username) const;

    // 获取在线用户名列表
    std::vector<std::string> onlineUsers() const;

    // 向指定用户发送 PDU（线程安全）
    void sendTo(const std::string& username, std::vector<uint8_t> data);

private:
    struct Session {
        int                            user_id;
        std::string                    username;
        std::shared_ptr<TcpConnection> conn;
    };

    mutable std::shared_mutex mutex_;
    std::unordered_map<int, Session>         by_id_;        // user_id → Session
    std::unordered_map<std::string, int>     name_to_id_;   // username → user_id
};

} // namespace cloudvault
