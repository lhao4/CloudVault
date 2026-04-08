# CloudVault 协议与 API 规范

> 版本：v2.0 | 状态：与当前代码对齐

---

## 1. 协议基础

### 1.1 传输层

| 项目 | 内容 |
|------|------|
| 传输协议 | TCP 长连接 |
| 字节序 | 网络字节序（大端） |
| 字符编码 | UTF-8 |
| 默认服务端口 | 5000/5001（由配置文件决定） |
| TLS | 暂未实现，当前仅保留配置占位 |

### 1.2 PDU 格式

所有消息均使用固定 16 字节头：

```text
0               4               8               12              16
┌───────────────┬───────────────┬───────────────┬───────────────┐
│ total_length  │ body_length   │ message_type  │ reserved      │
│ uint32_t      │ uint32_t      │ uint32_t      │ uint32_t      │
└───────────────┴───────────────┴───────────────┴───────────────┘
│←──────────────────── Header（16 字节）────────────────────────→│
┌───────────────────────────────────────────────────────────────┐
│ Body（body_length 字节）                                      │
└───────────────────────────────────────────────────────────────┘
```

### 1.3 Body 编码约定

当前代码统一使用 `PDUBuilder` / `PDUParser`：

| 类型 | 编码 |
|------|------|
| `uint8_t` | 1 字节 |
| `uint16_t` | 2 字节，大端 |
| `uint32_t` | 4 字节，大端 |
| `string` | `uint16_t length + UTF-8 bytes` |
| `fixed_string(N)` | 固定 N 字节，不足补 0 |

说明：

- 当前响应体**不是**统一的 `uint32_t status_code + payload` 模式。
- 大多数响应采用 `uint8_t status + string message + 业务字段`。
- 具体布局以对应 handler/service 中的 `PDUBuilder` 调用顺序为准。

---

## 2. MessageType 枚举

以下枚举值与 `common/include/common/protocol.h` 保持一致：

```cpp
enum class MessageType : uint32_t {
    PING  = 0x0001,
    PONG  = 0x0002,

    LOGIN_REQUEST            = 0x0101,
    LOGIN_RESPONSE           = 0x0102,
    REGISTER_REQUEST         = 0x0103,
    REGISTER_RESPONSE        = 0x0104,
    LOGOUT                   = 0x0105,
    UPDATE_PROFILE_REQUEST   = 0x0106,
    UPDATE_PROFILE_RESPONSE  = 0x0107,

    FIND_USER_REQUEST        = 0x0201,
    FIND_USER_RESPONSE       = 0x0202,
    ADD_FRIEND_REQUEST       = 0x0203,
    ADD_FRIEND_RESPONSE      = 0x0204,
    FRIEND_REQUEST_PUSH      = 0x0205,
    AGREE_FRIEND_REQUEST     = 0x0206,
    AGREE_FRIEND_RESPONSE    = 0x0207,
    FRIEND_ADDED_PUSH        = 0x0208,
    FLUSH_FRIENDS_REQUEST    = 0x0209,
    FLUSH_FRIENDS_RESPONSE   = 0x020A,
    DELETE_FRIEND_REQUEST    = 0x020B,
    DELETE_FRIEND_RESPONSE   = 0x020C,
    FRIEND_DELETED_PUSH      = 0x020D,
    ONLINE_USER_REQUEST      = 0x020E,
    ONLINE_USER_RESPONSE     = 0x020F,

    CHAT                     = 0x0301,
    GROUP_CHAT               = 0x0302,
    GET_HISTORY              = 0x0303,

    MKDIR                    = 0x0401,
    FLUSH_FILE               = 0x0402,
    MOVE                     = 0x0403,
    DELETE_FILE              = 0x0404,
    RENAME                   = 0x0405,
    SEARCH_FILE              = 0x0406,

    UPLOAD_REQUEST           = 0x0501,
    UPLOAD_DATA              = 0x0502,
    DOWNLOAD_REQUEST         = 0x0503,
    DOWNLOAD_DATA            = 0x0504,

    SHARE_REQUEST            = 0x0601,
    SHARE_NOTIFY             = 0x0602,
    SHARE_AGREE_REQUEST      = 0x0603,
    SHARE_AGREE_RESPOND      = 0x0604,

    CREATE_GROUP_REQUEST     = 0x0701,
    CREATE_GROUP_RESPONSE    = 0x0702,
    JOIN_GROUP_REQUEST       = 0x0703,
    JOIN_GROUP_RESPONSE      = 0x0704,
    LEAVE_GROUP_REQUEST      = 0x0705,
    LEAVE_GROUP_RESPONSE     = 0x0706,
    GET_GROUP_LIST_REQUEST   = 0x0707,
    GET_GROUP_LIST_RESPONSE  = 0x0708,
};
```

说明：

- `CHAT`、`GROUP_CHAT`、`GET_HISTORY` 采用“请求和推送/响应复用同一类型”的模式。
- `ONLINE_USER_*` 已保留在协议头文件中，但当前服务端尚未注册实现。

---

## 3. 已实现消息体规范

以下为当前代码已落地且在服务端注册的消息。

### 3.1 注册

`REGISTER_REQUEST`

```text
string username
fixed_string(64) password_hash
```

