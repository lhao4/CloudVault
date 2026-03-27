// =============================================================
// server/include/server/db/user_repository.h
// user_info 表的 CRUD 操作（使用 Prepared Statements）
// =============================================================

#pragma once

#include "server/db/database.h"

#include <optional>
#include <string>

namespace cloudvault {

/**
 * @brief user_info 表记录模型。
 */
struct UserInfo {
    /// @brief 用户 ID。
    int         user_id       = 0;
    /// @brief 用户名。
    std::string username;
    /// @brief 密码哈希值。
    std::string password_hash;
    /// @brief 在线状态。
    bool        is_online     = false;
};

/**
 * @brief 用户仓储。
 *
 * 封装 user_info 表的核心查询与更新操作。
 */
class UserRepository {
public:
    /**
     * @brief 构造仓储对象。
     * @param db 数据库连接池引用。
     */
    explicit UserRepository(Database& db);

    /**
     * @brief 新增用户。
     * @param username 用户名。
     * @param password_hash 密码哈希。
     * @return 成功返回 user_id(>0)，重名返回 -1，其它错误返回 0。
     */
    int insertUser(const std::string& username,
                   const std::string& password_hash);

    /**
     * @brief 按用户名查询用户信息。
     * @param username 用户名。
     * @return 用户记录，不存在返回 nullopt。
     */
    std::optional<UserInfo> findByName(const std::string& username);

    /**
     * @brief 更新在线状态。
     * @param user_id 用户 ID。
     * @param online 在线状态。
     * @return true 表示更新成功。
     */
    bool setOnline(int user_id, bool online);

private:
    /// @brief 数据库连接池引用。
    Database& db_;
};

} // namespace cloudvault
