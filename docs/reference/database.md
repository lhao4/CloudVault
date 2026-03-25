# CloudVault 云巢 — 数据库设计文档

> **版本**：v2.0 | **状态**：已确认 | **数据库**：MySQL 8.0

---

## 1. 概览

| 项目 | 内容 |
|------|------|
| 数据库名 | `cloudvault` |
| 字符集 | `utf8mb4` |
| 排序规则 | `utf8mb4_unicode_ci` |
| 引擎 | InnoDB（全表） |
| 时区 | UTC |

### 1.1 表清单

| 表名 | 用途 |
|------|------|
| `user_info` | 用户账号、密码哈希、在线状态、个人信息 |
| `friend` | 好友关系（含申请状态） |
| `chat_group` | 群组信息 |
| `group_member` | 群成员关系 |
| `chat_message` | 聊天记录（1对1 + 群聊） |
| `offline_message` | 离线消息队列 |

---

## 2. ER 图

```
┌───────────────────┐        ┌──────────────────────┐
│     user_info     │        │       friend         │
│───────────────────│        │──────────────────────│
│ id (PK)           │◄──────►│ id (PK)              │
│ name              │  1:N   │ user_id (FK→user)    │
│ password_hash     │        │ friend_id (FK→user)  │
│ avatar_path       │        │ status               │
│ nickname          │        │ created_at           │
│ signature         │        └──────────────────────┘
│ online            │
│ created_at        │        ┌──────────────────────┐
│ updated_at        │        │     chat_group       │
└───────────────────┘        │──────────────────────│
         │                   │ id (PK)              │
         │ 1:N               │ name                 │
         ▼                   │ owner_id (FK→user)   │
┌───────────────────┐        │ created_at           │
│   group_member    │        └──────────────────────┘
│───────────────────│                 │
│ id (PK)           │◄────────────────┘ 1:N
│ group_id (FK)     │
│ user_id (FK)      │
│ joined_at         │
└───────────────────┘

┌───────────────────────────────────────┐
│             chat_message              │
│───────────────────────────────────────│
│ id (PK)                               │
│ sender_id    (FK→user)                │
│ receiver_id  (FK→user, nullable)      │  ← 1对1
│ group_id     (FK→chat_group,nullable) │  ← 群聊
│ content                               │
│ created_at                            │
└───────────────────────────────────────┘

┌───────────────────────────────────────┐
│           offline_message             │
│───────────────────────────────────────│
│ id (PK)                               │
│ sender_id   (FK→user)                 │
│ receiver_id (FK→user)                 │
│ msg_type    (MessageType 枚举值)       │
│ content                               │
│ created_at                            │
│ delivered                             │
└───────────────────────────────────────┘
```

---

## 3. 表定义

### 3.1 user_info — 用户表

```sql
CREATE TABLE user_info (
    id            INT          NOT NULL AUTO_INCREMENT,
    name          VARCHAR(32)  NOT NULL COMMENT '登录用户名，唯一',
    password_hash VARCHAR(97)  NOT NULL COMMENT 'hex(salt):hex(SHA-256(salt+pwd))，共97字符',
    nickname      VARCHAR(64)  DEFAULT NULL COMMENT '昵称，可为空（显示时回退到 name）',
    signature     VARCHAR(128) DEFAULT NULL COMMENT '个性签名',
    avatar_path   VARCHAR(256) DEFAULT NULL COMMENT '头像文件路径（相对于用户根目录）',
    online        TINYINT      NOT NULL DEFAULT 0 COMMENT '0=离线, 1=在线',
    created_at    TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at    TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
                               ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户账号表';
```

**字段说明**：

| 字段 | 说明 |
|------|------|
| `name` | 登录名，注册后不可修改，最长 32 字节 |
| `password_hash` | 格式：`{32字节hex_salt}:{64字节hex_hash}`，总长 97 字符 |
| `nickname` | 可修改的显示名，为空时 UI 显示 `name` |
| `avatar_path` | 相对于该用户文件根目录的路径，如 `__profile__/avatar.png` |
| `online` | 服务端在登录/断开时维护；服务异常崩溃后需通过启动脚本重置为 0 |

---

### 3.2 friend — 好友关系表

```sql
CREATE TABLE friend (
    id         INT       NOT NULL AUTO_INCREMENT,
    user_id    INT       NOT NULL COMMENT '发起方用户 ID',
    friend_id  INT       NOT NULL COMMENT '接受方用户 ID',
    status     TINYINT   NOT NULL DEFAULT 0
                         COMMENT '0=待接受, 1=已添加',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_friendship (user_id, friend_id),
    KEY idx_friend_id (friend_id),
    CONSTRAINT fk_friend_user   FOREIGN KEY (user_id)   REFERENCES user_info(id) ON DELETE CASCADE,
    CONSTRAINT fk_friend_friend FOREIGN KEY (friend_id) REFERENCES user_info(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='好友关系表';
```

