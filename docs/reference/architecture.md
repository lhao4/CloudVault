# CloudVault 云巢 — 系统架构文档

> 版本：v2.1  
> 状态：与当前代码同步  
> 更新时间：2026-03-27

---

## 1. 总体架构

CloudVault 当前采用经典 C/S 架构：

- 客户端：Qt 6 Widgets，目标平台为 Windows
- 服务端：C++17 + epoll + MySQL，目标平台为 Linux
- 共享协议库：`common`
- 通信方式：自定义二进制 PDU 协议 over TCP 长连接

当前代码里尚未启用 TLS。文档中若出现 TLS，应视为后续规划，不属于现阶段实现。

```text
┌─────────────────────────────────────────────────────────────┐
│                      客户端（Windows）                     │
│                                                             │
│   UI Panel / Dialog  ◄──►  Service  ◄──►  Network          │
│   LoginWindow               AuthService       TcpClient      │
│   MainWindow                FriendService     ResponseRouter │
│   SidebarPanel              ChatService                         │
│   ChatPanel                 FileService                         │
│   FilePanel                 ShareService                        │
│   DetailPanel                                                │
│   ProfilePanel / Dialogs                                      │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               │ TCP 长连接 + PDU v2
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                       服务端（Linux）                      │
│                                                             │
│  EventLoop + TcpServer + TcpConnection                      │
│                 │                                           │
│                 ▼                                           │
│             ThreadPool                                      │
│                 │                                           │
│                 ▼                                           │
│         MessageDispatcher                                   │
│        ├── AuthHandler                                      │
│        ├── FriendHandler                                    │
│        ├── ChatHandler                                      │
│        ├── FileHandler                                      │
│        └── ShareHandler                                     │
│                 │                                           │
│   SessionManager / FileStorage / Database / Repository      │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
                           MySQL 数据库
```

---

## 2. 当前仓库结构

```text
CloudVault/
├── common/
│   ├── include/common/
│   │   ├── protocol.h
│   │   ├── protocol_codec.h
│   │   └── crypto_utils.h
│   ├── src/
│   └── tests/
├── server/
│   ├── config/
│   │   └── server.example.json
│   ├── include/server/
│   │   ├── server_app.h
│   │   ├── event_loop.h
│   │   ├── tcp_server.h
│   │   ├── tcp_connection.h
│   │   ├── thread_pool.h
│   │   ├── session_manager.h
│   │   ├── message_dispatcher.h
│   │   ├── file_storage.h
│   │   ├── db/
│   │   │   ├── database.h
│   │   │   ├── user_repository.h
│   │   │   ├── friend_repository.h
│   │   │   └── chat_repository.h
│   │   └── handler/
│   │       ├── auth_handler.h
│   │       ├── friend_handler.h
│   │       ├── chat_handler.h
│   │       ├── file_handler.h
│   │       └── share_handler.h
│   ├── src/
│   ├── sql/
│   └── tests/
├── client/
│   ├── config/
│   │   └── client.example.json
│   ├── resources/
│   │   ├── icons/app_icon.png
│   │   ├── styles/style.qss
│   │   └── resources.qrc
│   └── src/
│       ├── main.cpp
│       ├── app.h / app.cpp
│       ├── network/
│       │   ├── tcp_client.*
│       │   └── response_router.*
│       ├── service/
│       │   ├── auth_service.*
│       │   ├── friend_service.*
│       │   ├── chat_service.*
│       │   ├── file_service.*
│       │   └── share_service.*
│       └── ui/
│           ├── login_window.*
│           ├── main_window.*
│           ├── sidebar_panel.*
│           ├── chat_panel.*
│           ├── file_panel.*
│           ├── detail_panel.*
│           ├── profile_panel.*
│           ├── online_user_dialog.*
│           ├── share_file_dialog.*
│           └── group_list_dialog.*
└── docs/
```

说明：

- 当前仓库没有顶层 `CMakeLists.txt`
- `common`、`server`、`client` 独立构建
- 这符合“服务端在 Linux 构建，客户端在 Windows 构建”的现阶段目标

---

## 3. 共享协议库 `common`

### 3.1 职责

