// =============================================================
// server/include/server/db/chat_repository.h
// 聊天记录与离线消息存储
// =============================================================

#pragma once

#include "server/db/database.h"

#include <cstdint>
#include <string>
#include <vector>

namespace cloudvault {

struct ChatMessageRecord {
    int         sender_id = 0;
    int         receiver_id = 0;
    std::string sender_username;
    std::string receiver_username;
    std::string content;
    std::string created_at;
};

class ChatRepository {
public:
    explicit ChatRepository(Database& db);

    std::string storePrivateMessage(int sender_id,
                                    int receiver_id,
                                    const std::string& content);

    bool queueOfflineMessage(int sender_id,
                             int receiver_id,
                             uint32_t msg_type,
                             const std::string& content,
                             const std::string& created_at);

    std::vector<ChatMessageRecord> fetchUndeliveredMessages(int receiver_id);
    bool markMessagesDelivered(int receiver_id);

    std::vector<ChatMessageRecord> fetchPrivateHistory(int user_id,
                                                       int peer_id,
                                                       uint16_t limit = 100);

private:
    Database& db_;
};

} // namespace cloudvault