**设计说明**：

- 好友关系以**单向记录**存储：A 添加 B → 插入 `(A, B, 1)` 一条记录
- 查询"A 的好友列表"需用 `user_id=A OR friend_id=A` 的 UNION 查询
- `status=0` 表示 A 已发送申请但 B 尚未同意；`status=1` 表示双方互为好友
- `ON DELETE CASCADE`：用户注销时自动清理好友记录

---

### 3.3 chat_group — 群组表

```sql
CREATE TABLE chat_group (
    id         INT          NOT NULL AUTO_INCREMENT,
    name       VARCHAR(64)  NOT NULL COMMENT '群名称',
    owner_id   INT          NOT NULL COMMENT '群主用户 ID',
    created_at TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_owner (owner_id),
    CONSTRAINT fk_group_owner FOREIGN KEY (owner_id) REFERENCES user_info(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='群组表';
```

**设计说明**：

- `owner_id` 关联 `user_info.id`
- 群主退出时，服务端逻辑层负责解散群（删除 `chat_group` 记录，级联删除 `group_member`）
- v2 不支持转让群主，群主退出即解散

---

### 3.4 group_member — 群成员表

```sql
CREATE TABLE group_member (
    id         INT       NOT NULL AUTO_INCREMENT,
    group_id   INT       NOT NULL COMMENT '群组 ID',
    user_id    INT       NOT NULL COMMENT '成员用户 ID',
    joined_at  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_membership (group_id, user_id),
    KEY idx_user_groups (user_id),
    CONSTRAINT fk_member_group FOREIGN KEY (group_id) REFERENCES chat_group(id) ON DELETE CASCADE,
    CONSTRAINT fk_member_user  FOREIGN KEY (user_id)  REFERENCES user_info(id)  ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='群成员关系表';
```

**设计说明**：

- 群主本身也需插入 `group_member` 记录（创建群时同步插入）
- `ON DELETE CASCADE`：群解散时自动清理成员记录；用户注销时自动退出所有群

---

### 3.5 chat_message — 聊天记录表

```sql
CREATE TABLE chat_message (
    id          BIGINT       NOT NULL AUTO_INCREMENT,
    sender_id   INT          NOT NULL COMMENT '发送方用户 ID',
    receiver_id INT          DEFAULT NULL COMMENT '接收方用户 ID（1对1消息）',
    group_id    INT          DEFAULT NULL COMMENT '群组 ID（群聊消息）',
    content     TEXT         NOT NULL COMMENT '消息内容',
    created_at  TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) COMMENT '毫秒精度时间戳',
    PRIMARY KEY (id),
    KEY idx_private_chat (sender_id, receiver_id, created_at),
    KEY idx_group_chat   (group_id, created_at),
    CONSTRAINT fk_msg_sender   FOREIGN KEY (sender_id)   REFERENCES user_info(id)  ON DELETE CASCADE,
    CONSTRAINT fk_msg_receiver FOREIGN KEY (receiver_id) REFERENCES user_info(id)  ON DELETE SET NULL,
    CONSTRAINT fk_msg_group    FOREIGN KEY (group_id)    REFERENCES chat_group(id) ON DELETE CASCADE,
    CONSTRAINT chk_msg_target  CHECK (
        (receiver_id IS NOT NULL AND group_id IS NULL) OR
        (receiver_id IS NULL     AND group_id IS NOT NULL)
    )
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='聊天记录表（1对1 + 群聊）';
```

**设计说明**：

- `receiver_id` 和 `group_id` 互斥（`CHECK` 约束保证），区分私聊和群聊
- `created_at` 使用毫秒精度，支持消息排序
- `receiver_id ON DELETE SET NULL`：接收方注销后消息保留（发送方可查看历史），但接收方 ID 置空
- `id` 使用 `BIGINT`，支持大量消息

---

### 3.6 offline_message — 离线消息表

```sql
CREATE TABLE offline_message (
    id          BIGINT    NOT NULL AUTO_INCREMENT,
    sender_id   INT       NOT NULL COMMENT '发送方用户 ID',
    receiver_id INT       NOT NULL COMMENT '接收方用户 ID',
    msg_type    INT       NOT NULL COMMENT 'MessageType 枚举值',
    content     TEXT      DEFAULT NULL COMMENT '消息内容（文本消息）或 NULL（系统通知）',
    created_at  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    delivered   TINYINT   NOT NULL DEFAULT 0 COMMENT '0=未投递, 1=已投递',
    PRIMARY KEY (id),
    KEY idx_receiver_undelivered (receiver_id, delivered),
    CONSTRAINT fk_offline_sender   FOREIGN KEY (sender_id)   REFERENCES user_info(id) ON DELETE CASCADE,
    CONSTRAINT fk_offline_receiver FOREIGN KEY (receiver_id) REFERENCES user_info(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='离线消息队列';
```

