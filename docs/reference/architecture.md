# CloudVault 云巢 — 系统架构文档

> **版本**：v2.0 | **状态**：已确认

---

## 1. 整体架构概览

CloudVault 采用经典 **C/S（客户端/服务端）** 架构，客户端与服务端通过 TCP 长连接通信，共用同一套二进制协议库（`common`）。

```
┌─────────────────────────────────────────────────────────────┐
│                        客户端（Windows）                      │
│                                                             │
│   ┌──────────┐    ┌──────────────┐    ┌─────────────────┐  │
│   │  UI 层   │◄──►│  Service 层  │◄──►│  Network 层     │  │
│   │ Qt Widgets│    │ 业务逻辑/信号 │    │ TcpClient/Router│  │
│   └──────────┘    └──────────────┘    └────────┬────────┘  │
│                                                │            │
│                         ┌──────────────────────┘            │
│                         │  共享 common 库                   │
│                         │  PDU v2 编解码 / 密码哈希          │
└─────────────────────────┼────────────────────────────────────┘
                          │  TCP 长连接（TLS 加密）
                          │  自定义二进制协议 PDU v2
┌─────────────────────────┼────────────────────────────────────┐
│                         │  共享 common 库                   │
│                         │  PDU v2 编解码 / 密码哈希          │
│                         │                                   │
│   ┌─────────────────────▼──────────────────────────────┐   │
│   │                   服务端（Linux）                    │   │
│   │                                                    │   │
│   │  主线程 EventLoop (epoll)                           │   │
│   │  ├── TcpServer: accept 新连接                       │   │
│   │  ├── TcpConnection[]: 读写缓冲区 + PDU 拆包         │   │
│   │  └── 完整 PDU → 提交给 ThreadPool                   │   │
│   │                      ↓                             │   │
│   │  工作线程池 (std::thread × 8)                       │   │
│   │  └── MessageDispatcher                             │   │
│   │       ├── AuthHandler    ──► UserRepository        │   │
│   │       ├── FriendHandler  ──► FriendRepository      │   │
│   │       ├── ChatHandler    ──► SessionManager        │   │
│   │       └── FileHandler    ──► FileStorage           │   │
│   │                                                    │   │
│   │  SessionManager: username ↔ TcpConnection 映射      │   │
│   │  Database: MySQL 连接池                             │   │
│   └────────────────────────────────────────────────────┘   │
│                        服务端（Linux）                       │
└─────────────────────────────────────────────────────────────┘
                          │
               ┌──────────▼──────────┐
               │   MySQL 数据库       │
               │  user_info / friend  │
               │  chat_message 等     │
               └─────────────────────┘
```

---

## 2. 服务端架构

### 2.1 整体设计原则

- **零 Qt 依赖**：服务端仅使用 C++17 标准库、POSIX API、libmysqlclient、spdlog、nlohmann/json
- **主线程单 epoll，工作线程处理业务**：I/O 与业务逻辑分离，避免阻塞事件循环
- **无状态 Handler**：每个 Handler 方法无成员变量，依赖注入 Repository 和 SessionManager

### 2.2 服务端模块划分

```
server/
├── ServerApp          — 应用生命周期（初始化 / 运行 / 关闭）
├── Config             — JSON 配置加载与校验
├── EventLoop          — epoll 封装（add/modify/remove fd，run 循环）
├── TcpServer          — 监听 socket，accept 新连接，创建 TcpConnection
├── TcpConnection      — 单连接状态（fd、接收缓冲区、发送队列、会话信息）
├── ThreadPool         — 固定大小线程池，submit(std::function<void()>)
├── SessionManager     — 在线会话的线程安全映射（shared_mutex）
├── db/
│   ├── Database       — MySQL 连接池（acquire / release）
│   ├── UserRepository — 用户相关预处理语句
│   └── FriendRepository — 好友相关预处理语句
├── handler/
│   ├── MessageDispatcher — 按 MessageType 路由到具体 Handler
│   ├── AuthHandler    — 注册、登录
│   ├── FriendHandler  — 好友增删查
│   ├── ChatHandler    — 消息转发、离线消息
│   └── FileHandler    — 文件全部操作
└── FileStorage        — std::filesystem 封装（路径校验 + 操作）
```

