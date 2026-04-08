# CloudVault 模块设计文档

> 版本：v2.0 | 状态：与当前代码对齐

---

## 1. 模块依赖总览

```text
client/ui
  -> client/service
  -> client/network
  -> common

server/handler
  -> server/db
  -> server/session_manager
  -> common

server/server_app
  -> EventLoop / TcpServer / ThreadPool / MessageDispatcher
  -> Database / FileStorage / SessionManager
  -> AuthHandler / FriendHandler / ChatHandler / GroupHandler / FileHandler / ShareHandler
```

说明：

- `common` 只保留 `protocol.h`、`protocol_codec.h`、`crypto_utils.h` 等实际存在的组件。
- 旧文档中的 `message_types.h`、`constants.h`、`Config` 类均已不存在。

---

## 2. common 模块

### 2.1 `protocol.h`

核心内容：

- `enum class MessageType : uint32_t`
- `struct PDUHeader`
- `PDU_HEADER_SIZE`
- `PDU_MAX_BODY_SIZE`
- `FILE_TRANSFER_CHUNK_SIZE`

### 2.2 `protocol_codec.h`

当前 `PDUBuilder` 接口：

```cpp
class PDUBuilder {
public:
    explicit PDUBuilder(MessageType type);

    PDUBuilder& writeUInt8(uint8_t val);
    PDUBuilder& writeUInt16(uint16_t val);
    PDUBuilder& writeUInt32(uint32_t val);
    PDUBuilder& writeUInt64(uint64_t val);
    PDUBuilder& writeString(const std::string& s);
    PDUBuilder& writeFixedString(const std::string& s, size_t maxLen);
    PDUBuilder& writeBytes(const void* data, size_t len);

    std::vector<uint8_t> build() const;
};
```

当前 `PDUParser` 接口：

```cpp
class PDUParser {
public:
    void feed(const void* data, size_t len);
    bool tryParse(PDUHeader& header, std::vector<uint8_t>& body);
    void reset();
};
```

### 2.3 `crypto_utils.h`

当前保留的密码能力：

```cpp
namespace crypto {
std::string hashPassword(const std::string& password);
bool verifyPassword(const std::string& transport_hash,
                    const std::string& stored_hash);
std::string hashForTransport(const std::string& plain_password);
}
```

---

## 3. server 模块

### 3.1 `Database`

当前数据库层是 RAII 连接池，而不是旧版 `init()/shutdown()` 风格：

```cpp
class Database {
public:
    struct Config {
        std::string host;
        int         port;
        std::string db_name;
        std::string user;
        std::string password;
        int         pool_size;
    };

    explicit Database(Config cfg);
    ~Database();

    class Connection {
    public:
        MYSQL* get() const;
        ~Connection();
    };

    Connection acquire();
    void ping();
};
```

### 3.2 Repository 层

当前实际仓储：

- `UserRepository`
- `FriendRepository`
- `ChatRepository`
- `GroupRepository`

关键差异：

- `UserInfo` 使用 `user_id`、`username`、`is_online` 命名。
- `FriendRepository` 已迁移到 `friend.status`，不再依赖内存 pending map。
- `ChatRepository` 负责私聊、群聊、离线消息和私聊/群聊历史。
- `GroupRepository` 负责 `chat_group` / `group_member`。

### 3.3 `MessageDispatcher`

当前签名：

```cpp
class MessageDispatcher {
public:
    using HandlerFn = std::function<void(
        std::shared_ptr<TcpConnection>,
        const PDUHeader&,
        const std::vector<uint8_t>&)>;

    void registerHandler(MessageType type, HandlerFn handler);
    void dispatch(std::shared_ptr<TcpConnection> conn,
                  const PDUHeader& header,
                  const std::vector<uint8_t>& body);
};
```

### 3.4 Handler 层

当前已装配的 handler：

```cpp
AuthHandler
ChatHandler
FriendHandler
GroupHandler
FileHandler
ShareHandler
```

职责概览：

