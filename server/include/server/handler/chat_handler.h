// =============================================================
// server/include/server/handler/chat_handler.h
// TODO（第十章）：即时通讯业务逻辑
//
// 处理的 MessageType：
//   - CHAT           单聊（查目标是否在线 → 在线直发 / 离线存表）
//   - GROUP_CHAT     群聊（遍历在线成员广播 / 离线成员存表）
//   - GET_HISTORY    拉取聊天记录（分页查询 chat_message 表）
// =============================================================

#pragma once

// TODO（第十章）：实现 ChatHandler 类
