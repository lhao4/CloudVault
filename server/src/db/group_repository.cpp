// =============================================================
// server/src/db/group_repository.cpp
// chat_group / group_member 表 CRUD
// =============================================================

#include "server/db/group_repository.h"

#include <cstring>
#include <spdlog/spdlog.h>

namespace cloudvault {

namespace {

bool execSimple(MYSQL* conn, const char* sql) {
    return mysql_query(conn, sql) == 0;
}

} // namespace

GroupRepository::GroupRepository(Database& db) : db_(db) {}

// =============================================================
// createGroup(): 创建群组并将群主加为成员（事务）
// =============================================================
int GroupRepository::createGroup(const std::string& name, int owner_id) {
    auto conn = db_.acquire();
    MYSQL* raw = conn.get();

    if (!execSimple(raw, "START TRANSACTION")) return 0;

    // 插入群组
    {
        const char* sql = "INSERT INTO chat_group (name, owner_id) VALUES (?, ?)";
        MYSQL_STMT* stmt = mysql_stmt_init(raw);
        if (!stmt) { execSimple(raw, "ROLLBACK"); return 0; }
        if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
            mysql_stmt_close(stmt);
            execSimple(raw, "ROLLBACK");
            return 0;
        }

        auto name_len = static_cast<unsigned long>(name.size());
        MYSQL_BIND params[2]{};
        params[0].buffer_type   = MYSQL_TYPE_STRING;
        params[0].buffer        = const_cast<char*>(name.c_str());
        params[0].buffer_length = name_len;
        params[0].length        = &name_len;
        params[1].buffer_type   = MYSQL_TYPE_LONG;
        params[1].buffer        = &owner_id;

        if (mysql_stmt_bind_param(stmt, params) != 0 ||
            mysql_stmt_execute(stmt) != 0) {
            spdlog::error("createGroup insert failed: {}", mysql_stmt_error(stmt));
            mysql_stmt_close(stmt);
            execSimple(raw, "ROLLBACK");
            return 0;
        }

        int group_id = static_cast<int>(mysql_stmt_insert_id(stmt));
        mysql_stmt_close(stmt);

        // 将群主加为成员
        const char* member_sql =
            "INSERT INTO group_member (group_id, user_id) VALUES (?, ?)";
        MYSQL_STMT* stmt2 = mysql_stmt_init(raw);
        if (!stmt2) { execSimple(raw, "ROLLBACK"); return 0; }
        if (mysql_stmt_prepare(stmt2, member_sql,
                               static_cast<unsigned long>(std::strlen(member_sql))) != 0) {
            mysql_stmt_close(stmt2);
            execSimple(raw, "ROLLBACK");
            return 0;
        }

        MYSQL_BIND mp[2]{};
        mp[0].buffer_type = MYSQL_TYPE_LONG;
        mp[0].buffer      = &group_id;
        mp[1].buffer_type = MYSQL_TYPE_LONG;
        mp[1].buffer      = &owner_id;

        if (mysql_stmt_bind_param(stmt2, mp) != 0 ||
            mysql_stmt_execute(stmt2) != 0) {
            spdlog::error("createGroup addMember failed: {}", mysql_stmt_error(stmt2));
            mysql_stmt_close(stmt2);
            execSimple(raw, "ROLLBACK");
            return 0;
        }
        mysql_stmt_close(stmt2);

        if (!execSimple(raw, "COMMIT")) return 0;
        return group_id;
    }
}

bool GroupRepository::addMember(int group_id, int user_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;

    const char* sql = "INSERT INTO group_member (group_id, user_id) VALUES (?, ?)";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[2]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer      = &group_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer      = &user_id;

    bool ok = mysql_stmt_bind_param(stmt, params) == 0 &&
              mysql_stmt_execute(stmt) == 0;
    mysql_stmt_close(stmt);
    return ok;
}

bool GroupRepository::removeMember(int group_id, int user_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;

    const char* sql = "DELETE FROM group_member WHERE group_id = ? AND user_id = ?";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[2]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer      = &group_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer      = &user_id;

    bool ok = mysql_stmt_bind_param(stmt, params) == 0 &&
              mysql_stmt_execute(stmt) == 0;
    mysql_stmt_close(stmt);
    return ok;
}

bool GroupRepository::deleteGroup(int group_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;

    // group_member 会被 FK ON DELETE CASCADE 自动清理
    const char* sql = "DELETE FROM chat_group WHERE id = ?";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND param{};
    param.buffer_type = MYSQL_TYPE_LONG;
    param.buffer      = &group_id;

    bool ok = mysql_stmt_bind_param(stmt, &param) == 0 &&
              mysql_stmt_execute(stmt) == 0;
    mysql_stmt_close(stmt);
    return ok;
}

bool GroupRepository::isMember(int group_id, int user_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;

    const char* sql =
        "SELECT 1 FROM group_member WHERE group_id = ? AND user_id = ? LIMIT 1";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND params[2]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer      = &group_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer      = &user_id;
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

    bool found = (mysql_stmt_fetch(stmt) == 0);
    mysql_stmt_close(stmt);
    return found;
}