### 2.3 请求处理流程

```
客户端发送数据
      │
      ▼
EventLoop::epoll_wait()  ← 主线程，非阻塞
      │
      ▼
TcpConnection::onReadable()
  └── 数据追加到 recv_buffer
  └── 循环尝试解析完整 PDU（total_length 字段）
  └── PDU 完整 → 拷贝到堆 → submit 到 ThreadPool
                                   │
                                   ▼
                           ThreadPool 工作线程
                           MessageDispatcher::dispatch(pdu, conn)
                                   │
                           按 MessageType 路由
                                   │
                           具体 Handler::handle()
                           ├── 调用 Repository（DB 预处理语句）
                           ├── 调用 FileStorage（文件操作）
                           └── 调用 SessionManager（消息转发）
                                   │
                           构建响应 PDU
                                   │
                           TcpConnection::send(pdu)
                           └── 放入发送队列
                           └── 通知 EventLoop 注册 EPOLLOUT
                                   │
                                   ▼
                           EventLoop::onWritable() ← 主线程
                           └── 从发送队列取数据 write() 发送
```

### 2.4 线程模型

| 线程 | 数量 | 职责 |
|------|------|------|
| 主线程（EventLoop） | 1 | accept、read、write，不做任何阻塞操作 |
| 工作线程（ThreadPool） | 8（可配置） | 执行 Handler 业务逻辑、DB 查询、文件 I/O |

**关键约束**：
- 工作线程不直接 write socket，只将响应 PDU 放入 `TcpConnection` 的线程安全发送队列
- 主线程通过 `eventfd` 或 `EPOLLOUT` 感知到有数据待发送后执行实际 write

### 2.5 SessionManager

```cpp
class SessionManager {
    std::shared_mutex mutex_;
    std::unordered_map<std::string, TcpConnection*> name_to_conn_;
    std::unordered_map<int, std::string> fd_to_name_;
public:
    void     registerSession(const std::string& name, TcpConnection* conn);
    void     unregisterSession(int fd);
    TcpConnection* findByName(const std::string& name);  // nullptr = 离线
    std::vector<std::string> getOnlineUsers();
};
```

- 登录成功后调用 `registerSession`
- 连接断开（EPOLLRDHUP）后调用 `unregisterSession` 并更新 DB online=0
- 聊天转发、好友请求转发均通过 `findByName` 获取目标连接后直接发送

### 2.6 数据库连接池

- 初始化时创建 N 个 `MYSQL*` 连接（N = 线程池大小，默认 8）
- 工作线程通过 `DbConnection guard = db.acquire()` RAII 方式借用连接
- 连接断开时自动重连（`mysql_ping` 检测）
- 所有 SQL 均使用预处理语句（`mysql_stmt_*`），杜绝 SQL 注入

---

## 3. 客户端架构

### 3.1 整体设计原则

- **三层分离**：UI 层只负责显示和用户交互，Service 层封装业务语义，Network 层处理传输
- **Qt 信号槽驱动**：Service 收到服务端响应后 emit 信号，UI 通过 connect 订阅更新
- **无全局单例**：对象通过构造函数参数注入，取代原 `Client::getInstance()` 等单例

### 3.2 客户端模块划分

