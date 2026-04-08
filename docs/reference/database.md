# CloudVault 数据库设计文档

> 版本：v2.0 | 状态：与当前 `server/sql/init.sql` 对齐 | 数据库：MySQL 8.0

---

## 1. 概览

| 项目 | 内容 |
|------|------|
| 数据库名 | `cloudvault` |
| 字符集 | `utf8mb4` |
| 排序规则 | `utf8mb4_unicode_ci` |
| 引擎 | InnoDB |
| 默认时区 | 由 MySQL 实例决定，应用层时间戳统一按文本写入/读取 |

### 1.1 表清单

| 表名 | 用途 |
|------|------|
| `user_info` | 用户账号、密码哈希、昵称、签名、头像路径、在线状态 |
| `friend` | 好友申请与已确认好友关系 |
| `chat_group` | 群组信息 |
| `group_member` | 群成员关系 |
| `chat_message` | 私聊与群聊历史 |
| `offline_message` | 离线消息队列 |

---

## 2. 命名约定

当前代码使用以下命名，而不是旧文档中的 `id/name/online` 风格：

- 用户主键：`user_id`
- 用户名：`username`
- 在线状态：`is_online`
- 聊天消息主键：`msg_id`

---

## 3. 表定义

### 3.1 `user_info`

```sql
CREATE TABLE user_info (
    user_id       INT          NOT NULL AUTO_INCREMENT PRIMARY KEY,
    username      VARCHAR(32)  NOT NULL UNIQUE,
    password_hash VARCHAR(128) NOT NULL,
    nickname      VARCHAR(64)  DEFAULT NULL,
    signature     VARCHAR(128) DEFAULT NULL,
    avatar_path   VARCHAR(256) DEFAULT NULL,
    is_online     TINYINT      NOT NULL DEFAULT 0,
    created_at    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
                               ON UPDATE CURRENT_TIMESTAMP
);
```

说明：

- `password_hash` 存服务端加盐后的持久化哈希
- `nickname/signature/avatar_path` 为资料扩展字段
- `is_online` 由登录和连接断开逻辑维护

### 3.2 `friend`

```sql
CREATE TABLE friend (
    id          INT      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    user_id     INT      NOT NULL,
    friend_id   INT      NOT NULL,
    status      TINYINT  NOT NULL DEFAULT 0,
    created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_friendship (user_id, friend_id),
    KEY idx_friend_id (friend_id),
    CONSTRAINT fk_friend_user   FOREIGN KEY (user_id)   REFERENCES user_info(user_id) ON DELETE CASCADE,
    CONSTRAINT fk_friend_friend FOREIGN KEY (friend_id) REFERENCES user_info(user_id) ON DELETE CASCADE
);
```

说明：

- `(A, B, status=0)` 表示 A 向 B 发出了好友申请
- `status=1` 表示双方已成为好友
- 好友列表查询依赖 `UNION` 处理单向存储

### 3.3 `chat_group`

```sql
CREATE TABLE chat_group (
    id          INT         NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name        VARCHAR(64) NOT NULL,
    owner_id    INT         NOT NULL,
    created_at  DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    KEY idx_owner (owner_id),
    CONSTRAINT fk_group_owner FOREIGN KEY (owner_id) REFERENCES user_info(user_id) ON DELETE CASCADE
);
```

### 3.4 `group_member`

```sql
CREATE TABLE group_member (
    id         INT      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    group_id   INT      NOT NULL,
    user_id    INT      NOT NULL,
    joined_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_membership (group_id, user_id),
    KEY idx_user_groups (user_id),
    CONSTRAINT fk_member_group FOREIGN KEY (group_id) REFERENCES chat_group(id) ON DELETE CASCADE,
    CONSTRAINT fk_member_user  FOREIGN KEY (user_id)  REFERENCES user_info(user_id) ON DELETE CASCADE
);
```

### 3.5 `chat_message`

```sql
CREATE TABLE chat_message (
    msg_id      BIGINT      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_id   INT         NOT NULL,
    receiver_id INT         DEFAULT NULL,
    group_id    INT         DEFAULT NULL,
    content     TEXT        NOT NULL,
    created_at  DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    KEY idx_private_chat (sender_id, receiver_id, created_at),
    KEY idx_group_chat   (group_id, created_at),
    CONSTRAINT fk_msg_sender   FOREIGN KEY (sender_id)   REFERENCES user_info(user_id) ON DELETE CASCADE,
    CONSTRAINT fk_msg_receiver FOREIGN KEY (receiver_id) REFERENCES user_info(user_id) ON DELETE SET NULL,
    CONSTRAINT fk_msg_group    FOREIGN KEY (group_id)    REFERENCES chat_group(id)     ON DELETE CASCADE
);
```

