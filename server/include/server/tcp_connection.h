// =============================================================
// server/include/server/tcp_connection.h
// TODO（第七章）：单个 TCP 连接的状态管理
//
// 职责：
//   - 持有 fd、SSL*（TLS）、对端地址
//   - 接收缓冲区 + PDUParser（处理粘包/半包）
//   - 发送队列（mutex 保护，工作线程可安全入队）
//   - 连接关闭时通知 SessionManager 清理会话
// =============================================================

#pragma once

// TODO（第七章）：实现 TcpConnection 类
