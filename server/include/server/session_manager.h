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
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace cloudvault {

class TcpConnection;

/**
 * @brief 在线会话管理器（线程安全）。
 *
 * 维护 user_id 与连接对象的双向索引，用于在线状态判断、
 * 连接反查和定向消息推送。
 */
class SessionManager {
public:
    /**
     * @brief 会话基础信息。
     */
    struct SessionInfo {
        /// @brief 用户 ID。
        int         user_id = 0;
        /// @brief 用户名。
        std::string username;
    };

    /**
     * @brief 注册登录会话。
     * @param user_id 用户 ID。
     * @param username 用户名。
     * @param conn 连接对象。
     */
    void addSession(int user_id,
                    const std::string& username,
                    std::shared_ptr<TcpConnection> conn);

    /**
     * @brief 按 user_id 移除会话。
     * @param user_id 用户 ID。
     */
    void removeSession(int user_id);

    /**
     * @brief 按连接对象移除会话。
     * @param conn 连接对象。
     */
    void removeByConnection(std::shared_ptr<TcpConnection> conn);

    /**
     * @brief 查询用户是否在线。
     * @param username 用户名。
     * @return true 表示在线。
     */
    bool isOnline(const std::string& username) const;

    /**
     * @brief 获取用户连接对象。
     * @param username 用户名。
     * @return 连接对象，不存在则返回 nullptr。
     */
    std::shared_ptr<TcpConnection> getConnection(const std::string& username) const;

    /**
     * @brief 获取在线用户名列表。
     * @return 用户名数组。
     */
    std::vector<std::string> onlineUsers() const;

    /**
     * @brief 根据连接反查会话身份。
     * @param conn 连接对象。
     * @return 会话信息，不存在返回 nullopt。
     */
    std::optional<SessionInfo>
    findByConnection(std::shared_ptr<TcpConnection> conn) const;

    /**
     * @brief 向指定用户发送 PDU（线程安全）。
     * @param username 目标用户名。
     * @param data 已编码 PDU 字节。
     */
    void sendTo(const std::string& username, std::vector<uint8_t> data);

private:
    /**
     * @brief 内部会话结构。
     */
    struct Session {
        /// @brief 用户 ID。
        int                            user_id;
        /// @brief 用户名。
        std::string                    username;
        /// @brief 对应连接对象。
        std::shared_ptr<TcpConnection> conn;
    };

    /// @brief 读写锁，支持并发读。
    mutable std::shared_mutex mutex_;
    /// @brief user_id -> Session。
    std::unordered_map<int, Session>         by_id_;
    /// @brief username -> user_id。
    std::unordered_map<std::string, int>     name_to_id_;
};

} // namespace cloudvault
