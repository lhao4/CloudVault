# 第二章　技术选型与架构概览

> **本章目标**：搞清楚用什么技术、为什么这么选、整体架构长什么样。
>
> **本章不写代码**，细节实现在后续各章节中逐步展开。

---

## 2.1 技术选型

### 服务端

| 技术 | 选型 | 理由 |
|------|------|------|
| 语言 | C++17 | 标准库支持 `std::filesystem`、`std::optional`、结构化绑定等现代特性 |
| 网络 | POSIX epoll | Linux 下最高效的 I/O 多路复用，支撑高并发连接 |
| 线程 | `std::thread` + 线程池 | 标准库，无需第三方依赖 |
| 数据库 | MySQL 8.0 + libmysqlclient | 成熟可靠，C API 直接使用 |
| 日志 | spdlog | 高性能，支持异步日志，格式清晰 |
| 配置 | nlohmann/json | header-only，使用简单 |
| 加密 | OpenSSL | SHA-256 哈希 + TLS 加密 |
| 构建 | CMake 3.20+ | 业界标准，跨平台 |

### 客户端

| 技术 | 选型 | 理由 |
|------|------|------|
| 语言 | C++17 + Qt 6 | Qt 提供完整的 GUI 和网络库，跨平台 |
| 网络 | QTcpSocket / QSslSocket | Qt 封装好的 TCP 客户端，与信号槽天然集成 |
| 构建 | CMake 3.20+ + Qt 自动化工具 | 取代 qmake，与服务端统一构建体系 |

### 共享层（common）

| 技术 | 选型 | 理由 |
|------|------|------|
| 协议 | 自定义二进制 PDU | 轻量、高效，控制字段精确 |
| 密码 | OpenSSL SHA-256 | 服务端和客户端用同一套哈希逻辑 |

---

## 2.2 整体架构

```
客户端（Windows）
┌─────────────────────────────────┐
│  UI 层 (Qt Widgets)              │
│  LoginWindow / MainWindow / ...  │
│           ↕ Qt 信号槽             │
│  Service 层                      │
│  AuthService / ChatService / ... │
│           ↕ sendPDU / 回调        │
│  Network 层                      │
│  TcpClient + ResponseRouter      │
└────────────────┬────────────────┘
                 │ TCP 长连接（TLS）
                 │ 自定义二进制协议 PDU v2
┌────────────────┴────────────────┐
│  服务端（Linux）                  │
│  主线程 EventLoop（epoll）        │
│  ├── TcpServer：accept 新连接     │
│  └── TcpConnection[]：读写缓冲    │
│           ↓ 完整 PDU              │
│  ThreadPool（工作线程 ×8）         │
│  └── MessageDispatcher           │
│       ├── AuthHandler   → DB     │
│       ├── FriendHandler → DB     │
│       ├── ChatHandler   → Session│
│       └── FileHandler   → 文件系统│
└────────────────┬────────────────┘
                 │
          MySQL 8.0 数据库
```

---

## 2.3 三个关键设计决策

### 决策一：服务端脱离 Qt

v1 服务端用 Qt 的 `QTcpServer`，导致部署 Linux 服务器时需要安装完整 Qt 环境。

v2 服务端改为纯 C++ + POSIX API，只依赖：`libmysqlclient`、`spdlog`、`nlohmann/json`、`OpenSSL`，全部可以通过 apt 安装，镜像体积大幅减小。

### 决策二：common 静态库

协议代码（PDU 编解码 + 密码哈希）不在两端各写一份，而是提取到 `common/` 静态库，服务端和客户端各自链接。

好处：
- 改一处，两端同步更新，不会出现协议版本不一致
- 可以对 common 单独写单元测试

### 决策三：客户端三层分离

v1 的 UI 代码直接操作 PDU、直接调用网络，耦合严重。

v2 严格分三层：

```
UI 层     —— 只负责显示和响应用户操作
Service 层 —— 封装业务语义，构建/解析 PDU，emit Qt 信号
Network 层 —— 只负责字节收发和 PDU 拆包
```

UI 层不知道有网络的存在，Network 层不知道业务逻辑，各层可以独立测试。

---

## 2.4 本章边界说明

本章只给出高层结论，以下细节在对应章节展开：

- PDU 字段格式 → 第六章
- epoll 工作原理 → 第七章
- 数据库表结构 → 第八至十三章（随功能逐步引入）
- 密码安全链路 → 第八章
- 路径安全实现 → 第十一章

---

## 2.5 本章小结

- [x] 明确服务端和客户端的技术栈
- [x] 理解整体 C/S 架构
- [x] 理解三个核心设计决策的原因

下一章：[第三章 — 开发环境搭建](ch03-environment.md)
