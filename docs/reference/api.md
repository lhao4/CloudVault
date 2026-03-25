# CloudVault 云巢 — 协议与 API 规范

> **版本**：PDU v2.0 | **状态**：已确认

---

## 1. 协议基础

### 1.1 传输层

| 项目 | 内容 |
|------|------|
| 传输协议 | TCP 长连接 |
| 加密 | TLS 1.2+（OpenSSL / QSslSocket） |
| 端口 | 5000（可配置） |
| 字节序 | 小端（Little-Endian） |
| 字符编码 | UTF-8 |

### 1.2 PDU v2 格式

所有消息均封装为 PDU（Protocol Data Unit），格式固定：

```
 0               4               8               12              16
 ┌───────────────┬───────────────┬───────────────┬───────────────┐
 │  total_length │  body_length  │ message_type  │   reserved    │
 │   uint32_t    │   uint32_t    │   uint32_t    │   uint32_t    │
 └───────────────┴───────────────┴───────────────┴───────────────┘
 │←────────────────── Header（16 字节）──────────────────────────►│
 ┌─────────────────────────────────────────────────────────────────┐
 │                     Body（body_length 字节）                     │
 └─────────────────────────────────────────────────────────────────┘
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `total_length` | uint32_t | 整包字节数 = 16 + body_length |
| `body_length` | uint32_t | Body 区字节数（可为 0） |
| `message_type` | uint32_t | MessageType 枚举值（见第 2 节） |
| `reserved` | uint32_t | 预留，当前填 0 |

### 1.3 Body 编码约定

**请求 Body**：直接跟各字段，无前缀状态码。

**响应 Body**：首 4 字节固定为状态码，后跟数据字段。

```
响应 Body 布局：
┌────────────────┬─────────────────────────────┐
│  status_code   │         data fields          │
│   uint32_t     │         （可选）              │
└────────────────┴─────────────────────────────┘
```

**基本类型编码**：

| 类型 | 字节数 | 说明 |
|------|--------|------|
| `uint8_t` | 1 | 布尔值 / 字节标志 |
| `uint32_t` | 4 | 整数、枚举、长度 |
| `int64_t` | 8 | 文件大小、偏移量 |
| `fixed_string(N)` | N | 定长字符串，不足 N 字节用 `\0` 填充，不含终止符也可读 |
| `length_string` | 4 + N | `uint32_t` 长度 + N 字节 UTF-8 内容 |
| `path_string` | 4 + N | 同 length_string，用于文件路径 |

> **说明**：用户名、文件名等短字段使用 `fixed_string(32)`；文件路径、消息内容等变长字段使用 `length_string`。

---

## 2. 消息类型枚举

```cpp
enum class MessageType : uint32_t {
    // 用户系统
    REGISTER_REQUEST         = 1,
    REGISTER_RESPOND         = 2,
    LOGIN_REQUEST            = 3,
    LOGIN_RESPOND            = 4,
    LOGOUT_REQUEST           = 5,
    LOGOUT_RESPOND           = 6,
    UPDATE_PROFILE_REQUEST   = 7,
    UPDATE_PROFILE_RESPOND   = 8,

    // 用户查询
    FIND_USER_REQUEST        = 11,
    FIND_USER_RESPOND        = 12,
    ONLINE_USER_REQUEST      = 13,
    ONLINE_USER_RESPOND      = 14,

    // 好友系统
    ADD_FRIEND_REQUEST       = 21,
    ADD_FRIEND_RESPOND       = 22,
    AGREE_ADD_FRIEND_REQUEST = 23,
    AGREE_ADD_FRIEND_RESPOND = 24,
    FLUSH_FRIEND_REQUEST     = 25,
    FLUSH_FRIEND_RESPOND     = 26,
    DELETE_FRIEND_REQUEST    = 27,
    DELETE_FRIEND_RESPOND    = 28,

    // 聊天
    CHAT_REQUEST             = 31,
    CHAT_RESPOND             = 32,
    GROUP_CHAT_REQUEST       = 33,
    GROUP_CHAT_RESPOND       = 34,
    GET_HISTORY_REQUEST      = 35,
    GET_HISTORY_RESPOND      = 36,

