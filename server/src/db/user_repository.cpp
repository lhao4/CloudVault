// =============================================================
// server/src/db/user_repository.cpp
// user_info 表 CRUD（Prepared Statements）
// =============================================================

#include "server/db/user_repository.h"

#include <cstring>
#include <spdlog/spdlog.h>

namespace cloudvault {

UserRepository::UserRepository(Database& db) : db_(db) {}

// =============================================================
// insertUser()：注册新用户
// 返回 user_id（成功）/ -1（用户名重复）/ 0（其他错误）
// =============================================================
int UserRepository::insertUser(const std::string& username,
                               const std::string& password_hash) {
    auto conn = db_.acquire();

    // 使用 Prepared Statement 防止 SQL 注入
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return 0;

    const char* sql =
        "INSERT INTO user_info (username, password_hash) VALUES (?, ?)";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql))) != 0) {
        spdlog::error("insertUser prepare failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return 0;
    }

    // 绑定参数
    MYSQL_BIND params[2]{};

    // param[0]: username
    params[0].buffer_type   = MYSQL_TYPE_STRING;
    params[0].buffer        = const_cast<char*>(username.c_str());
    params[0].buffer_length = static_cast<unsigned long>(username.size());

    // param[1]: password_hash
    params[1].buffer_type   = MYSQL_TYPE_STRING;
    params[1].buffer        = const_cast<char*>(password_hash.c_str());
    params[1].buffer_length = static_cast<unsigned long>(password_hash.size());

    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return 0;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        unsigned int err = mysql_stmt_errno(stmt);
        mysql_stmt_close(stmt);
        // ER_DUP_ENTRY = 1062：唯一键冲突（用户名已存在）
        return (err == 1062) ? -1 : 0;
    }

    // 获取 AUTO_INCREMENT 生成的 user_id
    int user_id = static_cast<int>(mysql_stmt_insert_id(stmt));
    mysql_stmt_close(stmt);
    return user_id;
}

// =============================================================
// findByName()：按用户名查询
// =============================================================
std::optional<UserInfo> UserRepository::findByName(const std::string& username) {
    auto conn = db_.acquire();

    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return std::nullopt;

    const char* sql =
        "SELECT user_id, username, password_hash, is_online "
        "FROM user_info WHERE username = ? LIMIT 1";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return std::nullopt;
    }

    // 绑定输入参数
    MYSQL_BIND param{};
    param.buffer_type   = MYSQL_TYPE_STRING;
    param.buffer        = const_cast<char*>(username.c_str());
    param.buffer_length = static_cast<unsigned long>(username.size());
    mysql_stmt_bind_param(stmt, &param);

    // 执行并绑定结果列
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return std::nullopt;
    }

    // 结果缓冲区
    int    user_id      = 0;
    char   uname_buf[64]{};
    char   hash_buf[256]{};
    char   is_online_val = 0;

    unsigned long uname_len = 0, hash_len = 0;
    bool          is_online_null = false;

    MYSQL_BIND results[4]{};
    results[0].buffer_type   = MYSQL_TYPE_LONG;
    results[0].buffer        = &user_id;

    results[1].buffer_type   = MYSQL_TYPE_STRING;
    results[1].buffer        = uname_buf;
    results[1].buffer_length = sizeof(uname_buf);
    results[1].length        = &uname_len;

    results[2].buffer_type   = MYSQL_TYPE_STRING;
    results[2].buffer        = hash_buf;
    results[2].buffer_length = sizeof(hash_buf);
    results[2].length        = &hash_len;

    results[3].buffer_type   = MYSQL_TYPE_TINY;
    results[3].buffer        = &is_online_val;
    results[3].is_null       = &is_online_null;

    mysql_stmt_bind_result(stmt, results);

    std::optional<UserInfo> result;
    if (mysql_stmt_fetch(stmt) == 0) {
        UserInfo info;
        info.user_id       = user_id;
        info.username      = std::string(uname_buf, uname_len);
        info.password_hash = std::string(hash_buf, hash_len);
        info.is_online     = (is_online_val != 0);
        result = std::move(info);
    }

    mysql_stmt_close(stmt);
    return result;
}

// =============================================================
// setOnline()：更新在线状态
// =============================================================
bool UserRepository::setOnline(int user_id, bool online) {
    auto conn = db_.acquire();

    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;

    const char* sql = "UPDATE user_info SET is_online = ? WHERE user_id = ?";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    char online_val = online ? 1 : 0;

    MYSQL_BIND params[2]{};
    params[0].buffer_type = MYSQL_TYPE_TINY;
    params[0].buffer      = &online_val;

    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer      = &user_id;

    mysql_stmt_bind_param(stmt, params);
    bool ok = (mysql_stmt_execute(stmt) == 0);
    mysql_stmt_close(stmt);
    return ok;
}

} // namespace cloudvault
