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
    is_online     TINYINT      NOT NULL DEFAULT 0                  COMMENT '是否在线',
    created_at    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP  COMMENT '注册时间'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户信息表';

-- ── 好友关系表（第九章）──────────────────────────────────────
CREATE TABLE IF NOT EXISTS friend_relation (
    id          INT      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    user_id     INT      NOT NULL COMMENT '用户ID',
    friend_id   INT      NOT NULL COMMENT '好友ID',
    created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_friendship (user_id, friend_id),
    KEY idx_user_id   (user_id),
    KEY idx_friend_id (friend_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='好友关系表';

-- ── 聊天记录表（第十章）──────────────────────────────────────
CREATE TABLE IF NOT EXISTS chat_message (
    msg_id      BIGINT       NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_id   INT          NOT NULL COMMENT '发送者ID',
    receiver_id INT          NOT NULL COMMENT '接收者ID（私聊）',
    content     TEXT         NOT NULL COMMENT '消息内容',
    created_at  DATETIME(3)  NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    KEY idx_sender   (sender_id),
    KEY idx_receiver (receiver_id),
    KEY idx_time     (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='聊天记录表';

-- ── 离线消息表（第十章）──────────────────────────────────────
CREATE TABLE IF NOT EXISTS offline_message (
    id          BIGINT       NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_id   INT          NOT NULL COMMENT '发送者ID',
    receiver_id INT          NOT NULL COMMENT '接收者ID',
    msg_type    INT          NOT NULL COMMENT '消息类型',
    content     TEXT         NOT NULL COMMENT '消息内容',
    created_at  DATETIME(3)  NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    delivered   TINYINT      NOT NULL DEFAULT 0 COMMENT '是否已投递',
    KEY idx_receiver_undelivered (receiver_id, delivered),
    KEY idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='离线消息表';

-- ── 文件元数据表（第十一章）──────────────────────────────────
CREATE TABLE IF NOT EXISTS file_info (
    file_id     INT          NOT NULL AUTO_INCREMENT PRIMARY KEY,
    owner_id    INT          NOT NULL COMMENT '文件所有者ID',
    file_name   VARCHAR(255) NOT NULL COMMENT '文件名',
    file_size   BIGINT       NOT NULL DEFAULT 0 COMMENT '文件大小（字节）',
    file_path   VARCHAR(512) NOT NULL COMMENT '服务器存储路径',
    parent_id   INT          NOT NULL DEFAULT 0 COMMENT '父目录ID（0=根目录）',
    is_dir      TINYINT      NOT NULL DEFAULT 0 COMMENT '是否为目录',
    created_at  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    KEY idx_owner  (owner_id),
    KEY idx_parent (parent_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='文件元数据表';

-- ── 验证 ──────────────────────────────────────────────────────
SELECT 'CloudVault database initialized successfully.' AS status;
SHOW TABLES;
