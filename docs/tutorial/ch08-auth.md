# 第八章　注册与登录

> **状态**：✅ 已完成
>
> **本章目标**：客户端输入用户名和密码，服务端将用户信息写入 MySQL，并返回注册/登录结果，形成完整闭环。
>
> **验收标准**：
> - 客户端能发送 `REGISTER_REQUEST` 和 `LOGIN_REQUEST`
> - 服务端能写入 `user_info` 表并返回 `user_id`
> - 重复注册返回错误提示
> - 用户不存在、密码错误、重复登录能返回对应错误提示
> - 登录成功后建立会话，断开连接后自动清理

---

## 8.1 本章完成了什么

本章把第七章的“网络可达”推进为“业务可用”：

- 服务端接入 MySQL 连接池
- 新增 `user_info` 表访问层 `UserRepository`
- 新增认证业务处理器 `AuthHandler`
- 新增在线会话管理 `SessionManager`
- 客户端新增 `AuthService`
- 登录窗口改为通过配置文件读取服务器地址
- 客户端可完成注册、登录、错误提示、在线状态显示

最终联调结果是：

1. 客户端连接服务端
2. 客户端先发 `PING`，收到 `PONG`
3. 新用户注册成功
4. 已注册用户登录成功
5. 服务端记录在线会话
6. 客户端断开后服务端自动移除会话

---

## 8.2 数据流总览

```text
用户输入用户名/密码
      ↓
LoginWindow
      ↓
AuthService::registerUser() / login()
      ↓
TcpClient 发送 PDU
      ↓
ServerApp -> MessageDispatcher
      ↓
AuthHandler
      ↓
UserRepository <-> Database 连接池 <-> MySQL
      ↓
返回 REGISTER_RESPONSE / LOGIN_RESPONSE
      ↓
AuthService 解析响应
      ↓
LoginWindow 更新 UI
```

---

## 8.3 数据库设计

### 8.3.1 初始化脚本

数据库初始化脚本位于 [server/sql/init.sql](/mnt/d/CloudVault/server/sql/init.sql)。

脚本完成以下工作：

- 创建数据库 `cloudvault`
- 创建应用账号 `cloudvault_app`
- 授权 `cloudvault_app` 访问 `cloudvault.*`
- 创建 `user_info` 表
- 为后续章节预创建好友、聊天、文件相关表

执行方式：

```bash
mysql -u root -p < server/sql/init.sql
```

或：

```bash
mysql -u root -P 3307 -h 127.0.0.1 -p < server/sql/init.sql
```

### 8.3.2 用户表

本章真正用到的核心表是：

```sql
CREATE TABLE IF NOT EXISTS user_info (
    user_id       INT          NOT NULL AUTO_INCREMENT PRIMARY KEY,
    username      VARCHAR(32)  NOT NULL UNIQUE,
    password_hash VARCHAR(128) NOT NULL,
    is_online     TINYINT      NOT NULL DEFAULT 0,
    created_at    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

字段说明：

- `user_id`：登录成功后返回给客户端
- `username`：唯一用户名
- `password_hash`：服务端最终落库的加盐哈希
- `is_online`：用户在线状态
- `created_at`：注册时间

---

## 8.4 配置文件

### 8.4.1 服务端配置

服务端示例配置位于 [server.example.json](/mnt/d/CloudVault/server/config/server.example.json)：

```json
{
    "server": {
        "host": "0.0.0.0",
        "port": 5000,
        "thread_count": 8
    },
    "database": {
        "host": "127.0.0.1",
        "port": 3306,
        "name": "cloudvault",
        "user": "cloudvault_app",
        "password_env": "CLOUDVAULT_DB_PASSWORD"
    }
}
```

密码读取顺序与当前实现一致：

1. 优先读取 `database.password_env` 指向的环境变量
2. 如果环境变量不存在，再回退读取 `database.password`

对应实现见 [server_app.cpp](/mnt/d/CloudVault/server/src/server_app.cpp#L103)。

如果 `password_env` 指向的环境变量没有设置，启动日志会直接告警，避免出现“密码为空但原因不明”的排查成本。

### 8.4.2 客户端配置

客户端现在不再硬编码服务器地址，而是必须从配置文件读取。示例文件位于 [client.example.json](/mnt/d/CloudVault/client/config/client.example.json)：

```json
{
    "server": {
        "host": "127.0.0.1",
        "port": 5001
    }
}
```

客户端启动时会尝试在若干候选路径中查找 `client.json`，其中包含：

- `client/config/client.json`
- 构建目录上级的 `../client/config/client.json`
- 可执行文件同级或上级目录中的 `config/client.json`

只要找到一个合法配置，就会建立连接；如果所有配置都不存在或内容非法，客户端会进入“配置错误”状态，并禁用登录/注册按钮。

对应实现见 [login_window.cpp](/mnt/d/CloudVault/client/src/ui/login_window.cpp#L424)。

---

## 8.5 密码安全链路

当前实现的密码链路如下：

```text
用户输入明文密码
      ↓