| 模块 | 当前职责 |
|------|----------|
| `protocol.h` | 消息类型枚举、PDU 头部、协议常量 |
| `protocol_codec.h` | PDU 构建和拆包 |
| `crypto_utils.h` | SHA-256 + salt 哈希与校验 |

### 3.2 协议现状

- PDU 头部固定 16 字节
- 所有多字节字段使用网络字节序
- 文件分片常量、消息类型、头部定义目前统一放在 `protocol.h`
- 当前并未拆分为 `message_types.h`、`constants.h` 等多个头文件

### 3.3 当前消息类型覆盖

当前代码已覆盖：

- 认证：注册、登录、登出
- 好友：查找、申请、同意、删除、刷新好友列表
- 聊天：单聊、历史消息
- 文件管理：列表、建目录、重命名、移动、删除、搜索
- 文件传输：上传、下载
- 文件分享：分享请求、分享接受

`GROUP_CHAT` 仅保留协议枚举，当前客户端/服务端还没有完整群聊业务链路。

---

## 4. 服务端架构

### 4.1 设计原则

- 服务端不依赖 Qt
- 主线程负责 I/O，多线程负责业务处理
- Handler 聚焦协议解析和业务编排
- Repository 聚焦 MySQL 访问
- 文件系统操作统一收敛到 `FileStorage`

### 4.2 核心模块

| 模块 | 当前职责 |
|------|----------|
| `ServerApp` | 启动入口，加载 JSON 配置，初始化日志、数据库、文件存储、网络层和各 Handler |
| `EventLoop` | epoll 事件循环 |
| `TcpServer` | 监听端口并接收新连接 |
| `TcpConnection` | 单连接收发、PDU 拆包、关闭回调 |
| `ThreadPool` | 工作线程池 |
| `MessageDispatcher` | 按 `MessageType` 分发消息 |
| `SessionManager` | 在线会话管理 |
| `Database` | MySQL 连接池，RAII 借还连接 |
| `UserRepository` | 用户认证、在线状态等 |
| `FriendRepository` | 好友关系和好友请求 |
| `ChatRepository` | 历史消息持久化 |
| `FileStorage` | 物理文件和目录操作 |
| `AuthHandler` | 注册、登录、登出 |
| `FriendHandler` | 好友查找、申请、同意、删除、刷新 |
| `ChatHandler` | 单聊消息、历史消息 |
| `FileHandler` | 文件管理、上传、下载 |
| `ShareHandler` | 文件分享与接收 |

### 4.3 启动流程

`ServerApp::init()` 当前负责：

1. 读取 `server.example.json` 风格的配置文件
2. 初始化日志
3. 创建 `Database` 连接池
4. 创建 `FileStorage`
5. 创建 `EventLoop`、`ThreadPool`、`TcpServer`
6. 创建各类 Handler
7. 注册消息路由到 `MessageDispatcher`

当前没有独立的 `Config` 类，配置解析直接在 `ServerApp` 中完成。

### 4.4 请求处理链路

```text
客户端发包
  ↓
TcpConnection::onReadable()
  ↓
PDU 完整后提交到 ThreadPool
  ↓
MessageDispatcher::dispatch()
  ↓
具体 Handler
  ├── Repository / Database
  ├── SessionManager
  └── FileStorage
  ↓
构建响应 PDU
  ↓
TcpConnection::send()
  ↓
EventLoop 可写事件发送
```

### 4.5 线程模型

| 线程 | 职责 |
|------|------|
| 主线程 | accept、读 socket、写 socket、epoll 调度 |
| 工作线程池 | 数据库查询、文件操作、业务处理 |

关键约束：

- 工作线程不直接操作 epoll
- 响应通过 `TcpConnection` 的线程安全发送路径回到主线程输出

### 4.6 连接与会话

- 登录成功后，`SessionManager` 记录用户与连接映射
- 连接关闭时，服务端会清理会话并回收上传上下文
- 显式 `LOGOUT` 也会同步更新数据库在线状态

### 4.7 当前实现状态

已落地：

- 聊天与文件相关路由均已注册
- 上传/下载链路已接通
- `Database::Connection` 已改为只可移动，不可拷贝
- MySQL deprecated reconnect 警告已移除

