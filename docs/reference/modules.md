# CloudVault 云巢 — 模块设计文档

> **版本**：v2.0 | **状态**：已确认

---

## 1. 模块依赖总览

```
┌─────────────────────────────────────────────────────────────────────┐
│                         client（Qt 应用）                            │
│                                                                     │
│   ui/          ──依赖──►  service/      ──依赖──►  network/         │
│   LoginWindow             AuthService             TcpClient         │
│   MainWindow              FriendService           ResponseRouter    │
│   FriendWidget            ChatService                               │
│   FileWidget              FileService                               │
│   ...                                                               │
│                    ↓ 全部依赖                                        │
└────────────────────────────────────────────────────────────────────-┘
                     common（静态库）
                     protocol / message_types / crypto_utils / constants
┌─────────────────────────────────────────────────────────────────────┐
│                         server（纯 C++）                             │
│                                                                     │
│   handler/     ──依赖──►  db/           ──依赖──►  核心基础设施      │
│   AuthHandler             UserRepository          TcpServer         │
│   FriendHandler           FriendRepository        TcpConnection     │
│   ChatHandler             Database                EventLoop         │
│   FileHandler                                     ThreadPool        │
│                ──依赖──►  FileStorage             SessionManager    │
│                ──依赖──►  SessionManager          Config            │
│                                                                     │
│                    ↑ 全部依赖                                        │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 2. common 库

### 2.1 message_types.h

```cpp
// 消息类型强类型枚举，完整定义见 api.md 第 2 节
enum class MessageType : uint32_t { ... };
```

### 2.2 protocol.h / protocol_codec.h

**PDUHeader**：

```cpp
struct PDUHeader {
    uint32_t total_length;   // 整包字节数
    uint32_t body_length;    // body 区字节数
    uint32_t message_type;   // MessageType
    uint32_t reserved;       // 填 0
};
static_assert(sizeof(PDUHeader) == 16);
```

**PDUBuilder** — 构建发送 PDU：

```cpp
class PDUBuilder {
public:
    explicit PDUBuilder(MessageType type);

    PDUBuilder& writeUint8(uint8_t val);
    PDUBuilder& writeUint32(uint32_t val);
    PDUBuilder& writeInt64(int64_t val);
    PDUBuilder& writeFixedString(const std::string& s, size_t len); // 不足补 \0
    PDUBuilder& writeLengthString(const std::string& s);            // uint32_t + data
    PDUBuilder& writeBytes(const uint8_t* data, size_t len);

    std::vector<uint8_t> build() const; // 填充 header，返回完整 PDU 字节序列
};
```

**PDUParser** — 接收缓冲区拆包：

```cpp
class PDUParser {
public:
    // 将新收到的数据追加到内部缓冲区
    void feed(const uint8_t* data, size_t len);

    // 尝试取出一个完整 PDU；返回 false 表示数据不足
    bool tryParse(PDUHeader& header, std::vector<uint8_t>& body);

    size_t bufferedSize() const;
};
```

### 2.3 crypto_utils.h

```cpp
namespace crypto {

// 注册时：生成随机 salt，计算哈希，返回存储字符串 "hex_salt:hex_hash"
std::string hashPassword(const std::string& plain_password);

// 登录时：用存储字符串和输入密码验证
bool verifyPassword(const std::string& plain_password,
                    const std::string& stored_hash);

// 客户端发送前哈希（传输层哈希，非存储哈希）
std::string hashForTransport(const std::string& plain_password);

} // namespace crypto
```

### 2.4 constants.h

```cpp
namespace constants {
    constexpr size_t   MAX_NAME_LEN    = 32;
    constexpr size_t   MAX_PATH_LEN    = 512;
    constexpr size_t   CHUNK_SIZE      = 4 * 1024 * 1024; // 4MB
    constexpr uint32_t MAX_PDU_SIZE    = 64 * 1024 * 1024; // 64MB
    constexpr int      PROTOCOL_VER    = 2;
    constexpr uint16_t DEFAULT_PORT    = 5000;
}
```

---

## 3. server 模块

### 3.1 Config

```cpp
class Config {
public:
    // 从 JSON 文件加载，失败抛 std::runtime_error
    static Config load(const std::string& path);

