// =============================================================
// server/include/server/db/user_repository.h
// TODO（第八章）：用户表 CRUD 操作
//
// 操作（全部使用 Prepared Statements）：
//   - insert()        注册新用户（username + password_hash）
//   - findByName()    按用户名查询（返回 optional<UserInfo>）
//   - setOnline()     登录/登出时更新 online 字段
//   - getOnlineUsers() 查询所有在线用户列表
// =============================================================

#pragma once

// TODO（第八章）：实现 UserRepository 类
