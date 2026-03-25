// =============================================================
// server/include/server/tcp_server.h
// TODO（第七章）：TCP 监听服务器
//
// 职责：
//   - 创建并绑定监听 socket（SO_REUSEADDR）
//   - 向 EventLoop 注册可读事件
//   - accept() 新连接，创建 TcpConnection 实例并注册到 EventLoop
//   - 支持 TLS（OpenSSL SSL_accept）
// =============================================================

#pragma once

// TODO（第七章）：实现 TcpServer 类