    // 访问器
    std::string serverHost() const;
    uint16_t    serverPort() const;
    std::string rootPath()   const;
    std::string dbHost()     const;
    uint16_t    dbPort()     const;
    std::string dbName()     const;
    std::string dbUser()     const;
    std::string dbPassword() const; // 从环境变量读取
    int         threadPoolSize() const;
    std::string logLevel()   const;
    std::string logFile()    const;
    std::string tlsCertPath() const;
    std::string tlsKeyPath()  const;
};
```

**配置文件格式**（`config/server.json.example`）：

```json
{
  "server":   { "host": "0.0.0.0", "port": 5000, "root_path": "./filesys" },
  "database": { "host": "localhost", "port": 3306,
                "name": "cloudvault",  "user": "cloudvault_app",
                "password_env": "CLOUDVAULT_DB_PASSWORD" },
  "thread_pool": { "size": 8 },
  "log":      { "level": "info", "file": "./logs/server.log" },
  "tls":      { "cert": "./certs/server.crt", "key": "./certs/server.key" }
}
```

---

### 3.2 EventLoop

```cpp
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void addFd(int fd, uint32_t events, void* data);    // EPOLLIN | EPOLLET 等
    void modifyFd(int fd, uint32_t events, void* data);
    void removeFd(int fd);

    // 阻塞运行，直到 stop() 被调用
    void run();
    void stop();

    // 注册回调：fd 可读 / 可写 / 断开时调用
    using EventCallback = std::function<void(int fd, uint32_t events)>;
    void setCallback(EventCallback cb);

private:
    int epoll_fd_;
    std::atomic<bool> running_;
    EventCallback callback_;
};
```

---

### 3.3 TcpServer

```cpp
class TcpServer {
public:
    TcpServer(const Config& config, EventLoop& loop,
              SessionManager& sessions, ThreadPool& pool,
              MessageDispatcher& dispatcher);

    void start(); // 创建 listen socket，注册到 EventLoop
    void stop();

private:
    void onAccept();          // EventLoop 回调，accept 新连接
    void onReadable(int fd);  // 有数据可读
    void onWritable(int fd);  // 发送队列有数据待发
    void onDisconnect(int fd);

    std::unordered_map<int, std::unique_ptr<TcpConnection>> connections_;
    int listen_fd_;
    // 依赖引用
    EventLoop&         loop_;
    SessionManager&    sessions_;
    ThreadPool&        pool_;
    MessageDispatcher& dispatcher_;
};
```

---

### 3.4 TcpConnection

```cpp
class TcpConnection {
public:
    explicit TcpConnection(int fd);

    int  fd() const;

    // 将收到的原始字节追加到接收缓冲区，尝试拆包
    // 返回所有完整解析出的 PDU（header + body）
    std::vector<std::pair<PDUHeader, std::vector<uint8_t>>>
    feed(const uint8_t* data, size_t len);

    // 将 PDU 追加到发送队列（线程安全）
    void enqueue(std::vector<uint8_t> pdu_bytes);

    // 从发送队列取数据写入 socket，返回是否还有剩余
    bool flushSendQueue();

    // 会话信息（登录后设置）
    std::string loginName;
    int         userId = -1;
    bool        isLoggedIn = false;

private:
    int fd_;
    PDUParser parser_;
    std::mutex send_mutex_;
    std::queue<std::vector<uint8_t>> send_queue_;
};
```

---

### 3.5 ThreadPool

```cpp
class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count);
    ~ThreadPool(); // 等待所有任务完成后退出

    // 提交任务（线程安全）
    void submit(std::function<void()> task);

    // 等待所有当前队列中的任务完成
    void drain();

private:
    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mutex_;
    std::condition_variable           cv_;
    bool                              stop_ = false;
};
```

---

### 3.6 SessionManager

```cpp
class SessionManager {
public:
    // 登录成功后注册
    void registerSession(const std::string& name, TcpConnection* conn);

    // 连接断开后注销（thread-safe）
    void unregisterSession(int fd);

    // 查找在线连接（nullptr = 离线）
    TcpConnection* findByName(const std::string& name) const;

    // 获取所有在线用户名
    std::vector<std::string> getOnlineUsers() const;