**设计说明**：

- 用户登录时：`SELECT ... WHERE receiver_id=? AND delivered=0`，投递后批量 `UPDATE delivered=1`
- `msg_type` 存储 `MessageType` 枚举值，服务端知道如何重放该消息（聊天、好友申请通知等）
- 当前 v2 只存储文本聊天的离线消息；好友申请通知也可通过此表实现

---

## 4. 索引策略

| 表 | 索引 | 用途 |
|----|------|------|
| `user_info` | `UNIQUE(name)` | 登录、注册查重 |
| `friend` | `UNIQUE(user_id, friend_id)` | 防重复好友记录 |
| `friend` | `KEY(friend_id)` | 查询"谁把我加为好友" |
| `group_member` | `UNIQUE(group_id, user_id)` | 防重复入群 |
| `group_member` | `KEY(user_id)` | 查询用户所在的所有群 |
| `chat_message` | `KEY(sender_id, receiver_id, created_at)` | 拉取两人聊天历史（时间排序） |
| `chat_message` | `KEY(group_id, created_at)` | 拉取群聊历史（时间排序） |
| `offline_message` | `KEY(receiver_id, delivered)` | 登录时批量取未投递消息 |

---

## 5. 常用查询参考

### 5.1 查询 A 的全部好友（在线 + 离线）

```sql
SELECT u.id, u.name, u.nickname, u.online
FROM user_info u
WHERE u.id IN (
    SELECT friend_id FROM friend WHERE user_id = :uid AND status = 1
    UNION
    SELECT user_id   FROM friend WHERE friend_id = :uid AND status = 1
);
```

### 5.2 查询 A 与 B 的聊天记录（分页）

```sql
SELECT sender_id, content, created_at
FROM chat_message
WHERE (sender_id = :a AND receiver_id = :b)
   OR (sender_id = :b AND receiver_id = :a)
ORDER BY created_at ASC
LIMIT :page_size OFFSET :offset;
```

### 5.3 登录时投递离线消息

```sql
-- 取出未投递消息
SELECT id, sender_id, msg_type, content, created_at
FROM offline_message
WHERE receiver_id = :uid AND delivered = 0
ORDER BY created_at ASC;

-- 标记为已投递
UPDATE offline_message
SET delivered = 1
WHERE receiver_id = :uid AND delivered = 0;
```

### 5.4 查询群内所有在线成员（用于群消息广播）

```sql
SELECT u.id, u.name
FROM user_info u
JOIN group_member gm ON u.id = gm.user_id
WHERE gm.group_id = :gid AND u.online = 1;
```

---

## 6. 初始化脚本

完整的建库脚本位于 `sql/schema.sql`（第八章实现数据库层时创建），包含：

1. `CREATE DATABASE IF NOT EXISTS cloudvault`
2. 全部 6 张表的 `CREATE TABLE` 语句
3. 创建应用专用账号并授权（避免使用 root）

```sql
-- 创建应用账号示例（在 schema.sql 末尾）
CREATE USER IF NOT EXISTS 'cloudvault_app'@'localhost'
    IDENTIFIED BY 'your_password_here';
GRANT SELECT, INSERT, UPDATE, DELETE
    ON cloudvault.* TO 'cloudvault_app'@'localhost';
FLUSH PRIVILEGES;
```

> 生产环境数据库密码通过环境变量或 Docker Secret 注入，**不写入配置文件**。

---

## 7. 与 v1 的差异（迁移说明）

| 变更项 | v1 | v2 |
|--------|----|----|
| 数据库名 | `mydbqt` | `cloudvault` |
| 密码字段 | `pwd VARCHAR(64)`（明文） | `password_hash VARCHAR(97)`（SHA-256+salt） |
| 好友表主键 | 复合主键 `(user_id, friend_id)` | 独立自增 `id`，加唯一约束 |
| 好友状态 | 无（仅已添加） | `status` 字段（0=待接受, 1=已添加） |
| 时间戳 | 无 | 全表增加 `created_at`，user_info 增加 `updated_at` |
| 新增表 | — | `chat_group`, `group_member`, `chat_message`, `offline_message` |
| 字符集 | 未指定 | `utf8mb4`（支持 emoji） |

### 数据迁移步骤（v1 → v2）

1. 导出 v1 用户数据：`mysqldump mydbqt user_info friend`
2. 执行 v2 `schema.sql` 建立新库
3. 迁移 `user_info`：将 `pwd` 字段标记为"需重置密码"（v2 无法还原明文密码对应的哈希）
4. 迁移 `friend`：补充 `status=1`，`created_at=NOW()`
5. 通知用户首次登录需重新设置密码

> **注**：由于 v1 密码明文存储，迁移时无法自动生成 v2 格式的哈希，需要用户在首次登录时强制修改密码。
