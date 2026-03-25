# 第七章　建立连接——PING/PONG 联通

> **状态**：🔲 待写
>
> **本章目标**：客户端连接服务端，发送 `PING`，收到 `PONG`，两端日志可见。
>
> **验收标准**：
> - 服务端日志：`[info] Received PING from fd=5`
> - 客户端日志：`[debug] Received PONG`

---

## 本章大纲

### 7.1 服务端 TCP 层

需要实现的类：

- `EventLoop`：封装 epoll，管理 fd 的注册/注销，提供 `run()` 阻塞循环
- `TcpServer`：监听端口，accept 新连接，创建 `TcpConnection`
- `TcpConnection`：管理单个连接的收发缓冲区、PDU 拆包
- `MessageDispatcher`：注册消息类型到处理函数的映射

本章只注册一个处理函数：收到 `PING` → 回复 `PONG`。

### 7.2 epoll 工作流程

```
EventLoop::run()
  └── epoll_wait()  ← 阻塞，等待事件
        │
        ├── EPOLLIN（可读）
        │    ├── listen_fd → TcpServer::onAccept()
        │    └── conn_fd   → TcpConnection::onReadable()
        │         └── feed() → tryParse() → dispatcher_.dispatch()
        │
        └── EPOLLOUT（可写）
             └── TcpConnection::flushSendQueue()
```

详细原理：[epoll 工作原理图解](knowledge/epoll-guide.md)

### 7.3 客户端 TCP 层

需要实现的类：

- `TcpClient`：封装 `QTcpSocket`，提供 `connectToServer()`、`sendPDU()`，收到数据时 emit `pduReceived` 信号
- `ResponseRouter`：按 `MessageType` 分发响应到对应的 Service 回调

本章 `LoginWindow` 连接成功后自动发送 `PING`，收到 `PONG` 后在日志输出。

### 7.4 线程模型说明

服务端采用 **Reactor 模式**：

```
主线程（EventLoop）  ← 只做 I/O，不做业务
工作线程池           ← 只做业务，不做 I/O
```

工作线程完成业务后，将响应 PDU 放入 `TcpConnection` 的发送队列，
通过 `eventfd` 通知主线程，由主线程负责实际 `write()`。

### 7.5 本章新知识点

- [epoll 工作原理图解](knowledge/epoll-guide.md)
- Reactor 模式
- `eventfd` 跨线程通知

---

上一章：[第六章 — 协议基础](ch06-protocol.md) ｜ 下一章：[第八章 — 注册与登录](ch08-auth.md)
