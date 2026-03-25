// =============================================================
// server/include/server/handler/auth_handler.h
// TODO（第八章）：用户认证业务逻辑
//
// 处理的 MessageType：
//   - REGISTER       注册（校验格式 → 检查重名 → hash 密码 → 入库 → 建目录）
//   - LOGIN          登录（查用户 → verify 密码 → 防重复登录 → 注册会话 → 下发离线消息）
//   - LOGOUT         登出（移除会话 → 更新 online 状态）
//   - UPDATE_PROFILE 修改资料（昵称、签名、头像）
// =============================================================

#pragma once

// TODO（第八章）：实现 AuthHandler 类