```
client/
├── App               — 应用启动，创建并注入所有对象
├── network/
│   ├── TcpClient     — QTcpSocket 封装，PDU 收发缓冲，sendPDU()
│   └── ResponseRouter — 按 MessageType dispatch 到 Service 的回调
├── service/
│   ├── AuthService   — 构建注册/登录 PDU，解析响应，emit 信号
│   ├── FriendService — 好友相关请求/响应
│   ├── ChatService   — 聊天相关请求/响应
│   └── FileService   — 文件相关请求/响应
└── ui/
    ├── LoginWindow   — 登录/注册界面（原 Client）
    ├── MainWindow    — 主界面 Tab（原 Index）
    ├── FriendWidget  — 好友管理面板（原 Friend）
    ├── FileWidget    — 文件管理面板（原 File）
    ├── ChatWindow    — 聊天窗口（原 Chat）
    ├── OnlineUserDialog — 在线用户列表（原 OnlineUser）
    └── ShareFileDialog  — 文件分享选择（原 ShareFile）
```

### 3.3 数据流示例（以登录为例）

```
用户点击"登录"按钮
      │
      ▼
LoginWindow::onLoginClicked()
  └── authService->login(username, password)
            │
            ▼
      AuthService::login()
      └── 构建 LOGIN_REQUEST PDU（密码客户端先哈希）
      └── tcpClient->sendPDU(pdu)
                          │
                    TCP 发送到服务端
                          │
                    服务端处理后返回 LOGIN_RESPOND
                          │
                          ▼
      TcpClient::onDataReady()  ← QTcpSocket::readyRead 信号
      └── 拼接 recv_buffer，尝试解析完整 PDU
      └── PDU 完整 → ResponseRouter::dispatch(pdu)
                          │
                          ▼
      AuthService::onLoginResponse(pdu)
      └── 解析 status_code
      └── emit loginSucceeded(username)   或 emit loginFailed(reason)
                          │
                          ▼
      LoginWindow::onLoginSucceeded(username)
      └── 隐藏登录窗口
      └── 创建 MainWindow 并显示
```

### 3.4 对象生命周期与注入

```cpp
// App::init()
auto tcpClient     = std::make_unique<TcpClient>();
auto router        = std::make_unique<ResponseRouter>();
auto authService   = std::make_unique<AuthService>(tcpClient.get(), router.get());
auto friendService = std::make_unique<FriendService>(tcpClient.get(), router.get());
auto chatService   = std::make_unique<ChatService>(tcpClient.get(), router.get());
auto fileService   = std::make_unique<FileService>(tcpClient.get(), router.get());

auto loginWindow   = std::make_unique<LoginWindow>(authService.get());
// 登录成功后 LoginWindow 创建 MainWindow，传入各 service 指针
```

---

## 4. 共享协议库（common）

### 4.1 职责

| 模块 | 内容 |
|------|------|
| `message_types.h` | `enum class MessageType : uint32_t`，定义全部消息类型 |
| `protocol.h` | `PDUHeader` 结构体定义（16 字节固定头） |
| `protocol_codec.h` | `PDUBuilder`（构建 PDU）、`PDUParser`（拆包检测） |
| `constants.h` | `MAX_NAME_LEN`、`CHUNK_SIZE`、协议版本号等共享常量 |
| `crypto_utils.h` | SHA-256 + salt 哈希、哈希验证 |

### 4.2 PDU v2 格式

```
┌─────────────────────────────────────────────────────┐
│                   PDU Header（16 字节）               │
│  total_length : uint32_t  — 整包字节数（含头部）      │
│  body_length  : uint32_t  — body 区字节数             │
│  message_type : uint32_t  — MessageType 枚举值        │
│  reserved     : uint32_t  — 预留（版本/标志位）        │
├─────────────────────────────────────────────────────┤
│                   Body（变长）                        │
│  各消息类型自定义格式，见 api.md                       │
└─────────────────────────────────────────────────────┘
```

- 取消原 `caData[64]` 固定区，所有参数统一放入 body
- body 起始 4 字节为 `uint32_t status_code`（仅响应消息）
- 字符串字段均以固定长度（`MAX_NAME_LEN = 32`）或 `length + data` 形式编码

### 4.3 依赖关系

```
common（静态库，无第三方依赖）
  ├── 被 server 链接
  └── 被 client 链接

server 依赖：common, libmysqlclient, spdlog, nlohmann/json, pthread, OpenSSL
client 依赖：common, Qt6::Widgets, Qt6::Network, Qt6::Sql（可选）
```