    // 群组
    CREATE_GROUP_REQUEST     = 41,
    CREATE_GROUP_RESPOND     = 42,
    JOIN_GROUP_REQUEST       = 43,
    JOIN_GROUP_RESPOND       = 44,
    LEAVE_GROUP_REQUEST      = 45,
    LEAVE_GROUP_RESPOND      = 46,
    GET_GROUP_LIST_REQUEST   = 47,
    GET_GROUP_LIST_RESPOND   = 48,

    // 文件管理
    MKDIR_REQUEST            = 51,
    MKDIR_RESPOND            = 52,
    FLUSH_FILE_REQUEST       = 53,
    FLUSH_FILE_RESPOND       = 54,
    MOVE_FILE_REQUEST        = 55,
    MOVE_FILE_RESPOND        = 56,
    DELETE_FILE_REQUEST      = 57,
    DELETE_FILE_RESPOND      = 58,
    RENAME_FILE_REQUEST      = 59,
    RENAME_FILE_RESPOND      = 60,
    SEARCH_FILE_REQUEST      = 61,
    SEARCH_FILE_RESPOND      = 62,

    // 文件传输
    UPLOAD_FILE_REQUEST      = 71,
    UPLOAD_FILE_RESPOND      = 72,
    UPLOAD_FILE_DATA         = 73,
    UPLOAD_FILE_DATA_RESPOND = 74,
    DOWNLOAD_FILE_REQUEST    = 75,
    DOWNLOAD_FILE_RESPOND    = 76,
    DOWNLOAD_FILE_DATA       = 77,
    DOWNLOAD_FILE_DATA_RESPOND = 78,

    // 文件分享
    SHARE_FILE_REQUEST       = 81,
    SHARE_FILE_RESPOND       = 82,
    SHARE_FILE_NOTIFY        = 83,   // 服务端推送给接收方
    SHARE_FILE_AGREE_REQUEST = 84,
    SHARE_FILE_AGREE_RESPOND = 85,
};
```

---

## 3. 状态码

所有响应 Body 首 4 字节为 `uint32_t status_code`：

| 值 | 常量名 | 含义 |
|----|--------|------|
| 0 | `STATUS_OK` | 成功 |
| 1 | `STATUS_ERROR` | 通用错误 |
| 2 | `STATUS_USER_NOT_FOUND` | 用户不存在 |
| 3 | `STATUS_USER_OFFLINE` | 用户不在线 |
| 4 | `STATUS_WRONG_PASSWORD` | 密码错误 |
| 5 | `STATUS_USER_EXISTS` | 用户名已存在 |
| 6 | `STATUS_ALREADY_FRIENDS` | 已是好友 |
| 7 | `STATUS_NOT_FRIENDS` | 非好友关系 |
| 8 | `STATUS_FILE_NOT_FOUND` | 文件/目录不存在 |
| 9 | `STATUS_FILE_EXISTS` | 文件/目录已存在 |
| 10 | `STATUS_PATH_DENIED` | 路径越权（路径遍历攻击） |
| 11 | `STATUS_INVALID_INPUT` | 输入格式不合法 |
| 12 | `STATUS_GROUP_NOT_FOUND` | 群组不存在 |
| 13 | `STATUS_NOT_GROUP_MEMBER` | 非群成员 |
| 14 | `STATUS_ALREADY_LOGGED_IN` | 账号已在其他地方登录 |

---

## 4. 消息详细规范

> 符号说明：`→` = 客户端发送，`←` = 服务端发送

---

### 4.1 用户系统

#### REGISTER（注册）

**→ REGISTER_REQUEST**

```
Body:
┌─────────────────────┬─────────────────────┐
│  username           │  password_hash       │
│  fixed_string(32)   │  fixed_string(64)    │
└─────────────────────┴─────────────────────┘
```

> `password_hash`：客户端先用 `crypto_utils::hash(password)` 计算 SHA-256，发送 64 字节十六进制字符串。

**← REGISTER_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │  0=成功, 5=用户名已存在, 11=输入不合法
│  uint32_t     │
└───────────────┘
```

---

#### LOGIN（登录）

**→ LOGIN_REQUEST**

```
Body:
┌─────────────────────┬─────────────────────┐
│  username           │  password_hash       │
│  fixed_string(32)   │  fixed_string(64)    │
└─────────────────────┴─────────────────────┘
```

**← LOGIN_RESPOND**

```
Body:
┌───────────────┬─────────────────────┐
│  status_code  │  username           │  ← status=0 时填充，其余为空
│  uint32_t     │  fixed_string(32)   │
└───────────────┴─────────────────────┘
```

