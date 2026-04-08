// =============================================================
// server/src/db/chat_repository.cpp
// 聊天记录与离线消息存储
// =============================================================

#include "server/db/chat_repository.h"

#include "common/protocol.h"

#include <mysql/mysql.h>
#include <spdlog/spdlog.h>

#include <array>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <optional>
#include <sstream>

namespace cloudvault {

namespace {

constexpr size_t kUsernameBufferSize = 64;
constexpr size_t kContentBufferSize  = 4096;
constexpr size_t kTimeBufferSize     = 32;

std::string currentTimestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    const auto time = system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.'
        << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

std::optional<MYSQL_STMT*> prepareStmt(MYSQL* conn, const char* sql) {
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        return std::nullopt;
    }
    if (mysql_stmt_prepare(stmt, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
        spdlog::warn("ChatRepository prepare failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return std::nullopt;
    }
    return stmt;
}

bool bindAndExecute(MYSQL_STMT* stmt, MYSQL_BIND* params) {
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        spdlog::warn("ChatRepository bind failed: {}", mysql_stmt_error(stmt));
        return false;
    }
    if (mysql_stmt_execute(stmt) != 0) {
        spdlog::warn("ChatRepository execute failed: {}", mysql_stmt_error(stmt));
        return false;
    }
    return true;
}

} // namespace

ChatRepository::ChatRepository(Database& db) : db_(db) {}

std::string ChatRepository::storePrivateMessage(int sender_id,
                                                int receiver_id,
                                                const std::string& content) {
    auto conn = db_.acquire();
    auto created_at = currentTimestamp();

    const char* sql =
        "INSERT INTO chat_message (sender_id, receiver_id, content, created_at) "
        "VALUES (?, ?, ?, ?)";
    auto stmt_opt = prepareStmt(conn.get(), sql);
    if (!stmt_opt) {
        return {};
    }

    MYSQL_STMT* stmt = *stmt_opt;
    unsigned long content_len = static_cast<unsigned long>(content.size());
    unsigned long created_len = static_cast<unsigned long>(created_at.size());

    MYSQL_BIND params[4]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &sender_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &receiver_id;
    params[2].buffer_type = MYSQL_TYPE_STRING;
    params[2].buffer = const_cast<char*>(content.data());
    params[2].buffer_length = content_len;
    params[2].length = &content_len;
    params[3].buffer_type = MYSQL_TYPE_STRING;
    params[3].buffer = const_cast<char*>(created_at.data());
    params[3].buffer_length = created_len;
    params[3].length = &created_len;

    const bool ok = bindAndExecute(stmt, params);
    mysql_stmt_close(stmt);
    return ok ? created_at : std::string{};
}

std::string ChatRepository::storeGroupMessage(int sender_id,
                                              int group_id,
                                              const std::string& content) {
    auto conn = db_.acquire();
    auto created_at = currentTimestamp();

    const char* sql =
        "INSERT INTO chat_message (sender_id, group_id, content, created_at) "
        "VALUES (?, ?, ?, ?)";
    auto stmt_opt = prepareStmt(conn.get(), sql);
    if (!stmt_opt) {
        return {};
    }

    MYSQL_STMT* stmt = *stmt_opt;
    unsigned long content_len = static_cast<unsigned long>(content.size());
    unsigned long created_len = static_cast<unsigned long>(created_at.size());

    MYSQL_BIND params[4]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &sender_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &group_id;
    params[2].buffer_type = MYSQL_TYPE_STRING;
    params[2].buffer = const_cast<char*>(content.data());
    params[2].buffer_length = content_len;
    params[2].length = &content_len;
    params[3].buffer_type = MYSQL_TYPE_STRING;
    params[3].buffer = const_cast<char*>(created_at.data());
    params[3].buffer_length = created_len;
    params[3].length = &created_len;

    const bool ok = bindAndExecute(stmt, params);
    mysql_stmt_close(stmt);
    return ok ? created_at : std::string{};
}

bool ChatRepository::queueOfflineMessage(int sender_id,
                                         int receiver_id,
                                         uint32_t msg_type,
                                         const std::string& content,
                                         const std::string& created_at) {
    auto conn = db_.acquire();
    const char* sql =
        "INSERT INTO offline_message "
        "(sender_id, receiver_id, msg_type, content, created_at, delivered) "
        "VALUES (?, ?, ?, ?, ?, 0)";
    auto stmt_opt = prepareStmt(conn.get(), sql);
    if (!stmt_opt) {
        return false;
    }

    MYSQL_STMT* stmt = *stmt_opt;
    unsigned long content_len = static_cast<unsigned long>(content.size());
    unsigned long created_len = static_cast<unsigned long>(created_at.size());
    uint32_t msg_type_val = msg_type;

    MYSQL_BIND params[5]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &sender_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &receiver_id;
    params[2].buffer_type = MYSQL_TYPE_LONG;
    params[2].buffer = &msg_type_val;
    params[3].buffer_type = MYSQL_TYPE_STRING;
    params[3].buffer = const_cast<char*>(content.data());
    params[3].buffer_length = content_len;
    params[3].length = &content_len;
    params[4].buffer_type = MYSQL_TYPE_STRING;
    params[4].buffer = const_cast<char*>(created_at.data());
    params[4].buffer_length = created_len;
    params[4].length = &created_len;

    const bool ok = bindAndExecute(stmt, params);
    mysql_stmt_close(stmt);
    return ok;
}

std::vector<ChatMessageRecord> ChatRepository::fetchUndeliveredMessages(int receiver_id) {
    auto conn = db_.acquire();
    const char* sql =
        "SELECT om.msg_type, om.sender_id, om.receiver_id, 0 AS group_id, "
        "su.username, ru.username, om.content, "
        "DATE_FORMAT(om.created_at, '%Y-%m-%d %H:%i:%s.%f') "
        "FROM offline_message om "
        "JOIN user_info su ON su.user_id = om.sender_id "
        "JOIN user_info ru ON ru.user_id = om.receiver_id "
        "WHERE om.receiver_id = ? AND om.delivered = 0 "
        "ORDER BY om.created_at ASC";
    auto stmt_opt = prepareStmt(conn.get(), sql);
    if (!stmt_opt) {
        return {};
    }

    MYSQL_STMT* stmt = *stmt_opt;
    MYSQL_BIND params[1]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &receiver_id;

    if (!bindAndExecute(stmt, params)) {
        mysql_stmt_close(stmt);
        return {};
    }

    uint32_t msg_type = 0;
    int sender_id = 0;
    int receiver_id_value = 0;
    int group_id = 0;
    std::array<char, kUsernameBufferSize> sender_buf{};
    std::array<char, kUsernameBufferSize> receiver_buf{};
    std::array<char, kContentBufferSize> content_buf{};
    std::array<char, kTimeBufferSize> time_buf{};
    unsigned long sender_len = 0;
    unsigned long receiver_len = 0;
    unsigned long content_len = 0;
    unsigned long time_len = 0;

    MYSQL_BIND results[8]{};
    results[0].buffer_type = MYSQL_TYPE_LONG;
    results[0].buffer = &msg_type;
    results[1].buffer_type = MYSQL_TYPE_LONG;
    results[1].buffer = &sender_id;
    results[2].buffer_type = MYSQL_TYPE_LONG;
    results[2].buffer = &receiver_id_value;
    results[3].buffer_type = MYSQL_TYPE_LONG;
    results[3].buffer = &group_id;
    results[4].buffer_type = MYSQL_TYPE_STRING;
    results[4].buffer = sender_buf.data();
    results[4].buffer_length = sender_buf.size();
    results[4].length = &sender_len;
    results[5].buffer_type = MYSQL_TYPE_STRING;
    results[5].buffer = receiver_buf.data();
    results[5].buffer_length = receiver_buf.size();
    results[5].length = &receiver_len;
    results[6].buffer_type = MYSQL_TYPE_STRING;
    results[6].buffer = content_buf.data();
    results[6].buffer_length = content_buf.size();
    results[6].length = &content_len;
    results[7].buffer_type = MYSQL_TYPE_STRING;
    results[7].buffer = time_buf.data();
    results[7].buffer_length = time_buf.size();
    results[7].length = &time_len;

    if (mysql_stmt_bind_result(stmt, results) != 0 || mysql_stmt_store_result(stmt) != 0) {
        spdlog::warn("ChatRepository fetch offline bind/store failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return {};
    }

    std::vector<ChatMessageRecord> messages;
    while (mysql_stmt_fetch(stmt) == 0) {
        messages.push_back(ChatMessageRecord{
            msg_type,
            sender_id,
            receiver_id_value,
            group_id,
            std::string(sender_buf.data(), sender_len),
            std::string(receiver_buf.data(), receiver_len),
            std::string(content_buf.data(), std::min<unsigned long>(content_len, content_buf.size())),
            std::string(time_buf.data(), time_len),
        });
    }

    mysql_stmt_close(stmt);
    return messages;
}

bool ChatRepository::markMessagesDelivered(int receiver_id) {
    auto conn = db_.acquire();
    const char* sql =
        "UPDATE offline_message SET delivered = 1 "
        "WHERE receiver_id = ? AND delivered = 0";
    auto stmt_opt = prepareStmt(conn.get(), sql);
    if (!stmt_opt) {
        return false;
    }

    MYSQL_STMT* stmt = *stmt_opt;
    MYSQL_BIND params[1]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &receiver_id;
    const bool ok = bindAndExecute(stmt, params);
    mysql_stmt_close(stmt);
    return ok;
}

std::vector<ChatMessageRecord> ChatRepository::fetchPrivateHistory(int user_id,
                                                                   int peer_id,
                                                                   uint16_t limit) {
    auto conn = db_.acquire();
    const char* sql =
        "SELECT m.sender_id, m.receiver_id, su.username, ru.username, m.content, "
        "DATE_FORMAT(m.created_at, '%Y-%m-%d %H:%i:%s.%f') "
        "FROM chat_message m "
        "JOIN user_info su ON su.user_id = m.sender_id "
        "JOIN user_info ru ON ru.user_id = m.receiver_id "
        "WHERE ((m.sender_id = ? AND m.receiver_id = ?) "
        "    OR (m.sender_id = ? AND m.receiver_id = ?)) "
        "ORDER BY m.created_at ASC "
        "LIMIT ?";
    auto stmt_opt = prepareStmt(conn.get(), sql);
    if (!stmt_opt) {
        return {};
    }

    MYSQL_STMT* stmt = *stmt_opt;
    uint32_t limit_val = limit;
    MYSQL_BIND params[5]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &user_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &peer_id;
    params[2].buffer_type = MYSQL_TYPE_LONG;
    params[2].buffer = &peer_id;
    params[3].buffer_type = MYSQL_TYPE_LONG;
    params[3].buffer = &user_id;
    params[4].buffer_type = MYSQL_TYPE_LONG;
    params[4].buffer = &limit_val;

    if (!bindAndExecute(stmt, params)) {
        mysql_stmt_close(stmt);
        return {};
    }

    int sender_id = 0;
    int receiver_id_value = 0;
    std::array<char, kUsernameBufferSize> sender_buf{};
    std::array<char, kUsernameBufferSize> receiver_buf{};
    std::array<char, kContentBufferSize> content_buf{};
    std::array<char, kTimeBufferSize> time_buf{};
    unsigned long sender_len = 0;
    unsigned long receiver_len = 0;
    unsigned long content_len = 0;
    unsigned long time_len = 0;

    MYSQL_BIND results[6]{};
    results[0].buffer_type = MYSQL_TYPE_LONG;
    results[0].buffer = &sender_id;
    results[1].buffer_type = MYSQL_TYPE_LONG;
    results[1].buffer = &receiver_id_value;
    results[2].buffer_type = MYSQL_TYPE_STRING;
    results[2].buffer = sender_buf.data();
    results[2].buffer_length = sender_buf.size();
    results[2].length = &sender_len;
    results[3].buffer_type = MYSQL_TYPE_STRING;
    results[3].buffer = receiver_buf.data();
    results[3].buffer_length = receiver_buf.size();
    results[3].length = &receiver_len;
    results[4].buffer_type = MYSQL_TYPE_STRING;
    results[4].buffer = content_buf.data();
    results[4].buffer_length = content_buf.size();
    results[4].length = &content_len;
    results[5].buffer_type = MYSQL_TYPE_STRING;
    results[5].buffer = time_buf.data();
    results[5].buffer_length = time_buf.size();
    results[5].length = &time_len;

    if (mysql_stmt_bind_result(stmt, results) != 0 || mysql_stmt_store_result(stmt) != 0) {
        spdlog::warn("ChatRepository fetch history bind/store failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return {};
    }

    std::vector<ChatMessageRecord> messages;
    while (mysql_stmt_fetch(stmt) == 0) {
        messages.push_back(ChatMessageRecord{
            static_cast<uint32_t>(MessageType::CHAT),
            sender_id,
            receiver_id_value,
            0,
            std::string(sender_buf.data(), sender_len),
            std::string(receiver_buf.data(), receiver_len),
            std::string(content_buf.data(), std::min<unsigned long>(content_len, content_buf.size())),
            std::string(time_buf.data(), time_len),
        });
    }

    mysql_stmt_close(stmt);
    return messages;
}

std::vector<ChatMessageRecord> ChatRepository::fetchGroupHistory(int group_id,
                                                                 uint16_t limit) {
    auto conn = db_.acquire();
    const char* sql =
        "SELECT m.sender_id, m.group_id, su.username, m.content, "
        "DATE_FORMAT(m.created_at, '%Y-%m-%d %H:%i:%s.%f') "
        "FROM chat_message m "
        "JOIN user_info su ON su.user_id = m.sender_id "
        "WHERE m.group_id = ? "
        "ORDER BY m.created_at ASC "
        "LIMIT ?";
    auto stmt_opt = prepareStmt(conn.get(), sql);
    if (!stmt_opt) {
        return {};
    }

    MYSQL_STMT* stmt = *stmt_opt;
    uint32_t limit_val = limit;
    MYSQL_BIND params[2]{};
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &group_id;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &limit_val;

    if (!bindAndExecute(stmt, params)) {
        mysql_stmt_close(stmt);
        return {};
    }

    int sender_id = 0;
    int group_id_value = 0;
    std::array<char, kUsernameBufferSize> sender_buf{};
    std::array<char, kContentBufferSize> content_buf{};
    std::array<char, kTimeBufferSize> time_buf{};
    unsigned long sender_len = 0;
    unsigned long content_len = 0;
    unsigned long time_len = 0;

    MYSQL_BIND results[5]{};
    results[0].buffer_type = MYSQL_TYPE_LONG;
    results[0].buffer = &sender_id;
    results[1].buffer_type = MYSQL_TYPE_LONG;
    results[1].buffer = &group_id_value;
    results[2].buffer_type = MYSQL_TYPE_STRING;
    results[2].buffer = sender_buf.data();
    results[2].buffer_length = sender_buf.size();
    results[2].length = &sender_len;
    results[3].buffer_type = MYSQL_TYPE_STRING;
    results[3].buffer = content_buf.data();
    results[3].buffer_length = content_buf.size();
    results[3].length = &content_len;
    results[4].buffer_type = MYSQL_TYPE_STRING;
    results[4].buffer = time_buf.data();
    results[4].buffer_length = time_buf.size();
    results[4].length = &time_len;

    if (mysql_stmt_bind_result(stmt, results) != 0 || mysql_stmt_store_result(stmt) != 0) {
        spdlog::warn("ChatRepository fetch group history bind/store failed: {}",
                     mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return {};
    }

    std::vector<ChatMessageRecord> messages;
    while (mysql_stmt_fetch(stmt) == 0) {
        messages.push_back(ChatMessageRecord{
            static_cast<uint32_t>(MessageType::GROUP_CHAT),
            sender_id,
            0,
            group_id_value,
            std::string(sender_buf.data(), sender_len),
            {},
            std::string(content_buf.data(), std::min<unsigned long>(content_len, content_buf.size())),
            std::string(time_buf.data(), time_len),
        });
    }

    mysql_stmt_close(stmt);
    return messages;
}

} // namespace cloudvault
