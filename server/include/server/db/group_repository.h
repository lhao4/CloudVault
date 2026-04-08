// =============================================================
// server/include/server/db/group_repository.h
// chat_group / group_member 表的 CRUD 操作
// =============================================================

#pragma once

#include "server/db/database.h"

#include <optional>
#include <string>
#include <vector>

namespace cloudvault {

/// @brief 群组信息模型。
struct GroupInfo {
    int         id       = 0;
    std::string name;
    int         owner_id = 0;
};

/// @brief 群成员信息模型。
struct GroupMemberInfo {
    int         user_id   = 0;
    std::string username;
    bool        is_online = false;
};

/**
 * @brief 群组仓储。
 *
 * 封装 chat_group 和 group_member 表的读写操作。
 */
class GroupRepository {
public:
    explicit GroupRepository(Database& db);

    /**
     * @brief 创建群组并自动将群主加为成员。
     * @param name 群名称。
     * @param owner_id 群主用户 ID。
     * @return 成功返回 group_id(>0)，失败返回 0。
     */
    int createGroup(const std::string& name, int owner_id);

    /**
     * @brief 添加群成员。
     * @return true 表示成功。
     */
    bool addMember(int group_id, int user_id);

    /**
     * @brief 移除群成员。
     * @return true 表示成功。
     */
    bool removeMember(int group_id, int user_id);

    /**
     * @brief 删除群组（级联删除成员）。
     * @return true 表示成功。
     */
    bool deleteGroup(int group_id);

    /**
     * @brief 判断用户是否为群成员。
     */
    bool isMember(int group_id, int user_id);

    /**
     * @brief 查询群组信息。
     */
    std::optional<GroupInfo> findGroup(int group_id);

    /**
     * @brief 列出用户所在的全部群组。
     */
    std::vector<GroupInfo> listUserGroups(int user_id);

    /**
     * @brief 列出群的全部成员。
     */
    std::vector<GroupMemberInfo> listMembers(int group_id);

    /**
     * @brief 列出群的在线成员。
     */
    std::vector<GroupMemberInfo> listOnlineMembers(int group_id);

private:
    Database& db_;
};

} // namespace cloudvault