> 登录成功后服务端：① 在 SessionManager 注册会话；② 更新 DB online=1；③ 投递离线消息。

---

#### LOGOUT（登出）

**→ LOGOUT_REQUEST**

```
Body: 空（body_length = 0）
```

**← LOGOUT_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │  固定 0
│  uint32_t     │
└───────────────┘
```

---

#### UPDATE_PROFILE（更新个人信息）

**→ UPDATE_PROFILE_REQUEST**

```
Body:
┌───────────────┬─────────────────────┬─────────────────────┐
│  nickname_len │  nickname           │  signature_len ...   │
│  uint32_t     │  N 字节             │  length_string       │
└───────────────┴─────────────────────┴─────────────────────┘
完整布局：
[ length_string: nickname ][ length_string: signature ]
```

**← UPDATE_PROFILE_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │
└───────────────┘
```

---

### 4.2 用户查询

#### FIND_USER（查找用户）

**→ FIND_USER_REQUEST**

```
Body:
┌─────────────────────┐
│  username           │
│  fixed_string(32)   │
└─────────────────────┘
```

**← FIND_USER_RESPOND**

```
Body:
┌───────────────┬─────────────────────┬──────────────┐
│  status_code  │  username           │  online      │
│  uint32_t     │  fixed_string(32)   │  uint8_t     │
└───────────────┴─────────────────────┴──────────────┘
```

> `status_code`：0=找到，2=用户不存在

---

#### ONLINE_USER（在线用户列表）

**→ ONLINE_USER_REQUEST**

```
Body: 空
```

**← ONLINE_USER_RESPOND**

```
Body:
┌───────────────┬───────────────┬──────────────────────────────────┐
│  status_code  │  user_count   │  users[user_count]               │
│  uint32_t     │  uint32_t     │  每项 fixed_string(32)           │
└───────────────┴───────────────┴──────────────────────────────────┘
```

---

### 4.3 好友系统

#### ADD_FRIEND（发送好友申请）

**→ ADD_FRIEND_REQUEST**

```
Body:
┌─────────────────────┬─────────────────────┐
│  from_user          │  to_user             │
│  fixed_string(32)   │  fixed_string(32)    │
└─────────────────────┴─────────────────────┘
```

**← ADD_FRIEND_RESPOND**（返回给发起方）

```
Body:
┌───────────────┐
│  status_code  │  0=请求已发出, 2=用户不存在, 3=用户不在线, 6=已是好友
└───────────────┘
```

**← ADD_FRIEND_REQUEST**（服务端转发给目标方，同一消息类型）

```
Body:
┌─────────────────────┬─────────────────────┐
│  from_user          │  to_user             │
│  fixed_string(32)   │  fixed_string(32)    │
└─────────────────────┴─────────────────────┘
```

---

#### AGREE_ADD_FRIEND（同意好友申请）

**→ AGREE_ADD_FRIEND_REQUEST**

```
Body:
┌─────────────────────┬─────────────────────┐
│  from_user          │  to_user             │  ← from=同意方, to=发起方
│  fixed_string(32)   │  fixed_string(32)    │
└─────────────────────┴─────────────────────┘
```

**← AGREE_ADD_FRIEND_RESPOND**（发送给双方）

```
Body:
┌───────────────┐
│  status_code  │  0=成功, 1=失败
└───────────────┘
```

---

#### FLUSH_FRIEND（刷新好友列表）

**→ FLUSH_FRIEND_REQUEST**

```
Body:
┌─────────────────────┐
│  username           │
│  fixed_string(32)   │
└─────────────────────┘
```

**← FLUSH_FRIEND_RESPOND**

```
Body:
┌───────────────┬───────────────┬──────────────────────────────────┐
│  status_code  │ friend_count  │  friends[friend_count]           │
│  uint32_t     │  uint32_t     │  每项 fixed_string(32) + uint8_t │
└───────────────┴───────────────┴──────────────────────────────────┘

每项好友条目（33 字节）：
┌─────────────────────┬──────────┐
│  username           │  online  │
│  fixed_string(32)   │ uint8_t  │
└─────────────────────┴──────────┘
```

---

#### DELETE_FRIEND（删除好友）

**→ DELETE_FRIEND_REQUEST**