    // 向指定用户发送 PDU（若离线则静默失败）
    void sendTo(const std::string& name, const std::vector<uint8_t>& pdu);

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, TcpConnection*> name_to_conn_;
    std::unordered_map<int, std::string>            fd_to_name_;
};
```

---

### 3.7 Database / Repository

**Database（连接池）**：

```cpp
class Database {
public:
    void init(const Config& config, size_t pool_size);
    void shutdown();

    // RAII 连接借用：超出作用域自动归还
    class Connection {
    public:
        MYSQL* handle();
        ~Connection(); // 归还到池
    private:
        friend class Database;
        MYSQL*    mysql_;
        Database* pool_;
    };

    Connection acquire(); // 阻塞直到有可用连接

private:
    std::queue<MYSQL*>      pool_;
    std::mutex              mutex_;
    std::condition_variable cv_;
};
```

**UserRepository**：

```cpp
class UserRepository {
public:
    explicit UserRepository(Database& db);

    struct UserRecord {
        int         id;
        std::string name;
        std::string password_hash;
        bool        online;
    };

    // 注册：插入用户，创建目录（由 FileStorage 完成）
    // 返回 false = 用户名已存在
    bool insertUser(const std::string& name,
                    const std::string& password_hash);

    // 登录：返回用户记录（id + stored_hash）；不存在返回 nullopt
    std::optional<UserRecord> findByName(const std::string& name);

    void setOnline(int user_id, bool online);

    // 查询在线状态：-1=不存在, 0=离线, 1=在线
    int  queryOnlineStatus(const std::string& name);

    std::vector<std::string> getOnlineUsers();

private:
    Database& db_;
    // 预处理语句（init 时 prepare）
    // MYSQL_STMT* stmt_insert_, stmt_find_, stmt_set_online_, ...
};
```

**FriendRepository**：

```cpp
class FriendRepository {
public:
    explicit FriendRepository(Database& db);

    // 检查是否已是好友（status=1）
    bool areFriends(int user_id, int friend_id);

    // 插入好友申请记录（status=0）
    bool insertRequest(int from_id, int to_id);

    // 同意申请，将 status 置为 1
    bool acceptRequest(int from_id, int to_id);

    // 删除好友关系（双向）
    bool removeFriend(int user_id, int friend_id);

    // 获取在线好友列表
    std::vector<std::string> getOnlineFriends(int user_id);

    // 获取全部好友列表（含在线状态）
    struct FriendEntry { std::string name; bool online; };
    std::vector<FriendEntry> getAllFriends(int user_id);

private:
    Database& db_;
};
```

---

### 3.8 FileStorage

```cpp
class FileStorage {
public:
    explicit FileStorage(const std::string& root_path);

    // 创建用户根目录（注册时调用）
    bool createUserDir(const std::string& username);

    // 解析并验证路径（防路径遍历），返回绝对路径
    // 若路径越权抛 SecurityException
    std::filesystem::path resolvePath(const std::string& user_root,
                                      const std::string& relative);

    // 目录操作
    bool mkdir(const std::filesystem::path& path);
    bool remove(const std::filesystem::path& path);  // 文件或空目录
    bool removeAll(const std::filesystem::path& path); // 递归删除
    bool rename(const std::filesystem::path& src,
                const std::filesystem::path& dst);
    bool copyFile(const std::filesystem::path& src,
                  const std::filesystem::path& dst);
    bool copyDir(const std::filesystem::path& src,
                 const std::filesystem::path& dst);

    // 列目录
    struct FileEntry { std::string name; bool is_dir; };
    std::vector<FileEntry> listDir(const std::filesystem::path& path);

    // 搜索（递归，按文件名子串匹配）
    std::vector<std::string> search(const std::filesystem::path& user_root,
                                    const std::string& keyword);

    // 文件读写（上传/下载）
    std::ofstream openForWrite(const std::filesystem::path& path);
    std::ifstream openForRead(const std::filesystem::path& path);

private:
    std::filesystem::path root_;
};
```

---

### 3.9 MessageDispatcher

```cpp
class MessageDispatcher {
public:
    using HandlerFn = std::function<
        void(const PDUHeader&, const std::vector<uint8_t>& body,
             TcpConnection& conn)>;