说明：

- 私聊：`receiver_id` 非空，`group_id` 为空
- 群聊：`group_id` 非空，`receiver_id` 为空
- 互斥关系目前由应用层保证
- 旧版文档中的 `CHECK` 约束已去除，以兼容 MySQL 8.0 初始化

### 3.6 `offline_message`

```sql
CREATE TABLE offline_message (
    id          BIGINT      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_id   INT         NOT NULL,
    receiver_id INT         NOT NULL,
    msg_type    INT         NOT NULL,
    content     TEXT        NOT NULL,
    created_at  DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    delivered   TINYINT     NOT NULL DEFAULT 0,
    KEY idx_receiver_undelivered (receiver_id, delivered),
    KEY idx_created_at (created_at),
    CONSTRAINT fk_offline_sender   FOREIGN KEY (sender_id)   REFERENCES user_info(user_id) ON DELETE CASCADE,
    CONSTRAINT fk_offline_receiver FOREIGN KEY (receiver_id) REFERENCES user_info(user_id) ON DELETE CASCADE
);
```

说明：

- `msg_type` 保存 `MessageType` 原始值
- 私聊离线消息直接存文本内容
- 群聊离线消息当前将 `group_id + '\n' + content` 编码到 `content` 中，由应用层解包重放

---

## 4. 常用查询参考

### 4.1 查询某用户的全部好友

```sql
SELECT u.user_id, u.username, u.is_online
FROM user_info u
WHERE u.user_id IN (
    SELECT friend_id FROM friend WHERE user_id = :uid AND status = 1
    UNION
    SELECT user_id   FROM friend WHERE friend_id = :uid AND status = 1
)
ORDER BY u.username ASC;
```

### 4.2 查询私聊历史

```sql
SELECT m.sender_id, m.receiver_id, m.content, m.created_at
FROM chat_message m
WHERE ((m.sender_id = :a AND m.receiver_id = :b)
    OR (m.sender_id = :b AND m.receiver_id = :a))
ORDER BY m.created_at ASC;
```

### 4.3 查询群聊历史

```sql
SELECT m.sender_id, m.group_id, m.content, m.created_at
FROM chat_message m
WHERE m.group_id = :gid
ORDER BY m.created_at ASC;
```

### 4.4 登录时投递离线消息

```sql
SELECT id, sender_id, receiver_id, msg_type, content, created_at
FROM offline_message
WHERE receiver_id = :uid AND delivered = 0
ORDER BY created_at ASC;

UPDATE offline_message
SET delivered = 1
WHERE receiver_id = :uid AND delivered = 0;
```

### 4.5 查询群内在线成员

```sql
SELECT u.user_id, u.username
FROM user_info u
JOIN group_member gm ON u.user_id = gm.user_id
WHERE gm.group_id = :gid AND u.is_online = 1
ORDER BY u.username ASC;
```

---

## 5. 初始化脚本与专用容器

当前初始化脚本：

- `server/sql/init.sql`

项目专用 MySQL 容器：

- `docker-compose.mysql.yml`

默认专用容器参数：

| 项目 | 值 |
|------|----|
| Host | `127.0.0.1` |
| Port | `3308` |
| Database | `cloudvault` |
| App User | `cloudvault_app` |
| App Password | `cloudvault_dev_123` |
| Root Password | `cloudvault_root_123` |

---

## 6. 与旧设计的差异

| 项目 | 旧文档 | 当前实现 |
|------|--------|----------|
| 用户主键 | `id` | `user_id` |
| 用户名列 | `name` | `username` |
| 在线列 | `online` | `is_online` |
| 表数量 | 文档多次不一致 | 固定 6 张表 |
| 好友申请 | 内存 pending | `friend.status` |
| 群组 | 缺表 | `chat_group` + `group_member` 已落地 |
| 聊天记录约束 | 文档声明 `CHECK` | 当前实现去掉 `CHECK`，由应用层保证 |