```
Body:
┌─────────────────────┬─────────────────────┐
│  username           │  friend_name         │
│  fixed_string(32)   │  fixed_string(32)    │
└─────────────────────┴─────────────────────┘
```

**← DELETE_FRIEND_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │  0=成功, 7=非好友, 1=失败
└───────────────┘
```

---

### 4.4 聊天

#### CHAT（1 对 1 聊天）

**→ CHAT_REQUEST**

```
Body:
┌─────────────────────┬─────────────────────┬──────────────────────┐
│  from_user          │  to_user             │  message             │
│  fixed_string(32)   │  fixed_string(32)    │  length_string       │
└─────────────────────┴─────────────────────┴──────────────────────┘
```

**← CHAT_RESPOND**（服务端转发给接收方，结构同请求）

```
Body:
┌─────────────────────┬─────────────────────┬──────────────────────┐
│  from_user          │  to_user             │  message             │
│  fixed_string(32)   │  fixed_string(32)    │  length_string       │
└─────────────────────┴─────────────────────┴──────────────────────┘
```

> 服务端不向发送方回复确认，仅转发给目标方（在线）或存入 offline_message（离线）。

---

#### GROUP_CHAT（群聊）

**→ GROUP_CHAT_REQUEST**

```
Body:
┌─────────────────────┬───────────────┬──────────────────────┐
│  from_user          │  group_id     │  message             │
│  fixed_string(32)   │  uint32_t     │  length_string       │
└─────────────────────┴───────────────┴──────────────────────┘
```

**← GROUP_CHAT_RESPOND**（服务端广播给所有在线群成员）

```
Body:
┌─────────────────────┬───────────────┬──────────────────────┐
│  from_user          │  group_id     │  message             │
│  fixed_string(32)   │  uint32_t     │  length_string       │
└─────────────────────┴───────────────┴──────────────────────┘
```

---

#### GET_HISTORY（获取聊天记录）

**→ GET_HISTORY_REQUEST**

```
Body:
┌─────────────────────┬───────────────┬───────────────┬───────────────┐
│  peer_name / 空     │  group_id / 0 │  page_size    │  offset       │
│  fixed_string(32)   │  uint32_t     │  uint32_t     │  uint32_t     │
└─────────────────────┴───────────────┴───────────────┴───────────────┘
```

> `peer_name` 非空则查询私聊记录；`group_id` 非 0 则查询群聊记录；两者互斥。

**← GET_HISTORY_RESPOND**

```
Body:
┌───────────────┬───────────────┬──────────────────────────────────────────────┐
│  status_code  │  msg_count    │  messages[msg_count]                         │
└───────────────┴───────────────┴──────────────────────────────────────────────┘

每条消息条目：
┌─────────────────────┬──────────────────────┬──────────────────────┐
│  sender_name        │  timestamp_ms        │  content             │
│  fixed_string(32)   │  int64_t             │  length_string       │
└─────────────────────┴──────────────────────┴──────────────────────┘
```

---

### 4.5 群组管理

#### CREATE_GROUP（创建群）

**→ CREATE_GROUP_REQUEST**

```
Body:
┌─────────────────────┬──────────────────────┐
│  owner_name         │  group_name           │
│  fixed_string(32)   │  fixed_string(64)     │
└─────────────────────┴──────────────────────┘
```

**← CREATE_GROUP_RESPOND**

```
Body:
┌───────────────┬───────────────┐
│  status_code  │  group_id     │  ← 成功时返回新群 ID
│  uint32_t     │  uint32_t     │
└───────────────┴───────────────┘
```

---

#### JOIN_GROUP / LEAVE_GROUP

**→ JOIN_GROUP_REQUEST / LEAVE_GROUP_REQUEST**

```
Body:
┌─────────────────────┬───────────────┐
│  username           │  group_id     │
│  fixed_string(32)   │  uint32_t     │
└─────────────────────┴───────────────┘
```

**← JOIN_GROUP_RESPOND / LEAVE_GROUP_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │
└───────────────┘
```

---

#### GET_GROUP_LIST（获取群列表）

**→ GET_GROUP_LIST_REQUEST**

```
Body:
┌─────────────────────┐
│  username           │
│  fixed_string(32)   │
└─────────────────────┘
```

**← GET_GROUP_LIST_RESPOND**

