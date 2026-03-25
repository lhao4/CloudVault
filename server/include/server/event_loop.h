// =============================================================
// server/include/server/event_loop.h
// TODO（第七章）：epoll 事件循环封装
//
// 职责：
//   - 封装 epoll_create / epoll_ctl / epoll_wait
//   - 管理注册的 fd 和对应的回调函数
//   - 主线程唯一入口：run() 阻塞在 epoll_wait，处理所有 I/O 事件
//   - 支持通过 eventfd 从工作线程安全唤醒主线程
// =============================================================

#pragma once

// TODO（第七章）：实现 EventLoop 类