---

## 5. 关键业务数据流

### 5.1 文件上传（普通文件）

```
客户端                                        服务端
   │                                            │
   ├─ UPLOAD_FILE_REQUEST ──────────────────►  │
   │  body: [filename][filesize][dir_path]      │ FileHandler::handleUploadFile()
   │                                            │ ├── 校验路径（防遍历）
   │                                            │ └── 打开目标文件（ofstream）
   │  ◄────────────────── UPLOAD_FILE_RESPOND ─┤
   │  body: [status_code]                       │
   │                                            │
   ├─ UPLOAD_FILE_DATA ─────────────────────►  │
   │  body: [chunk_data(≤4MB)]                  │ FileHandler::handleUploadFileData()
   │  （循环发送直到文件结束）                    │ └── 写入文件，累计接收字节
   │                                            │
   ├─ UPLOAD_FILE_DATA ─────────────────────►  │
   │  ...                                       │ 接收完毕 → 关闭文件
   │                                            │
   │  ◄──────────────── UPLOAD_FILE_DATA_RESP ─┤
   │  body: [status_code]                       │
```

### 5.2 聊天消息转发（含离线处理）

```
发送方                        服务端                         接收方
   │                            │                              │
   ├─ CHAT_REQUEST ──────────► │                              │
   │  body:[sender][target][msg]│                              │
   │                            │ ChatHandler::handleChat()    │
   │                            │ SessionManager::findByName() │
   │                            │                              │
   │                            │ [在线] ─────────────────────►│
   │                            │        CHAT_RESPOND          │
   │                            │        body:[sender][msg]    │
   │                            │                              │
   │                            │ [离线] → 写入 offline_message│
   │                            │          表，待对方登录时投递  │
```

### 5.3 好友请求流程

```
发起方                        服务端                          目标方
   │                            │                              │
   ├─ ADD_FRIEND_REQUEST ──────►│                              │
   │  body:[me][target]         │                              │
   │                            │ FriendHandler::handleAddFriend()
   │                            │ ├── 检查是否已是好友          │
   │                            │ ├── 检查目标是否在线          │
   │                            │ └── [在线] ─────────────────►│
   │                            │    转发 ADD_FRIEND_REQUEST   │
   │  ◄── ADD_FRIEND_RESPOND ───│                              │
   │                            │                              │
   │                            │         ◄── AGREE_ADD_FRIEND_REQUEST
   │                            │                              │
   │                            │ FriendHandler::handleAgreeAddFriend()
   │                            │ └── DB 插入好友记录           │
   │  ◄── AGREE_ADD_FRIEND_RESP─│                              │
   │  （通知发起方添加成功）       │   ─ AGREE_ADD_FRIEND_RESP ──►│
```

---

## 6. 安全架构

### 6.1 密码安全链路

```
客户端输入明文密码
      │
      ▼ crypto_utils::hash(password)  （common 库，客户端执行）
      │ SHA-256(random_salt + password)
      ▼
TLS 加密传输 → 服务端收到 password_hash
      │
      ▼ UserRepository::verifyPassword()
      │ 取出 DB 中的 stored_hash
      │ 重新计算 SHA-256(stored_salt + received_hash)
      │ 时序安全比较（防时序攻击）
      ▼
登录结果
```

### 6.2 路径遍历防护（FileStorage）

```cpp
// 所有文件操作前调用
std::filesystem::path FileStorage::safePath(
    const std::string& user_root,
    const std::string& relative_path)
{
    auto canonical = std::filesystem::weakly_canonical(user_root / relative_path);
    auto root      = std::filesystem::weakly_canonical(user_root);
    // 确保结果路径以 user_root 为前缀
    if (!canonical.string().starts_with(root.string())) {
        throw SecurityException("Path traversal detected");
    }
    return canonical;
}
```

### 6.3 TLS 配置

