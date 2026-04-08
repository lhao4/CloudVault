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

/**
 * @brief 聊天消息记录模型。
 */
struct ChatMessageRecord {
    /// @brief 消息类型（CHAT / GROUP_CHAT）。
    uint32_t    msg_type = 0;
    /// @brief 发送者 ID。
    int         sender_id = 0;
    /// @brief 接收者 ID。
    int         receiver_id = 0;
    /// @brief 群组 ID（私聊时为 0）。
    int         group_id = 0;
    /// @brief 发送者用户名。
    std::string sender_username;
    /// @brief 接收者用户名。
    std::string receiver_username;
    /// @brief 消息内容。
    std::string content;
    /// @brief 创建时间文本。
    std::string created_at;
};

/**
 * @brief 聊天仓储。
 *
 * 封装聊天消息持久化、离线队列和历史查询操作。
 */
class ChatRepository {
public:
    /**
     * @brief 构造仓储对象。
     * @param db 数据库连接池引用。
     */
    explicit ChatRepository(Database& db);

    /**
     * @brief 持久化私聊消息。
     * @param sender_id 发送者 ID。
     * @param receiver_id 接收者 ID。
     * @param content 消息内容。
     * @return 消息创建时间文本。
     */
    std::string storePrivateMessage(int sender_id,
                                    int receiver_id,
                                    const std::string& content);

    /**
     * @brief 持久化群聊消息。
     * @param sender_id 发送者 ID。
     * @param group_id 群组 ID。
     * @param content 消息内容。
     * @return 消息创建时间文本。
     */
    std::string storeGroupMessage(int sender_id,
                                  int group_id,
                                  const std::string& content);

    /**
     * @brief 写入离线消息队列。
     * @param sender_id 发送者 ID。
     * @param receiver_id 接收者 ID。
     * @param msg_type 消息类型。
     * @param content 消息内容。
     * @param created_at 创建时间。
     * @return true 表示成功。
     */
    bool queueOfflineMessage(int sender_id,
                             int receiver_id,
                             uint32_t msg_type,
                             const std::string& content,
                             const std::string& created_at);

    /**
     * @brief 拉取接收者未投递离线消息。
     * @param receiver_id 接收者 ID。
     * @return 消息记录列表。
     */
    std::vector<ChatMessageRecord> fetchUndeliveredMessages(int receiver_id);
    /**
     * @brief 将接收者离线消息标记为已投递。
     * @param receiver_id 接收者 ID。
     * @return true 表示成功。
     */
    bool markMessagesDelivered(int receiver_id);

    /**
     * @brief 拉取双方私聊历史。
     * @param user_id 当前用户 ID。
     * @param peer_id 对端用户 ID。
     * @param limit 最多条数。
     * @return 历史消息列表。
     */
    std::vector<ChatMessageRecord> fetchPrivateHistory(int user_id,
                                                       int peer_id,
                                                       uint16_t limit = 100);

    /**
     * @brief 拉取群聊历史。
     * @param group_id 群组 ID。
     * @param limit 最多条数。
     * @return 历史消息列表。
     */
    std::vector<ChatMessageRecord> fetchGroupHistory(int group_id,
                                                     uint16_t limit = 100);

private:
    /// @brief 数据库连接池引用。
    Database& db_;
};

} // namespace cloudvault
