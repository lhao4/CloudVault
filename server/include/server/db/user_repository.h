// =============================================================
// server/include/server/db/user_repository.h
// user_info 表的 CRUD 操作（使用 Prepared Statements）
// =============================================================

#pragma once

#include "server/db/database.h"

#include <optional>
#include <string>

namespace cloudvault {

// user_info 表的一行数据
struct UserInfo {
    int         user_id       = 0;
    std::string username;
    std::string password_hash;
    bool        is_online     = false;
};

class UserRepository {
public:
    explicit UserRepository(Database& db);

    // 注册新用户
    // 成功：返回 user_id（> 0）
    // 用户名已存在：返回 -1
    // 其他错误：返回 0
    int insertUser(const std::string& username,
                   const std::string& password_hash);

    // 按用户名查询，不存在时返回 nullopt
    std::optional<UserInfo> findByName(const std::string& username);

    // 更新在线状态
    bool setOnline(int user_id, bool online);

private:
    Database& db_;
};

} // namespace cloudvault