- `AuthHandler`：注册、登录、登出、资料更新、登录后离线消息投递
- `FriendHandler`：查找用户、发好友申请、同意申请、刷新好友、删除好友
- `ChatHandler`：私聊发送、私聊历史查询
- `GroupHandler`：建群、入群、退群、群列表、群聊广播
- `FileHandler`：目录与文件管理、上传下载
- `ShareHandler`：文件分享与接受

### 3.5 `ServerApp`

当前服务端入口保留“两步初始化”：

```cpp
class ServerApp {
public:
    ServerApp();
    ~ServerApp();

    bool init(const std::string& config_path);
    void run();
};
```

当前 `ServerApp` 负责：

1. 加载 JSON 配置
2. 初始化日志
3. 创建 `Database`
4. 创建 `FileStorage`
5. 创建各业务 handler
6. 创建 `EventLoop` / `ThreadPool` / `TcpServer`
7. 在 `registerHandlers()` 中注册路由

---

## 4. client 模块

### 4.1 `TcpClient`

当前网络接口：

```cpp
class TcpClient : public QObject {
    Q_OBJECT
public:
    explicit TcpClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    void send(std::vector<uint8_t> data);
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void pduReceived(PDUHeader header, std::vector<uint8_t> body);
    void errorOccurred(const QString& message);
};
```

### 4.2 `ResponseRouter`

当前是轻量分发表，不再要求继承 `QObject`：

```cpp
class ResponseRouter {
public:
    using HandlerFn = std::function<void(
        const PDUHeader&,
        const std::vector<uint8_t>&)>;

    void registerHandler(MessageType type, HandlerFn handler);
    void dispatch(const PDUHeader& header,
                  const std::vector<uint8_t>& body);
};
```

### 4.3 Service 层

当前实际存在：

- `AuthService`
- `FriendService`
- `ChatService`
- `GroupService`
- `FileService`
- `ShareService`

关键接口：

```cpp
AuthService::registerUser / login / logout / updateProfile
FriendService::findUser / addFriend / agreeFriend / flushFriends / deleteFriend
ChatService::sendMessage / loadHistory / loadGroupHistory
GroupService::createGroup / joinGroup / leaveGroup / getGroupList / sendGroupMessage
FileService::listFiles / createDirectory / renamePath / movePath / deletePath / uploadFile / downloadFile / search
ShareService::shareFile / acceptShare
```

说明：

- `ChatService::loadGroupHistory()` 当前仅保留接口占位，尚未接入独立协议。
- 群聊实时收发由 `GroupService` 完成。

### 4.4 App 装配层

当前 `App` 统一持有：

```cpp
TcpClient
ResponseRouter
AuthService
GroupService
ChatService
FriendService
FileService
ShareService
```

主窗口构造签名已更新为：

```cpp
MainWindow(const QString& username,
           AuthService& auth_service,
           GroupService& group_service,
           ChatService& chat_service,
           FriendService& friend_service,
           FileService& file_service,
           ShareService& share_service,
           QWidget* parent = nullptr);
```

### 4.5 UI 层

当前主界面由以下核心组件协作：

- `MainWindow`
- `SidebarPanel`
- `ChatPanel`
- `FilePanel`
- `DetailPanel`
- `ProfilePanel`
- `GroupListDialog`
- `OnlineUserDialog`
- `ShareFileDialog`

状态特征：

- 私聊页面已完整接入服务层
- 资料页保存会同步到服务端
- 群组列表已可创建、进入、退出群组
- 群聊实时消息已接入
- 群聊历史仍依赖后续独立协议补齐

---

## 5. 已移除或不再适用的旧设计

以下内容不再属于当前实现：

- `message_types.h`
- `constants.h`
- 单独的 `Config` 类
- `TcpServer(const Config&, ...)` 风格构造
- `ServerApp(const std::string& config_path)` 单步构造初始化
- 统一 `uint32_t status_code` 响应体规范
- 基于内存 `pending_requests_` 的好友申请管理

---

## 6. 文档使用约定

后续若协议或模块签名再发生变更，应优先同步以下源码作为事实来源：

- 协议：`common/include/common/protocol.h`
- 编解码：`common/include/common/protocol_codec.h`
- 服务端路由：`server/src/server_app.cpp`
- 客户端服务层：`client/src/service`
