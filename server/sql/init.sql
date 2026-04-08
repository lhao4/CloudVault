-- =============================================================
-- server/sql/init.sql
-- CloudVault 数据库初始化脚本
--
-- 使用方式：
--   mysql -u root -p < server/sql/init.sql
--   或：mysql -u root -P 3307 -h 127.0.0.1 -p < server/sql/init.sql
--
-- 幂等设计：可重复执行，不会破坏已有数据
-- =============================================================

-- ── 创建数据库 ────────────────────────────────────────────────
CREATE DATABASE IF NOT EXISTS cloudvault
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

-- ── 创建应用账号 ──────────────────────────────────────────────
-- 生产环境请修改密码，并限制只允许从应用服务器 IP 访问
CREATE USER IF NOT EXISTS 'cloudvault_app'@'%'
    IDENTIFIED BY 'cloudvault_dev_123';

GRANT ALL PRIVILEGES ON cloudvault.* TO 'cloudvault_app'@'%';
FLUSH PRIVILEGES;

-- ── 切换到 cloudvault 数据库 ──────────────────────────────────
USE cloudvault;

-- ── 用户表 ────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS user_info (
    user_id       INT          NOT NULL AUTO_INCREMENT PRIMARY KEY COMMENT '用户ID',
    username      VARCHAR(32)  NOT NULL UNIQUE                     COMMENT '登录用户名',
    password_hash VARCHAR(128) NOT NULL                            COMMENT 'salt:sha256hex',
    nickname      VARCHAR(64)  DEFAULT NULL                        COMMENT '昵称，可为空（显示时回退到 username）',
    signature     VARCHAR(128) DEFAULT NULL                        COMMENT '个性签名',
    avatar_path   VARCHAR(256) DEFAULT NULL                        COMMENT '头像文件路径（相对于用户根目录）',
    is_online     TINYINT      NOT NULL DEFAULT 0                  COMMENT '0=离线, 1=在线',
    created_at    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP  COMMENT '注册时间',
    updated_at    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
                               ON UPDATE CURRENT_TIMESTAMP         COMMENT '最后更新时间'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户信息表';

-- ── 好友关系表 ────────────────────────────────────────────────
-- 单向记录：A 添加 B → 插入 (A, B, 0)，B 同意后 UPDATE status=1
-- 查询"A 的好友列表"需用 user_id=A OR friend_id=A 的 UNION 查询
CREATE TABLE IF NOT EXISTS friend (
    id          INT      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    user_id     INT      NOT NULL COMMENT '发起方用户 ID',
    friend_id   INT      NOT NULL COMMENT '接受方用户 ID',
    status      TINYINT  NOT NULL DEFAULT 0 COMMENT '0=待接受, 1=已添加',
    created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_friendship (user_id, friend_id),
    KEY idx_friend_id (friend_id),
    CONSTRAINT fk_friend_user   FOREIGN KEY (user_id)   REFERENCES user_info(user_id) ON DELETE CASCADE,
    CONSTRAINT fk_friend_friend FOREIGN KEY (friend_id) REFERENCES user_info(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='好友关系表';

-- ── 群组表 ────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS chat_group (
    id          INT          NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name        VARCHAR(64)  NOT NULL COMMENT '群名称',
    owner_id    INT          NOT NULL COMMENT '群主用户 ID',
    created_at  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    KEY idx_owner (owner_id),
    CONSTRAINT fk_group_owner FOREIGN KEY (owner_id) REFERENCES user_info(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='群组表';

-- ── 群成员表 ──────────────────────────────────────────────────
-- 群主本身也需插入 group_member 记录（创建群时同步插入）
CREATE TABLE IF NOT EXISTS group_member (
    id          INT       NOT NULL AUTO_INCREMENT PRIMARY KEY,
    group_id    INT       NOT NULL COMMENT '群组 ID',
    user_id     INT       NOT NULL COMMENT '成员用户 ID',
    joined_at   DATETIME  NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_membership (group_id, user_id),
    KEY idx_user_groups (user_id),
    CONSTRAINT fk_member_group FOREIGN KEY (group_id) REFERENCES chat_group(id) ON DELETE CASCADE,
    CONSTRAINT fk_member_user  FOREIGN KEY (user_id)  REFERENCES user_info(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='群成员关系表';

-- ── 聊天记录表 ────────────────────────────────────────────────
-- receiver_id 和 group_id 互斥：私聊时 receiver_id 非空，群聊时 group_id 非空
-- MySQL 8.0 在某些版本下不允许 CHECK 约束引用带 ON DELETE SET NULL
-- 的外键列，这里改由应用层保证互斥关系。
CREATE TABLE IF NOT EXISTS chat_message (
    msg_id      BIGINT       NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_id   INT          NOT NULL COMMENT '发送者 ID',
    receiver_id INT          DEFAULT NULL COMMENT '接收者 ID（私聊）',
    group_id    INT          DEFAULT NULL COMMENT '群组 ID（群聊）',
    content     TEXT         NOT NULL COMMENT '消息内容',
    created_at  DATETIME(3)  NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    KEY idx_private_chat (sender_id, receiver_id, created_at),
    KEY idx_group_chat   (group_id, created_at),
    CONSTRAINT fk_msg_sender   FOREIGN KEY (sender_id)   REFERENCES user_info(user_id) ON DELETE CASCADE,
    CONSTRAINT fk_msg_receiver FOREIGN KEY (receiver_id) REFERENCES user_info(user_id) ON DELETE SET NULL,
    CONSTRAINT fk_msg_group    FOREIGN KEY (group_id)    REFERENCES chat_group(id)     ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='聊天记录表（私聊 + 群聊）';

-- ── 离线消息表 ────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS offline_message (
    id          BIGINT       NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_id   INT          NOT NULL COMMENT '发送者 ID',
    receiver_id INT          NOT NULL COMMENT '接收者 ID',
    msg_type    INT          NOT NULL COMMENT '消息类型（MessageType 枚举值）',
    content     TEXT         NOT NULL COMMENT '消息内容',
    created_at  DATETIME(3)  NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    delivered   TINYINT      NOT NULL DEFAULT 0 COMMENT '0=未投递, 1=已投递',
    KEY idx_receiver_undelivered (receiver_id, delivered),
    KEY idx_created_at (created_at),
    CONSTRAINT fk_offline_sender   FOREIGN KEY (sender_id)   REFERENCES user_info(user_id) ON DELETE CASCADE,
    CONSTRAINT fk_offline_receiver FOREIGN KEY (receiver_id) REFERENCES user_info(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='离线消息表';

-- ── 验证 ────────────────────────────────────────���─────────────
SELECT 'CloudVault database initialized successfully.' AS status;
SHOW TABLES;