    void registerHandler(MessageType type, HandlerFn fn);

    // 由 ThreadPool 工作线程调用
    void dispatch(const PDUHeader& header,
                  const std::vector<uint8_t>& body,
                  TcpConnection& conn);

private:
    std::unordered_map<uint32_t, HandlerFn> handlers_;
};
```

---

### 3.10 Handler 层

所有 Handler 在 `ServerApp` 初始化时注入依赖，方法均为无状态函数式调用。

**AuthHandler**：

```cpp
class AuthHandler {
public:
    AuthHandler(UserRepository& users, FileStorage& fs,
                SessionManager& sessions);

    void handleRegister(const PDUHeader&, const std::vector<uint8_t>& body,
                        TcpConnection& conn);
    void handleLogin   (const PDUHeader&, const std::vector<uint8_t>& body,
                        TcpConnection& conn);
    void handleLogout  (const PDUHeader&, const std::vector<uint8_t>& body,
                        TcpConnection& conn);
    void handleUpdateProfile(const PDUHeader&, const std::vector<uint8_t>&,
                             TcpConnection&);

private:
    UserRepository& users_;
    FileStorage&    fs_;
    SessionManager& sessions_;
};
```

**FriendHandler**：

```cpp
class FriendHandler {
public:
    FriendHandler(UserRepository& users, FriendRepository& friends,
                  SessionManager& sessions);

    void handleFindUser     (...);
    void handleOnlineUsers  (...);
    void handleAddFriend    (...); // 转发申请给目标方
    void handleAgreeAddFriend(...);// 写 DB，通知双方
    void handleFlushFriend  (...);
    void handleDeleteFriend (...);
};
```

**ChatHandler**：

```cpp
class ChatHandler {
public:
    ChatHandler(SessionManager& sessions, UserRepository& users,
                Database& db);

    void handleChat      (...); // 在线转发 or 写 offline_message
    void handleGroupChat (...); // 广播给所有在线成员
    void handleGetHistory(...); // 查询 chat_message 表，分页返回
};
```

**FileHandler**：

```cpp
class FileHandler {
public:
    FileHandler(FileStorage& fs, SessionManager& sessions);

    void handleMkdir       (...);
    void handleFlushFile   (...);
    void handleMoveFile    (...);
    void handleDeleteFile  (...);
    void handleRenameFile  (...);
    void handleSearchFile  (...);
    void handleUploadFile  (...); // 初始化：打开文件句柄
    void handleUploadData  (...); // 写数据块；最后一块关闭并响应
    void handleDownloadFile(...); // 获取文件大小，响应后连续推送数据块
    void handleShareFile   (...); // 向每个好友推送 SHARE_FILE_NOTIFY
    void handleShareAgree  (...); // 服务端复制文件到接收方目录

private:
    FileStorage&    fs_;
    SessionManager& sessions_;

    // 每个连接的上传状态（fd → UploadState）
    struct UploadState {
        std::ofstream file;
        int64_t       expected_size;
        int64_t       received_size;
    };
    std::unordered_map<int, UploadState> upload_states_;
    std::mutex upload_mutex_;
};
```

---

### 3.11 ServerApp（组装入口）

```cpp
class ServerApp {
public:
    explicit ServerApp(const std::string& config_path);
    void run();   // 阻塞，直到收到 SIGINT/SIGTERM
    void stop();

private:
    void init();
    void setupSignalHandlers();

    Config             config_;
    Database           db_;
    UserRepository     user_repo_;
    FriendRepository   friend_repo_;
    FileStorage        file_storage_;
    SessionManager     session_mgr_;
    ThreadPool         thread_pool_;
    MessageDispatcher  dispatcher_;
    EventLoop          event_loop_;
    TcpServer          tcp_server_;

    // Handlers（持有引用，依赖注入）
    AuthHandler    auth_handler_;
    FriendHandler  friend_handler_;
    ChatHandler    chat_handler_;
    FileHandler    file_handler_;
};
```

初始化顺序：`Config` → `Database` → `Repositories` → `FileStorage` → `SessionManager` → `ThreadPool` → `Handlers 注册到 Dispatcher` → `TcpServer` → `EventLoop::run()`

---

## 4. client 模块

### 4.1 TcpClient

```cpp
class TcpClient : public QObject {
    Q_OBJECT
public:
    explicit TcpClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port);
    void disconnect();
    void sendPDU(const std::vector<uint8_t>& pdu);

