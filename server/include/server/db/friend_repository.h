// =============================================================
// server/include/server/db/friend_repository.h
// TODO（第九章）：好友关系表 CRUD 操作
//
// 操作（全部使用 Prepared Statements）：
//   - insertRequest()    发送好友请求（status=0）
//   - acceptRequest()    接受请求（status=1，双向插入）
//   - areFriends()       检查两个用户是否已是好友
//   - getAllFriends()     获取用户的完整好友列表
//   - getOnlineFriends() 获取在线好友列表
//   - remove()           删除好友关系（双向删除）
// =============================================================

#pragma once

// TODO（第九章）：实现 FriendRepository 类
