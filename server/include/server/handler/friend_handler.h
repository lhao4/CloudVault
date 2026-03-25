// =============================================================
// server/include/server/handler/friend_handler.h
// TODO（第九章）：好友系统业务逻辑
//
// 处理的 MessageType：
//   - FIND_USER      查找用户（返回用户名、昵称、在线状态）
//   - ADD_FRIEND     发送好友请求（转发给在线目标 / 存离线消息）
//   - AGREE_FRIEND   接受好友请求（双向入库）
//   - FLUSH_FRIENDS  获取完整好友列表（含在线状态）
//   - DELETE_FRIEND  删除好友（双向删除，通知对方）
// =============================================================

#pragma once

// TODO（第九章）：实现 FriendHandler 类
