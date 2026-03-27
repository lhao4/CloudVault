// =============================================================
// server/include/server/db/friend_repository.h
// friend_relation 表的 CRUD 操作
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
};

/**
 * @brief 好友关系仓储。
 *
 * 封装 friend_relation 表相关的读写操作。
 */
class FriendRepository {
public:
    /**
     * @brief 构造仓储对象。
     * @param db 数据库连接池引用。
     */
    explicit FriendRepository(Database& db);

    /**
     * @brief 判断两人是否已是好友。
     * @param user_id 用户 ID。
     * @param friend_id 好友 ID。
     * @return true 表示已是好友。
     */
    bool areFriends(int user_id, int friend_id);
    /**
     * @brief 新增双向好友关系。
     * @param user_id 用户 ID。
     * @param friend_id 好友 ID。
     * @return true 表示成功。
     */
    bool addFriendPair(int user_id, int friend_id);
    /**
     * @brief 删除双向好友关系。
     * @param user_id 用户 ID。
     * @param friend_id 好友 ID。
     * @return true 表示成功。
     */
    bool removeFriendPair(int user_id, int friend_id);
    /**
     * @brief 列出用户全部好友。
     * @param user_id 用户 ID。
     * @return 好友列表。
     */
    std::vector<FriendInfo> listFriends(int user_id);

private:
    /// @brief 数据库连接池引用。
    Database& db_;
};

} // namespace cloudvault