客户端：crypto::hashForTransport(password)
      ↓
网络上传输 64 字节十六进制 SHA-256 字符串
      ↓
服务端：crypto::hashPassword(transport_hash)
      ↓
带 salt 的结果写入 user_info.password_hash
      ↓
登录时：crypto::verifyPassword(password_hash_from_client, stored_hash)
```

这条链路的目标不是替代 TLS，而是做到：

- 网络层不直接传明文密码
- 数据库不存明文，也不存可直接复用的传输哈希
- 落库值包含 salt，避免简单的明文比对与复用

---

## 8.6 协议设计

### 8.6.1 注册请求

`REGISTER_REQUEST` body：

```text
username      : string   (uint16 长度前缀 + UTF-8)
password_hash : fixed64  (客户端 hashForTransport 结果)
```

客户端构造逻辑见 [auth_service.cpp](/mnt/d/CloudVault/client/src/network/auth_service.cpp#L39)。

### 8.6.2 注册响应

`REGISTER_RESPONSE` body：

```text
status  : uint8
message : string
```

状态码约定：

- `0`：成功
- `1`：用户名已存在
- `2`：服务器错误或请求格式错误

### 8.6.3 登录请求

`LOGIN_REQUEST` body：

```text
username      : string
password_hash : fixed64
```

### 8.6.4 登录响应

`LOGIN_RESPONSE` body：

```text
status  : uint8
user_id : uint32
message : string
```

状态码约定：

- `0`：登录成功
- `1`：密码错误
- `2`：用户不存在
- `3`：该账号已在线
- `4`：服务器错误或请求格式错误

服务端协议解析与回包逻辑见 [auth_handler.cpp](/mnt/d/CloudVault/server/src/handler/auth_handler.cpp)。

---

## 8.7 服务端实现

### 8.7.1 Database：MySQL 连接池

[database.cpp](/mnt/d/CloudVault/server/src/db/database.cpp) 实现了一个最小可用连接池：

- 启动时预建 `pool_size` 个连接
- 借出连接时阻塞等待
- 通过 RAII `Connection` 守卫在析构时自动归还
- 使用 `mysql_ping()` 做连接活性检测

关键点：

- `mysql_library_init()` 必须在任何 MySQL 调用前执行
- 每个连接统一设置 `utf8mb4`
- 连接失败直接抛异常，启动阶段中止服务

### 8.7.2 UserRepository：用户表访问层

[user_repository.cpp](/mnt/d/CloudVault/server/src/db/user_repository.cpp) 负责 `user_info` 表操作：

- `insertUser()`：注册新用户
- `findByName()`：按用户名查询
- `setOnline()`：更新在线状态

这里统一使用 `mysql_stmt_*` 预处理语句，避免 SQL 注入，并且让字符串绑定更稳定。

`insertUser()` 返回值约定：

- `> 0`：新用户 `user_id`
- `-1`：用户名重复
- `0`：其他数据库错误

### 8.7.3 SessionManager：在线会话管理

[session_manager.cpp](/mnt/d/CloudVault/server/src/session_manager.cpp) 维护两张索引：

- `by_id_`：`user_id -> Session`
- `name_to_id_`：`username -> user_id`

这样可以快速完成：

- 检查某个用户名是否在线
- 根据用户名找到连接并发消息
- 连接断开时移除会话

### 8.7.4 AuthHandler：认证业务核心

[auth_handler.cpp](/mnt/d/CloudVault/server/src/handler/auth_handler.cpp) 完成三类消息处理：

- `handleRegister()`
- `handleLogin()`
- `handleLogout()`

#### 注册流程

1. 解析 `username` 和 64 字节传输哈希
2. 对传输哈希再次加盐：`crypto::hashPassword()`
3. 调用 `insertUser()` 写入数据库
4. 根据结果返回成功、重复用户或服务器错误

#### 登录流程

1. 解析用户名和传输哈希
2. 调用 `findByName()` 查询用户
3. 用 `crypto::verifyPassword()` 验证密码
4. 用 `sessions_.isOnline()` 防止重复登录
5. 登录成功后：
   - `sessions_.addSession(...)`
   - `users_.setOnline(uid, true)`
   - 注册连接关闭回调，在断开时移除会话并更新 `is_online = 0`

### 8.7.5 ServerApp：把认证接入总流程

[server_app.cpp](/mnt/d/CloudVault/server/src/server_app.cpp) 在 `init()` 中完成：

1. 加载配置与日志
2. 初始化数据库连接池
3. 创建 `AuthHandler`
4. 初始化网络层
5. 注册 `REGISTER_REQUEST` / `LOGIN_REQUEST` / `LOGOUT` 处理器

这一层的作用是把“数据库”和“消息分发”接起来，让服务端在网络启动前就完成依赖初始化。

---

## 8.8 客户端实现

### 8.8.1 AuthService

[auth_service.cpp](/mnt/d/CloudVault/client/src/network/auth_service.cpp) 是客户端认证协议封装层。

它负责：

- 将用户名和密码组装成 PDU
- 对密码执行 `hashForTransport()`
- 解析 `REGISTER_RESPONSE` / `LOGIN_RESPONSE`
- 向 UI 发出 Qt 信号：
  - `registerSuccess`
  - `registerFailed`
  - `loginSuccess`
  - `loginFailed`

注意这里已经做了 Windows 兼容处理：

- Windows 使用 `<winsock2.h>`
- Linux 使用 `<arpa/inet.h>`

这样 `ntohl()` / `ntohs()` 在 MinGW 和 Linux 下都能编译。

### 8.8.2 LoginWindow

[login_window.cpp](/mnt/d/CloudVault/client/src/ui/login_window.cpp) 在本章承担三件事：

1. 从配置文件读取服务器地址
2. 创建网络连接并发送 `PING`
3. 接收 `AuthService` 信号并更新 UI

当前行为：

- 启动成功后自动连接服务端
- 连通后先发送 `PING`
- 收到 `PONG` 后将状态栏标为“已连接”
- 登录/注册失败时在表单下方展示错误
- 配置文件非法时直接禁用操作按钮

---

## 8.9 联调步骤

### 8.9.1 初始化数据库

```bash
mysql -u root -P 3307 -h 127.0.0.1 -p < server/sql/init.sql
```

### 8.9.2 准备服务端配置

推荐使用环境变量方式：

```json
{
    "database": {
        "host": "127.0.0.1",
        "port": 3307,
        "name": "cloudvault",
        "user": "cloudvault_app",
        "password_env": "CLOUDVAULT_DB_PASSWORD",
        "pool_size": 8
    }
}
```

然后在启动前导出：

```bash
export CLOUDVAULT_DB_PASSWORD=cloudvault_dev_123
```

如果只是本地调试，也可以直接在本地 `server/config/server.json` 中填写 `password` 字段，但不要提交进仓库。

### 8.9.3 准备客户端配置

创建本地文件 `client/config/client.json`：

```json
{
    "server": {
        "host": "127.0.0.1",
        "port": 5001
    }
}
```

### 8.9.4 启动服务端

```bash
./cloudvault_server /path/to/server/config/server.json
```

正常日志应包含：

- `Database pool: ...`
- `初始化完成，按 Ctrl+C 关闭服务`
- `服务已启动，等待连接...`

### 8.9.5 启动客户端

```bash
cloudvault_client
```

正常日志应包含：

- `Loaded client config from ...`
- `Connecting to "127.0.0.1" : 5001`
- `Connected to server`
- `PONG received — server alive`

### 8.9.6 验证注册与登录

建议按这个顺序：

1. 输入一个不存在的用户名尝试登录，确认返回“用户名不存在”
2. 用同一用户名注册，确认返回“注册成功”
3. 再次登录，确认返回 `uid`
4. 查询数据库：

```sql
SELECT user_id, username, is_online, created_at FROM user_info;
```

5. 关闭客户端，确认服务端打印会话移除日志

---

## 8.10 本章知识点

- MySQL C API 基础用法
- 预处理语句与参数绑定
- RAII 管理数据库连接
- 客户端与服务端的双层密码哈希链路
- Qt 信号槽驱动的登录/注册状态更新
- 配置文件驱动客户端连接，而不是硬编码端口

---

## 8.11 小结

第七章解决的是“能连上”，第八章解决的是“连上以后能做事”。

到本章结束，CloudVault 已经具备：

- 真正的用户注册
- 真正的用户登录
- 持久化用户信息
- 在线状态与会话管理

下一章将在这个基础上继续实现好友体系。

---

上一章：[第七章 — PING/PONG 联通](ch07-ping-pong.md) ｜ 下一章：[第九章 — 好友功能](ch09-friend.md)