signals:
    void connected();
    void disconnected();
    void pduReceived(PDUHeader header, std::vector<uint8_t> body);
    void errorOccurred(QString message);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();

private:
    QTcpSocket* socket_;
    PDUParser   parser_;
};
```

---

### 4.2 ResponseRouter

```cpp
class ResponseRouter : public QObject {
    Q_OBJECT
public:
    explicit ResponseRouter(TcpClient* client, QObject* parent = nullptr);

    using HandlerFn = std::function<
        void(const PDUHeader&, const std::vector<uint8_t>&)>;

    void registerHandler(MessageType type, HandlerFn fn);

private slots:
    void onPduReceived(PDUHeader header, std::vector<uint8_t> body);

private:
    std::unordered_map<uint32_t, HandlerFn> handlers_;
};
```

---

### 4.3 Service 层

每个 Service 的职责：
- 接收来自 UI 的操作请求，构建 PDU，调用 `TcpClient::sendPDU()`
- 在 `ResponseRouter` 注册回调，解析响应，发出 Qt 信号供 UI 连接

**AuthService**：

```cpp
class AuthService : public QObject {
    Q_OBJECT
public:
    AuthService(TcpClient* client, ResponseRouter* router,
                QObject* parent = nullptr);

    void registerUser(const QString& name, const QString& password);
    void login(const QString& name, const QString& password);
    void logout();
    void updateProfile(const QString& nickname, const QString& signature);

signals:
    void registerSucceeded();
    void registerFailed(QString reason);
    void loginSucceeded(QString username);
    void loginFailed(QString reason);
    void logoutSucceeded();
    void profileUpdated();

private:
    void onRegisterRespond(const PDUHeader&, const std::vector<uint8_t>&);
    void onLoginRespond   (const PDUHeader&, const std::vector<uint8_t>&);
    // ...
    TcpClient*      client_;
    ResponseRouter* router_;
};
```

**FriendService**：

```cpp
class FriendService : public QObject {
    Q_OBJECT
public:
    FriendService(TcpClient*, ResponseRouter*, QObject* parent = nullptr);

    void findUser(const QString& name);
    void getOnlineUsers();
    void addFriend(const QString& me, const QString& target);
    void agreeAddFriend(const QString& me, const QString& requester);
    void flushFriends(const QString& me);
    void deleteFriend(const QString& me, const QString& target);

signals:
    void userFound(QString name, bool online);
    void userNotFound(QString name);
    void onlineUsersReceived(QStringList users);
    void friendRequestSent(int status);
    void incomingFriendRequest(QString from);  // 被动推送
    void friendAdded();
    void friendAddFailed(QString reason);
    void friendsRefreshed(QList<QPair<QString,bool>> friends);
    void friendDeleted();
    void friendDeleteFailed(QString reason);
};
```

**ChatService**：

```cpp
class ChatService : public QObject {
    Q_OBJECT
public:
    ChatService(TcpClient*, ResponseRouter*, QObject* parent = nullptr);

    void sendMessage(const QString& from, const QString& to,
                     const QString& message);
    void sendGroupMessage(const QString& from, uint32_t group_id,
                          const QString& message);
    void getHistory(const QString& peer, int page_size, int offset);

signals:
    void messageReceived(QString from, QString to, QString content);
    void groupMessageReceived(QString from, uint32_t group_id, QString content);
    void historyReceived(QList<ChatMessage> messages);
    void incomingMessage(QString from, QString content); // 被动推送
};
```

**FileService**：

```cpp
class FileService : public QObject {
    Q_OBJECT
public:
    FileService(TcpClient*, ResponseRouter*, QObject* parent = nullptr);