```
Body:
┌───────────────┬───────────────┬────────────────────────────────────────────┐
│  status_code  │  group_count  │  groups[group_count]                       │
└───────────────┴───────────────┴────────────────────────────────────────────┘

每项群条目（72 字节）：
┌───────────────┬──────────────────────┐
│  group_id     │  group_name          │
│  uint32_t     │  fixed_string(64)    │
└───────────────┴──────────────────────┘
```

---

### 4.6 文件管理

#### MKDIR（新建文件夹）

**→ MKDIR_REQUEST**

```
Body:
┌──────────────────────┬──────────────────────┐
│  dir_name            │  parent_path          │
│  fixed_string(32)    │  path_string          │
└──────────────────────┴──────────────────────┘
```

**← MKDIR_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │  0=成功, 9=已存在, 10=路径越权
└───────────────┘
```

---

#### FLUSH_FILE（列目录）

**→ FLUSH_FILE_REQUEST**

```
Body:
┌──────────────────────┐
│  dir_path            │
│  path_string         │
└──────────────────────┘
```

**← FLUSH_FILE_RESPOND**

```
Body:
┌───────────────┬───────────────┬──────────────────────────────────────────┐
│  status_code  │  file_count   │  files[file_count]                       │
└───────────────┴───────────────┴──────────────────────────────────────────┘

每项文件条目（37 字节）：
┌──────────────────────┬──────────────┐
│  filename            │  file_type   │
│  fixed_string(32)    │  uint32_t    │  0=目录, 1=普通文件
└──────────────────────┴──────────────┘
```

---

#### MOVE_FILE（移动文件）

**→ MOVE_FILE_REQUEST**

```
Body:
┌──────────────────────┬──────────────────────┐
│  src_path            │  dst_path             │
│  path_string         │  path_string          │
└──────────────────────┴──────────────────────┘
```

**← MOVE_FILE_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │  0=成功, 8=源路径不存在, 9=目标已存在, 10=路径越权
└───────────────┘
```

---

#### DELETE_FILE（删除文件/文件夹）

**→ DELETE_FILE_REQUEST**

```
Body:
┌──────────────────────┐
│  target_path         │
│  path_string         │
└──────────────────────┘
```

**← DELETE_FILE_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │  0=成功, 8=不存在, 10=路径越权
└───────────────┘
```

---

#### RENAME_FILE（重命名）

**→ RENAME_FILE_REQUEST**

```
Body:
┌──────────────────────┬──────────────────────┐
│  target_path         │  new_name             │
│  path_string         │  fixed_string(32)     │
└──────────────────────┴──────────────────────┘
```

**← RENAME_FILE_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │  0=成功, 8=不存在, 9=新名已存在, 10=路径越权
└───────────────┘
```

---

#### SEARCH_FILE（搜索文件）

**→ SEARCH_FILE_REQUEST**

```
Body:
┌─────────────────────┬──────────────────────┐
│  username           │  keyword              │
│  fixed_string(32)   │  length_string        │
└─────────────────────┴──────────────────────┘
```

**← SEARCH_FILE_RESPOND**

```
Body:
┌───────────────┬───────────────┬──────────────────────────────────────────┐
│  status_code  │  result_count │  results[result_count]                   │
└───────────────┴───────────────┴──────────────────────────────────────────┘

每项结果（path_string 变长）：
┌──────────────────────┐
│  file_path           │  相对于用户根目录的完整路径
│  path_string         │
└──────────────────────┘
```

---

### 4.7 文件上传

#### UPLOAD_FILE（上传初始化）

**→ UPLOAD_FILE_REQUEST**

```
Body:
┌──────────────────────┬───────────────┬──────────────────────┐
│  filename            │  file_size    │  dir_path             │
│  fixed_string(32)    │  int64_t      │  path_string          │
└──────────────────────┴───────────────┴──────────────────────┘
```

