// =============================================================
// server/src/db/friend_repository.cpp
// friend 表 CRUD
// =============================================================

#include "server/db/friend_repository.h"

#include <cstring>
#include <spdlog/spdlog.h>

namespace cloudvault {

namespace {

bool execSimple(MYSQL* conn, const char* sql) {
    return mysql_query(conn, sql) == 0;
}

bool prepareAndExecTwoInt(MYSQL* conn, const char* sql, int v1, int v2) {
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[2]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer      = &v1;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer      = &v2;

    const bool ok =
        mysql_stmt_bind_param(stmt, params) == 0 &&
        mysql_stmt_execute(stmt) == 0;
    if (!ok) {
        spdlog::warn("FriendRepository SQL failed: {}", mysql_stmt_error(stmt));
    }
    mysql_stmt_close(stmt);
    return ok;
}

} // namespace

FriendRepository::FriendRepository(Database& db) : db_(db) {}

// =============================================================
// areFriends(): 检查双方是否已是好友（status=1，任意方向）
// =============================================================
bool FriendRepository::areFriends(int user_id, int friend_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;

    const char* sql =
        "SELECT 1 FROM friend "
        "WHERE ((user_id = ? AND friend_id = ?) OR (user_id = ? AND friend_id = ?)) "
        "AND status = 1 LIMIT 1";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[4]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer      = &user_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer      = &friend_id;
    params[2].buffer_type = MYSQL_TYPE_LONG;
    params[2].buffer      = &friend_id;
    params[3].buffer_type = MYSQL_TYPE_LONG;
    params[3].buffer      = &user_id;
    mysql_stmt_bind_param(stmt, params);

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    int value = 0;
    MYSQL_BIND result{};
    result.buffer_type = MYSQL_TYPE_LONG;
    result.buffer      = &value;
    mysql_stmt_bind_result(stmt, &result);

    const bool found = (mysql_stmt_fetch(stmt) == 0);
    mysql_stmt_close(stmt);
    return found;
}

// =============================================================
// hasPendingRequest(): 检查是否有待处理申请（status=0）
// =============================================================
bool FriendRepository::hasPendingRequest(int from_id, int to_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;

    const char* sql =
        "SELECT 1 FROM friend "
        "WHERE user_id = ? AND friend_id = ? AND status = 0 LIMIT 1";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[2]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer      = &from_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer      = &to_id;
    mysql_stmt_bind_param(stmt, params);

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    int value = 0;
    MYSQL_BIND result{};
    result.buffer_type = MYSQL_TYPE_LONG;
    result.buffer      = &value;
    mysql_stmt_bind_result(stmt, &result);

    const bool found = (mysql_stmt_fetch(stmt) == 0);
    mysql_stmt_close(stmt);
    return found;
}

// =============================================================
// insertRequest(): 插入好友申请（单向，status=0）
// =============================================================
bool FriendRepository::insertRequest(int from_id, int to_id) {
    auto conn = db_.acquire();
    const char* sql =
        "INSERT INTO friend (user_id, friend_id, status) VALUES (?, ?, 0)";
    return prepareAndExecTwoInt(conn.get(), sql, from_id, to_id);
}

// =============================================================
// acceptRequest(): 同意申请，将 status 更新为 1
// =============================================================
bool FriendRepository::acceptRequest(int from_id, int to_id) {
    auto conn = db_.acquire();
    const char* sql =
        "UPDATE friend SET status = 1 WHERE user_id = ? AND friend_id = ? AND status = 0";
    return prepareAndExecTwoInt(conn.get(), sql, from_id, to_id);
}

// =============================================================
// removeFriend(): 删除好友关系（双向查找并删除）
// =============================================================
bool FriendRepository::removeFriend(int user_id, int friend_id) {
    auto conn = db_.acquire();
    MYSQL* raw = conn.get();

    if (!execSimple(raw, "START TRANSACTION")) return false;

    const char* sql =
        "DELETE FROM friend "
        "WHERE (user_id = ? AND friend_id = ?) OR (user_id = ? AND friend_id = ?)";

    MYSQL_STMT* stmt = mysql_stmt_init(raw);
    if (!stmt) {
        execSimple(raw, "ROLLBACK");
        return false;
    }

    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        execSimple(raw, "ROLLBACK");
        return false;
    }

    MYSQL_BIND params[4]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer      = &user_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer      = &friend_id;
    params[2].buffer_type = MYSQL_TYPE_LONG;
    params[2].buffer      = &friend_id;
    params[3].buffer_type = MYSQL_TYPE_LONG;
    params[3].buffer      = &user_id;
    mysql_stmt_bind_param(stmt, params);

    bool ok = (mysql_stmt_execute(stmt) == 0);
    mysql_stmt_close(stmt);

    if (ok) {
        return execSimple(raw, "COMMIT");
    }
    execSimple(raw, "ROLLBACK");
    return false;
}

// =============================================================
// listFriends(): 列出全部已确认好友（status=1），含在线状态
// 使用 UNION 查询处理单向记录
// =============================================================
std::vector<FriendInfo> FriendRepository::listFriends(int user_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return {};

    const char* sql =
        "SELECT u.user_id, u.username, COALESCE(u.nickname,''), COALESCE(u.signature,''), u.is_online "
        "FROM user_info u "
        "WHERE u.user_id IN ("
        "  SELECT friend_id FROM friend WHERE user_id = ? AND status = 1"
        "  UNION"
        "  SELECT user_id FROM friend WHERE friend_id = ? AND status = 1"
        ") "
        "ORDER BY u.username ASC";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        spdlog::error("listFriends prepare failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return {};
    }

    MYSQL_BIND param[2]{};
    param[0].buffer_type = MYSQL_TYPE_LONG;
    param[0].buffer      = &user_id;
    param[1].buffer_type = MYSQL_TYPE_LONG;
    param[1].buffer      = &user_id;
    mysql_stmt_bind_param(stmt, param);

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return {};
    }

    int  uid_val = 0;
    char username_buf[64]{};
    char nickname_buf[128]{};
    char signature_buf[256]{};
    char online_val = 0;
    unsigned long username_len = 0;
    unsigned long nickname_len = 0;
    unsigned long signature_len = 0;

    MYSQL_BIND results[5]{};
    results[0].buffer_type   = MYSQL_TYPE_LONG;
    results[0].buffer        = &uid_val;
    results[1].buffer_type   = MYSQL_TYPE_STRING;
    results[1].buffer        = username_buf;
    results[1].buffer_length = sizeof(username_buf);
    results[1].length        = &username_len;
    results[2].buffer_type   = MYSQL_TYPE_STRING;
    results[2].buffer        = nickname_buf;
    results[2].buffer_length = sizeof(nickname_buf);
    results[2].length        = &nickname_len;
    results[3].buffer_type   = MYSQL_TYPE_STRING;
    results[3].buffer        = signature_buf;
    results[3].buffer_length = sizeof(signature_buf);
    results[3].length        = &signature_len;
    results[4].buffer_type   = MYSQL_TYPE_TINY;
    results[4].buffer        = &online_val;
    mysql_stmt_bind_result(stmt, results);

    std::vector<FriendInfo> friends;
    while (mysql_stmt_fetch(stmt) == 0) {
        friends.push_back(FriendInfo{
            uid_val,
            std::string(username_buf, username_len),
            std::string(nickname_buf, nickname_len),
            std::string(signature_buf, signature_len),
            online_val != 0,
        });
    }

    mysql_stmt_close(stmt);
    return friends;
}

} // namespace cloudvault
