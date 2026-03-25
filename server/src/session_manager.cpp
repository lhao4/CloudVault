// =============================================================
// server/src/session_manager.cpp
// 在线会话管理实现
// =============================================================

#include "server/session_manager.h"
#include "server/tcp_connection.h"

#include <spdlog/spdlog.h>

namespace cloudvault {

void SessionManager::addSession(int user_id,
                                 const std::string& username,
                                 std::shared_ptr<TcpConnection> conn) {
    std::unique_lock lock(mutex_);
    by_id_[user_id]      = Session{user_id, username, conn};
    name_to_id_[username] = user_id;
    spdlog::debug("Session added: {} (uid={})", username, user_id);
}

void SessionManager::removeSession(int user_id) {
    std::unique_lock lock(mutex_);
    auto it = by_id_.find(user_id);
    if (it != by_id_.end()) {
        name_to_id_.erase(it->second.username);
        by_id_.erase(it);
        spdlog::debug("Session removed: uid={}", user_id);
    }
}

void SessionManager::removeByConnection(std::shared_ptr<TcpConnection> conn) {
    std::unique_lock lock(mutex_);
    for (auto it = by_id_.begin(); it != by_id_.end(); ++it) {
        if (it->second.conn == conn) {
            name_to_id_.erase(it->second.username);
            spdlog::debug("Session removed by conn: {}", it->second.username);
            by_id_.erase(it);
            return;
        }
    }
}

bool SessionManager::isOnline(const std::string& username) const {
    std::shared_lock lock(mutex_);
    return name_to_id_.count(username) > 0;
}

std::shared_ptr<TcpConnection>
SessionManager::getConnection(const std::string& username) const {
    std::shared_lock lock(mutex_);
    auto it = name_to_id_.find(username);
    if (it == name_to_id_.end()) return nullptr;
    auto it2 = by_id_.find(it->second);
    if (it2 == by_id_.end()) return nullptr;
    return it2->second.conn;
}

std::vector<std::string> SessionManager::onlineUsers() const {
    std::shared_lock lock(mutex_);
    std::vector<std::string> users;
    users.reserve(by_id_.size());
    for (const auto& [id, sess] : by_id_) {
        users.push_back(sess.username);
    }
    return users;
}

void SessionManager::sendTo(const std::string& username,
                             std::vector<uint8_t> data) {
    auto conn = getConnection(username);
    if (conn) {
        conn->send(std::move(data));
    }
}

} // namespace cloudvault