std::optional<GroupInfo> GroupRepository::findGroup(int group_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return std::nullopt;

    const char* sql = "SELECT id, name, owner_id FROM chat_group WHERE id = ? LIMIT 1";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return std::nullopt;
    }

    MYSQL_BIND param{};
    param.buffer_type = MYSQL_TYPE_LONG;
    param.buffer      = &group_id;
    mysql_stmt_bind_param(stmt, &param);

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return std::nullopt;
    }

    int  id_val = 0, owner_val = 0;
    char name_buf[128]{};
    unsigned long name_len = 0;

    MYSQL_BIND results[3]{};
    results[0].buffer_type   = MYSQL_TYPE_LONG;
    results[0].buffer        = &id_val;
    results[1].buffer_type   = MYSQL_TYPE_STRING;
    results[1].buffer        = name_buf;
    results[1].buffer_length = sizeof(name_buf);
    results[1].length        = &name_len;
    results[2].buffer_type   = MYSQL_TYPE_LONG;
    results[2].buffer        = &owner_val;
    mysql_stmt_bind_result(stmt, results);

    std::optional<GroupInfo> result;
    if (mysql_stmt_fetch(stmt) == 0) {
        result = GroupInfo{id_val, std::string(name_buf, name_len), owner_val};
    }
    mysql_stmt_close(stmt);
    return result;
}

std::vector<GroupInfo> GroupRepository::listUserGroups(int user_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return {};

    const char* sql =
        "SELECT g.id, g.name, g.owner_id "
        "FROM chat_group g "
        "JOIN group_member gm ON g.id = gm.group_id "
        "WHERE gm.user_id = ? "
        "ORDER BY g.name ASC";
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

    int  id_val = 0, owner_val = 0;
    char name_buf[128]{};
    unsigned long name_len = 0;

    MYSQL_BIND results[3]{};
    results[0].buffer_type   = MYSQL_TYPE_LONG;
    results[0].buffer        = &id_val;
    results[1].buffer_type   = MYSQL_TYPE_STRING;
    results[1].buffer        = name_buf;
    results[1].buffer_length = sizeof(name_buf);
    results[1].length        = &name_len;
    results[2].buffer_type   = MYSQL_TYPE_LONG;
    results[2].buffer        = &owner_val;
    mysql_stmt_bind_result(stmt, results);

    std::vector<GroupInfo> groups;
    while (mysql_stmt_fetch(stmt) == 0) {
        groups.push_back(GroupInfo{id_val, std::string(name_buf, name_len), owner_val});
    }
    mysql_stmt_close(stmt);
    return groups;
}

std::vector<GroupMemberInfo> GroupRepository::listMembers(int group_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return {};

    const char* sql =
        "SELECT u.user_id, u.username, u.is_online "
        "FROM user_info u "
        "JOIN group_member gm ON u.user_id = gm.user_id "
        "WHERE gm.group_id = ? "
        "ORDER BY u.username ASC";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return {};
    }

    MYSQL_BIND param{};
    param.buffer_type = MYSQL_TYPE_LONG;
    param.buffer      = &group_id;
    mysql_stmt_bind_param(stmt, &param);

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return {};
    }

    int  uid_val = 0;
    char uname_buf[64]{};
    char online_val = 0;
    unsigned long uname_len = 0;

    MYSQL_BIND results[3]{};
    results[0].buffer_type   = MYSQL_TYPE_LONG;
    results[0].buffer        = &uid_val;
    results[1].buffer_type   = MYSQL_TYPE_STRING;
    results[1].buffer        = uname_buf;
    results[1].buffer_length = sizeof(uname_buf);
    results[1].length        = &uname_len;
    results[2].buffer_type   = MYSQL_TYPE_TINY;
    results[2].buffer        = &online_val;
    mysql_stmt_bind_result(stmt, results);

    std::vector<GroupMemberInfo> members;
    while (mysql_stmt_fetch(stmt) == 0) {
        members.push_back(GroupMemberInfo{
            uid_val, std::string(uname_buf, uname_len), online_val != 0});
    }
    mysql_stmt_close(stmt);
    return members;
}

std::vector<GroupMemberInfo> GroupRepository::listOnlineMembers(int group_id) {
    auto conn = db_.acquire();
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return {};

    const char* sql =
        "SELECT u.user_id, u.username "
        "FROM user_info u "
        "JOIN group_member gm ON u.user_id = gm.user_id "
        "WHERE gm.group_id = ? AND u.is_online = 1 "
        "ORDER BY u.username ASC";
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        mysql_stmt_close(stmt);
        return {};
    }

    MYSQL_BIND param{};
    param.buffer_type = MYSQL_TYPE_LONG;
    param.buffer      = &group_id;
    mysql_stmt_bind_param(stmt, &param);

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return {};
    }

    int  uid_val = 0;
    char uname_buf[64]{};
    unsigned long uname_len = 0;

    MYSQL_BIND results[2]{};
    results[0].buffer_type   = MYSQL_TYPE_LONG;
    results[0].buffer        = &uid_val;
    results[1].buffer_type   = MYSQL_TYPE_STRING;
    results[1].buffer        = uname_buf;
    results[1].buffer_length = sizeof(uname_buf);
    results[1].length        = &uname_len;
    mysql_stmt_bind_result(stmt, results);

    std::vector<GroupMemberInfo> members;
    while (mysql_stmt_fetch(stmt) == 0) {
        members.push_back(GroupMemberInfo{
            uid_val, std::string(uname_buf, uname_len), true});
    }
    mysql_stmt_close(stmt);
    return members;
}

} // namespace cloudvault
