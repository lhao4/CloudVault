// =============================================================
// server/include/server/db/friend_repository.h
// friend_relation 表的 CRUD 操作
// =============================================================

#pragma once

#include "server/db/database.h"

#include <string>
#include <vector>

namespace cloudvault {

struct FriendInfo {
    int         user_id = 0;
    std::string username;
};

class FriendRepository {
public:
    explicit FriendRepository(Database& db);

    bool areFriends(int user_id, int friend_id);
    bool addFriendPair(int user_id, int friend_id);
    bool removeFriendPair(int user_id, int friend_id);
    std::vector<FriendInfo> listFriends(int user_id);

private:
    Database& db_;
};

} // namespace cloudvault