`REGISTER_RESPONSE`

```text
uint8  status
string message
```

状态：

- `0` 注册成功
- `1` 用户名已被占用
- `2` 请求格式错误或服务器内部错误

### 3.2 登录

`LOGIN_REQUEST`

```text
string username
fixed_string(64) password_hash
```

`LOGIN_RESPONSE`

```text
uint8  status
uint32 user_id
string message
```

状态：

- `0` 成功
- `1` 密码错误
- `2` 用户不存在
- `3` 已在线
- `4` 请求格式错误或服务器错误

### 3.3 退出登录

`LOGOUT`

```text
Body 为空
```

当前代码中 `LOGOUT` 无单独响应包。

### 3.4 更新个人资料

`UPDATE_PROFILE_REQUEST`

```text
string nickname
string signature
```

`UPDATE_PROFILE_RESPONSE`

```text
uint8  status
string message
```

### 3.5 查找用户

`FIND_USER_REQUEST`

```text
string username
```

`FIND_USER_RESPONSE`

```text
uint8  status
string username   // 仅 status=0 时有效
uint8  is_online  // 仅 status=0 时有效
```

### 3.6 好友申请 / 同意 / 列表 / 删除

`ADD_FRIEND_REQUEST`

```text
string target_username
```

`ADD_FRIEND_RESPONSE`

```text
uint8  status
string message
```

`FRIEND_REQUEST_PUSH`

```text
string from_username
```

`AGREE_FRIEND_REQUEST`

```text
string requester_username
```

`AGREE_FRIEND_RESPONSE`

```text
uint8  status
string message
```

`FRIEND_ADDED_PUSH`

```text
string username
```

`FLUSH_FRIENDS_REQUEST`

```text
Body 为空
```

`FLUSH_FRIENDS_RESPONSE`

```text
uint8  status
uint16 friend_count
repeat friend_count times:
  string username
  uint8  is_online
```

`DELETE_FRIEND_REQUEST`

```text
string friend_username
```

`DELETE_FRIEND_RESPONSE`

```text
uint8  status
string message
```

`FRIEND_DELETED_PUSH`

```text
string username
```

### 3.7 私聊

`CHAT` 请求：

```text
string target_username
string content
```

`CHAT` 推送：

```text
string from_username
string to_username
string content
string created_at
```

说明：

- 服务端不向发送方返回单独确认包。
- 对方离线时写入 `offline_message`，登录后重放为同结构 `CHAT` 推送。

### 3.8 私聊历史

`GET_HISTORY` 请求：

```text
string peer_username
```

`GET_HISTORY` 响应：

```text
uint8  status
string peer_username

// status = 0:
uint16 count
repeat count times:
  string sender_username
  string receiver_username
  string content
  string created_at

// status != 0:
string message
```

说明：

- 当前 `GET_HISTORY` 仅公开支持私聊历史。
- 群聊历史已在服务端仓储层实现，但尚未定义独立对外协议。

### 3.9 群组管理

`CREATE_GROUP_REQUEST`

```text
string group_name
```

`CREATE_GROUP_RESPONSE`

```text
uint8  status
uint32 group_id
string group_name
string message
```

`JOIN_GROUP_REQUEST` / `LEAVE_GROUP_REQUEST`

```text
uint32 group_id
```

`JOIN_GROUP_RESPONSE` / `LEAVE_GROUP_RESPONSE`

```text
uint8  status
uint32 group_id
string message
```

`GET_GROUP_LIST_REQUEST`

```text
Body 为空
```

`GET_GROUP_LIST_RESPONSE`

```text
uint8  status
uint16 group_count
repeat group_count times:
  uint32 group_id
  string group_name
  uint32 owner_id
  uint16 online_count
```

### 3.10 群聊

`GROUP_CHAT` 请求：

```text
uint32 group_id
string content
```

`GROUP_CHAT` 推送：

```text
uint32 group_id
string from_username
string content
string created_at
```

说明：

- 当前群聊消息会广播给在线成员，并为离线成员写入离线队列。
- 离线群消息在登录后按 `GROUP_CHAT` 推送重放。

---

## 4. 文件与分享协议

文件管理、上传下载、分享消息仍沿用 `protocol.h` 中的枚举值，并由：

- `server/src/handler/file_handler.cpp`
- `server/src/handler/share_handler.cpp`

作为实现来源。

由于这些消息体较多且现阶段未发生枚举级别变更，本文不再保留旧版的过时固定字符串布局描述；需要确认具体字段顺序时，应以对应 handler 中的 `PDUBuilder` / `PDUParser` 实现为准。

---

## 5. 实现状态说明

| 能力 | 状态 |
|------|------|
| 注册 / 登录 / 登出 | 已实现 |
| 个人资料同步 | 已实现 |
| 查找用户 | 已实现 |
| 好友申请、同意、删除、刷新列表 | 已实现 |
| 在线用户列表 | 仅保留枚举，未注册实现 |
| 私聊消息与离线投递 | 已实现 |
| 私聊历史查询 | 已实现 |
| 群组创建、加入、退出、列表 | 已实现 |
| 群聊实时消息与离线投递 | 已实现 |
| 群聊历史对外协议 | 尚未完成 |
| TLS 加密 | 尚未实现 |