**← UPLOAD_FILE_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │  0=服务端就绪, 9=文件已存在, 10=路径越权
└───────────────┘
```

---

#### UPLOAD_FILE_DATA（上传数据块）

**→ UPLOAD_FILE_DATA**（循环发送，无需等待每块确认）

```
Body:
┌───────────────┬──────────────────────────────────────┐
│  chunk_size   │  chunk_data                          │
│  uint32_t     │  chunk_size 字节                     │
└───────────────┴──────────────────────────────────────┘
```

> `chunk_size` ≤ 4MB（4 × 1024 × 1024 字节）。

**← UPLOAD_FILE_DATA_RESPOND**（仅在全部数据接收完毕后发送一次）

```
Body:
┌───────────────┐
│  status_code  │  0=上传成功, 1=写入失败
└───────────────┘
```

---

### 4.8 文件下载

#### DOWNLOAD_FILE（下载初始化）

**→ DOWNLOAD_FILE_REQUEST**

```
Body:
┌──────────────────────┐
│  file_path           │
│  path_string         │
└──────────────────────┘
```

**← DOWNLOAD_FILE_RESPOND**

```
Body:
┌───────────────┬───────────────┬──────────────────────┐
│  status_code  │  file_size    │  filename             │
│  uint32_t     │  int64_t      │  fixed_string(32)     │
└───────────────┴───────────────┴──────────────────────┘
```

---

#### DOWNLOAD_FILE_DATA（数据推送）

**← DOWNLOAD_FILE_DATA**（服务端连续发送，直到文件结束）

```
Body:
┌───────────────┬──────────────────────────────────────┐
│  chunk_size   │  chunk_data                          │
│  uint32_t     │  chunk_size 字节                     │
└───────────────┴──────────────────────────────────────┘
```

**→ DOWNLOAD_FILE_DATA_RESPOND**（客户端接收完毕后发送一次）

```
Body:
┌───────────────┐
│  status_code  │  0=成功接收, 1=写入本地失败
└───────────────┘
```

---

### 4.9 文件分享

#### SHARE_FILE（发起分享）

**→ SHARE_FILE_REQUEST**

```
Body:
┌─────────────────────┬───────────────┬──────────────────────────────────────┬──────────────────────┐
│  from_user          │  friend_count │  friends[friend_count]               │  file_path           │
│  fixed_string(32)   │  uint32_t     │  每项 fixed_string(32)               │  path_string         │
└─────────────────────┴───────────────┴──────────────────────────────────────┴──────────────────────┘
```

**← SHARE_FILE_RESPOND**（返回给发送方）

```
Body:
┌───────────────┐
│  status_code  │  0=分享请求已发出
└───────────────┘
```

**← SHARE_FILE_NOTIFY**（服务端推送给每个接收方）

```
Body:
┌─────────────────────┬──────────────────────┐
│  from_user          │  file_path            │  ← 源文件路径
│  fixed_string(32)   │  path_string          │
└─────────────────────┴──────────────────────┘
```

---

#### SHARE_FILE_AGREE（接受分享）

**→ SHARE_FILE_AGREE_REQUEST**

```
Body:
┌─────────────────────┬──────────────────────┐
│  receiver_name      │  src_file_path        │  ← 源文件路径（来自 NOTIFY）
│  fixed_string(32)   │  path_string          │
└─────────────────────┴──────────────────────┘
```

**← SHARE_FILE_AGREE_RESPOND**

```
Body:
┌───────────────┐
│  status_code  │  0=复制成功, 8=源文件不存在, 1=复制失败
└───────────────┘
```

---

## 5. 连接生命周期

```
客户端                                       服务端
   │                                            │
   ├── TCP 连接（TLS 握手）──────────────────► │
   │                                            │
   ├── LOGIN_REQUEST ───────────────────────► │
   │  ◄────────────────────── LOGIN_RESPOND ── │
   │                                            │
   │       （正常业务通信）                     │
   │  ◄──────────────── 离线消息批量投递 ─────  │
   │                                            │
   ├── LOGOUT_REQUEST ──────────────────────► │
   │  ◄───────────────────── LOGOUT_RESPOND ── │
   │                                            │
   ├── TCP 断开 ─────────────────────────────► │
   │                             清理会话       │
   │                             online=0       │
   │                                            │
   │  （或异常断开，服务端检测 EPOLLRDHUP 后清理）
```

---

## 6. 错误处理约定

1. **客户端收到非预期消息类型**：记录日志，丢弃，不断开连接
2. **服务端收到格式非法的 PDU**（`total_length` 超过限制、`body_length` 与 `total_length` 不符）：关闭该连接，记录 WARN 日志
3. **最大单包大小**：`total_length` ≤ 64MB；超过则视为攻击，直接断开
4. **超时**：客户端连接建立后 30 秒内未发送 `LOGIN_REQUEST` → 服务端主动断开

---

## 7. 版本兼容

`reserved` 字段预留用于未来版本协商。当前版本填 0，服务端收到非 0 值时记录日志但不断开（向前兼容）。