仍待完善：

- TLS 传输层
- 更完整的服务端业务测试覆盖
- 群聊后端链路

---

## 5. 客户端架构

### 5.1 分层

当前客户端已经收敛为：

```text
UI 层
  ├── LoginWindow
  ├── MainWindow
  ├── SidebarPanel / ChatPanel / FilePanel / DetailPanel / ProfilePanel
  └── OnlineUserDialog / ShareFileDialog / GroupListDialog

Service 层
  ├── AuthService
  ├── FriendService
  ├── ChatService
  ├── FileService
  └── ShareService

Network 层
  ├── TcpClient
  └── ResponseRouter
```

### 5.2 `App` 装配层

当前客户端已有明确的应用装配入口 `App`：

- 创建 `TcpClient`
- 创建 `ResponseRouter`
- 创建全部 Service
- 创建 `LoginWindow`
- 登录成功后创建 `MainWindow`
- 监听 socket 连接状态
- 负责断线横幅和自动重连触发

也就是说，`LoginWindow` 已不再承担应用装配职责。

### 5.3 `LoginWindow`

职责：

- 服务器配置读取
- 连接服务器
- 登录/注册输入与校验
- 注册响应处理、登录响应处理
- 登录成功后发出 `loginSucceeded`

当前界面保留“昵称”输入框，但注册请求实际仍只提交用户名和密码；昵称尚未进入协议和后端存储。

### 5.4 `MainWindow`

当前 `MainWindow` 已经从“大而全页面类”收敛成壳层协调器，主要负责：

- 顶层布局和底部导航
- 三个主 Tab 切换
- 各 Service 信号连接
- 跨面板状态同步
- 上传/下载/分享等主业务动作
- 连接横幅与事件日志

### 5.5 主窗口面板拆分

| 面板 | 当前职责 |
|------|----------|
| `SidebarPanel` | 左侧栏标题、搜索、联系人列表、选择态 |
| `ChatPanel` | 聊天中栏、消息列表绘制、空态、群聊占位、输入框与发送动作 |
| `FilePanel` | 文件中栏、路径栏、搜索、文件列表、底部操作栏、传输进度 |
| `DetailPanel` | 右侧联系人详情和共享文件摘要 |
| `ProfilePanel` | “我”页个人信息卡片、保存/取消/退出登录 |

### 5.6 客户端流程

```text
应用启动
  ↓
App 创建 TcpClient / Router / Service / LoginWindow
  ↓
用户登录成功
  ↓
App 创建 MainWindow
  ↓
MainWindow 触发刷新好友列表
  ↓
用户在消息/文件/我 三个主 Tab 间切换
```

### 5.7 网络状态反馈

当前客户端已经实现：

- 顶部断线横幅
- 事件日志栏
- 自动重连尝试

这些逻辑由 `App` 与 `MainWindow` 协作完成，而不是散落在多个窗口里。

### 5.8 当前 UI 状态

已实现：

- 三栏主界面
- 聊天气泡绘制
- 文件管理与上传下载
- 右侧联系人详情
- 个人信息页
- 在线用户弹窗
- 分享文件弹窗
- 群组列表弹窗

占位或未完成：

- 群聊消息流仅有界面占位，暂无完整业务链路
- 客户端自动化测试尚未接入

---

## 6. 当前架构与旧文档差异

下面这些内容以当前代码为准：

- 无 TLS
- 客户端已有 `App`
- `service/` 已从 `network/` 中拆出
- 主界面已拆分为多个 Panel，而不是把所有 UI 塞进 `MainWindow`
- “我”页当前是 `ProfilePanel`，不是 `ProfileDialog`
- `MessageDispatcher` 位于 `server/include/server/message_dispatcher.h`
- 服务端配置解析位于 `ServerApp`，不是独立 `Config` 类

---

## 7. 后续建议

如果下一阶段继续向更完整工程化靠拢，建议优先做：

1. 群聊协议与后端实现
2. 客户端昵称字段真正入协议、入库
3. 为服务端 Handler 引入更可测试的注入 seam
4. 增加 Linux 服务端 CI 构建与测试
5. 如有安全要求，再补 TLS 传输层