- 服务端：OpenSSL `SSL_CTX`，加载证书和私钥（路径配置在 `server.json`）
- 客户端：`QSslSocket`，可配置是否验证服务端证书（开发环境可关闭，生产环境开启）

---

## 7. 构建系统

### 7.1 CMake 结构

客户端（Windows）和服务端（Linux）**各自独立构建**，没有统一顶层 CMakeLists.txt。
两者通过相对路径共享 `common/` 静态库。

```
CloudVault/
├── common/
│   └── CMakeLists.txt          → 静态库 cloudvault_common
│                                 （两端各自 add_subdirectory 引入）
├── server/
│   └── CMakeLists.txt          → 可执行文件 cloudvault_server
│                                 add_subdirectory(../common)
│                                 add_subdirectory(tests)  [-DBUILD_TESTING=ON]
└── client/
    └── CMakeLists.txt          → 可执行文件 cloudvault_client
                                  add_subdirectory(../common)
```

**构建方式：**

```bash
# 服务端（Linux）
cd server
cmake -B build -G Ninja
cmake --build build

# 客户端（Windows，需已安装 Qt 6）
cd client
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### 7.2 第三方依赖

| 库 | 版本 | 引入方式 | 用于 |
|----|------|---------|------|
| spdlog | ≥1.11 | FetchContent | server — 日志 |
| nlohmann/json | ≥3.11 | FetchContent | server — 配置文件解析 |
| OpenSSL | ≥3.0 | find_package | server + common — TLS、SHA-256 |
| libmysqlclient | ≥8.0 | find_package（自定义） | server — MySQL 连接 |
| Google Test | ≥1.14 | FetchContent | server/tests — 单元/集成测试 |
| Qt6 | ≥6.2 | find_package | client 专用 — UI、网络、文件 |

---

## 8. 部署拓扑

### 8.1 裸机部署（最小化）

```
┌─────────────────────────────────────────┐
│              Linux 服务器                │
│                                         │
│  cloudvault_server  ←→  MySQL 8.0        │
│  监听 :5000 (TLS)       数据库: cloudvault │
│                                         │
│  文件存储: /data/cloudvault/filesys/       │
└─────────────────────────────────────────┘
         ↑ TLS TCP
┌────────┴────────┐
│  Windows 客户端  │  ×N
└─────────────────┘
```

### 8.2 Docker Compose 部署

```yaml
services:
  db:
    image: mysql:8.0
    volumes: [ "db_data:/var/lib/mysql" ]
    environment:
      MYSQL_DATABASE: cloudvault
      MYSQL_USER: cloudvault_app
      MYSQL_PASSWORD_FILE: /run/secrets/db_password

  server:
    build: ./server
    ports: [ "5000:5000" ]
    depends_on: [ db ]
    volumes:
      - "file_data:/data/filesys"
      - "./config/server.json:/etc/cloudvault/server.json:ro"
    secrets: [ db_password, tls_cert, tls_key ]
```

---

## 9. 与 v1 的主要差异

| 方面 | v1 | v2 |
|------|----|----|
| 服务端框架 | Qt（QWidget + QTcpServer） | 纯 C++（epoll + std::thread） |
| 构建系统 | qmake | CMake 3.20+ |
| 协议 | PDU（caData[64] + malloc/free） | PDU v2（16字节头 + 变长body，RAII） |
| 数据库操作 | 字符串拼接 SQL（注入漏洞） | 预处理语句 |
| 密码 | 明文存储和传输 | SHA-256 + salt，传输前客户端哈希 |
| 线程模型 | QRunnable + moveToThread（错误用法） | 主线程 epoll I/O + 工作线程业务逻辑 |
| 客户端架构 | 单例 + UI 直接操作 PDU | 三层分离（network/service/ui） |
| 协议代码 | 双端各一份（重复） | 单一 common 静态库 |
| 安全 | 无 TLS，无路径校验 | TLS + 路径遍历防护 + 输入校验 |
| 新功能 | — | 文件下载/删除/重命名、离线消息、群聊、大文件分片 |
