// =============================================================
// server/include/server/db/friend_repository.h
// friend 表的 CRUD 操作
// =============================================================

#pragma once

#include "server/db/database.h"

#include <string>
#include <vector>

namespace cloudvault {

/**
 * @brief 好友记录模型。
 */
struct FriendInfo {
    /// @brief 好友用户 ID。
    int         user_id = 0;
    /// @brief 好友用户名。
    std::string username;
    /// @brief 好友昵称。
    std::string nickname;
    /// @brief 好友签名。
    std::string signature;
    /// @brief 在线状态。
    bool        is_online = false;
};

/**
 * @brief 好友关系仓储。
 *
 * 封装 friend 表相关的读写操作。
 * 好友关系以单向记录存储：A 添加 B → 插入 (A, B, status=0)。
 * 查询好友列表需用 UNION 查询。
 */
class FriendRepository {
public:
    /**
     * @brief 构造仓储对象。
     * @param db 数据库连接池引用。
     */
    explicit FriendRepository(Database& db);

    /**
     * @brief 判断两人是否已是好友（status=1）。
     * @param user_id 用户 ID。
     * @param friend_id 好友 ID。
     * @return true 表示已是好友。
     */
    bool areFriends(int user_id, int friend_id);

    /**
     * @brief 检查是否存在待处理的好友申请（status=0）。
     * @param from_id 申请发起方 ID。
     * @param to_id 申请目标方 ID。
     * @return true 表示存在待处理申请。
     */
    bool hasPendingRequest(int from_id, int to_id);

    /**
     * @brief 插入好友申请（status=0）。
     * @param from_id 发起方 ID。
     * @param to_id 目标方 ID。
     * @return true 表示成功。
     */
    bool insertRequest(int from_id, int to_id);

    /**
     * @brief 同意好友申请，将 status 更新为 1。
     * @param from_id 原申请发起方 ID。
     * @param to_id 原申请目标方 ID（即同意方）。
     * @return true 表示成功。
     */
    bool acceptRequest(int from_id, int to_id);

    /**
     * @brief 删除好友关系（双向删除）。
     * @param user_id 用户 ID。
     * @param friend_id 好友 ID。
     * @return true 表示成功。
     */
    bool removeFriend(int user_id, int friend_id);

    /**
     * @brief 列出用户全部已确认好友（status=1），含在线状态。
     * @param user_id 用户 ID。
     * @return 好友列表。
     */
    std::vector<FriendInfo> listFriends(int user_id);

private:
    /// @brief 数据库连接池引用。
    Database& db_;
};

} // namespace cloudvault
