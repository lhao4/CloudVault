// =============================================================
// server/src/db/friend_repository.cpp
// friend_relation 表 CRUD
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

bool FriendRepository::areFriends(int user_id, int friend_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;

    const char* sql =
        "SELECT 1 FROM friend_relation WHERE user_id = ? AND friend_id = ? LIMIT 1";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[2]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer      = &user_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer      = &friend_id;
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

bool FriendRepository::addFriendPair(int user_id, int friend_id) {
    auto conn = db_.acquire();
    MYSQL* raw = conn.get();

    if (!execSimple(raw, "START TRANSACTION")) return false;

    const char* sql =
        "INSERT INTO friend_relation (user_id, friend_id) VALUES (?, ?)";
    const bool ok1 = prepareAndExecTwoInt(raw, sql, user_id, friend_id);
    const bool ok2 = ok1 && prepareAndExecTwoInt(raw, sql, friend_id, user_id);

    if (ok1 && ok2) {
        return execSimple(raw, "COMMIT");
    }

    execSimple(raw, "ROLLBACK");
    return false;
}

bool FriendRepository::removeFriendPair(int user_id, int friend_id) {
    auto conn = db_.acquire();
    MYSQL* raw = conn.get();

    if (!execSimple(raw, "START TRANSACTION")) return false;

    const char* sql =
        "DELETE FROM friend_relation WHERE user_id = ? AND friend_id = ?";
    const bool ok1 = prepareAndExecTwoInt(raw, sql, user_id, friend_id);
    const bool ok2 = ok1 && prepareAndExecTwoInt(raw, sql, friend_id, user_id);

    if (ok1 && ok2) {
        return execSimple(raw, "COMMIT");
    }

    execSimple(raw, "ROLLBACK");
    return false;
}

std::vector<FriendInfo> FriendRepository::listFriends(int user_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return {};

    const char* sql =
        "SELECT u.user_id, u.username "
        "FROM friend_relation f "
        "JOIN user_info u ON u.user_id = f.friend_id "
        "WHERE f.user_id = ? "
        "ORDER BY u.username ASC";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return {};
    }

    MYSQL_BIND param{};
    param.buffer_type = MYSQL_TYPE_LONG;
    param.buffer      = &user_id;
    mysql_stmt_bind_param(stmt, &param);

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return {};
    }

    int user_id_value = 0;
    char username_buf[64]{};
    unsigned long username_len = 0;

    MYSQL_BIND results[2]{};
    results[0].buffer_type = MYSQL_TYPE_LONG;
    results[0].buffer      = &user_id_value;
    results[1].buffer_type = MYSQL_TYPE_STRING;
    results[1].buffer      = username_buf;
    results[1].buffer_length = sizeof(username_buf);
    results[1].length      = &username_len;
    mysql_stmt_bind_result(stmt, results);

    std::vector<FriendInfo> friends;
    while (mysql_stmt_fetch(stmt) == 0) {
        friends.push_back(FriendInfo{
            user_id_value,
            std::string(username_buf, username_len),
        });
    }

    mysql_stmt_close(stmt);
    return friends;
}

} // namespace cloudvault
