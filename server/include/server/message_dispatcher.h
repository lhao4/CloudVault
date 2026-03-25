// =============================================================
// server/include/server/message_dispatcher.h
// TODO（第七章）：PDU 消息路由分发器
//
// 职责：
//   - 维护 MessageType → HandlerFn 路由表
//   - 解析收到的 PDU，根据 message_type 调用对应 Handler
//   - Handler 注册采用函数对象，支持灵活绑定（bind / lambda）
//   - 未知 MessageType 记录警告并回复错误响应
// =============================================================

#pragma once

// TODO（第七章）：实现 MessageDispatcher 类