    void listDir(const QString& path);
    void mkdir(const QString& name, const QString& parent_path);
    void moveFile(const QString& src, const QString& dst);
    void deleteFile(const QString& path);
    void renameFile(const QString& path, const QString& new_name);
    void searchFile(const QString& username, const QString& keyword);
    void uploadFile(const QString& local_path, const QString& server_dir);
    void downloadFile(const QString& server_path, const QString& local_save_path);
    void shareFile(const QString& from, const QStringList& friends,
                   const QString& file_path);
    void agreeShareFile(const QString& receiver, const QString& src_path);

signals:
    void dirListed(QList<FileEntry> files);
    void mkdirSucceeded();
    void mkdirFailed(QString reason);
    void fileMoved();
    void fileDeleted();
    void fileRenamed();
    void searchResult(QStringList paths);
    void uploadProgress(int64_t sent, int64_t total);
    void uploadSucceeded();
    void uploadFailed(QString reason);
    void downloadProgress(int64_t received, int64_t total);
    void downloadSucceeded();
    void downloadFailed(QString reason);
    void shareSucceeded();
    void incomingShareNotify(QString from, QString filename, QString src_path);
    void shareAgreeSucceeded();
};
```

---

### 4.4 UI 层

UI 层只做两件事：**渲染数据** + **响应用户操作**。不直接构建 PDU，不直接访问网络。

**原则**：
- 构造函数接收所需 Service 指针，通过 `connect()` 绑定信号槽
- 发起操作：调用 Service 方法
- 响应结果：连接 Service 信号，更新 UI 控件

**LoginWindow 示例**：

```cpp
class LoginWindow : public QWidget {
    Q_OBJECT
public:
    explicit LoginWindow(AuthService* auth, QWidget* parent = nullptr);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLoginSucceeded(QString username);
    void onLoginFailed(QString reason);

private:
    Ui::LoginWindow* ui_;
    AuthService*     auth_;
    MainWindow*      main_window_ = nullptr;
};

// 构造函数中绑定信号槽
LoginWindow::LoginWindow(AuthService* auth, QWidget* parent)
    : QWidget(parent), ui_(new Ui::LoginWindow), auth_(auth)
{
    ui_->setupUi(this);
    connect(auth_, &AuthService::loginSucceeded,
            this,  &LoginWindow::onLoginSucceeded);
    connect(auth_, &AuthService::loginFailed,
            this,  &LoginWindow::onLoginFailed);
    connect(ui_->login_btn, &QPushButton::clicked,
            this, &LoginWindow::onLoginClicked);
}
```

---

### 4.5 App（客户端组装入口）

```cpp
class App {
public:
    App(int argc, char* argv[]);
    int run();

private:
    void loadConfig();

    QApplication app_;

    // 网络层
    TcpClient      tcp_client_;
    ResponseRouter router_;

    // Service 层
    AuthService    auth_service_;
    FriendService  friend_service_;
    ChatService    chat_service_;
    FileService    file_service_;

    // UI 层（首个窗口）
    LoginWindow    login_window_;
};
```

---

## 5. 关键设计模式

| 模式 | 应用场景 |
|------|---------|
| **依赖注入（构造函数）** | 所有模块通过构造函数接受依赖，无全局单例 |
| **观察者（Qt 信号槽）** | Service → UI 的结果通知 |
| **RAII** | Database::Connection 自动归还、PDUBuilder 内存管理 |
| **策略（Handler 注册表）** | MessageDispatcher 和 ResponseRouter 的 Handler 映射 |
| **工厂** | PDUBuilder 统一构建 PDU，避免直接操作字节 |
| **连接池** | Database 管理固定大小的 MYSQL* 连接集合 |

---

## 6. 错误处理分层

| 层级 | 错误类型 | 处理方式 |
|------|---------|---------|
| 基础设施（DB 连接、socket） | 致命错误 | 抛 `std::runtime_error`，ServerApp 顶层捕获并记录 |
| 业务逻辑（Handler） | 业务错误 | 构建含错误 `status_code` 的响应 PDU，不抛异常 |
| 客户端网络 | 连接断开/超时 | `TcpClient` 发出 `errorOccurred` 信号，UI 显示重连提示 |
| 客户端业务 | 服务端返回错误码 | Service 解析后 emit `xxxFailed(reason)` 信号 |
| 输入校验 | 格式不合法 | UI 层本地校验，不发送请求；服务端二次校验返回 `STATUS_INVALID_INPUT` |
