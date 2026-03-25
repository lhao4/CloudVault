# 第十章　即时聊天

> **状态**：🔲 待写
>
> **本章目标**：实现好友间实时消息与离线消息补投。
>
> **验收标准**：
> - 两端在线时，消息实时送达
> - 一端离线时，消息存库；对方上线后自动补投

---

## 本章大纲

### 10.1 数据库层

**`chat_message` 表**（持久化聊天记录）：

```sql
CREATE TABLE chat_message (
    id         BIGINT       NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender     VARCHAR(32)  NOT NULL,
    receiver   VARCHAR(32)  NOT NULL,
    content    TEXT         NOT NULL,
    created_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

**`offline_message` 表**（待投递的离线消息）：

```sql
CREATE TABLE offline_message (
    id          BIGINT      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    target_user VARCHAR(32) NOT NULL,
    sender      VARCHAR(32) NOT NULL,
    content     TEXT        NOT NULL,
    created_at  DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

### 10.2 服务端 ChatHandler（待完成）

**在线转发流程**：

```
收到 CHAT_REQUEST（sender, target, content）
  ↓
SessionManager::findByName(target)
  ├── 找到（在线）→ 直接发送 CHAT_RESPOND 给 target
  └── 未找到（离线）→ 写入 offline_message 表
```

**离线补投流程**：

```
用户登录成功后
  ↓
查询 offline_message WHERE target_user = 用户名
  ↓
逐条发送 CHAT_RESPOND
  ↓
删除已投递的记录
```

### 10.3 客户端 ChatService 与 UI（待完成）

- `sendMessage()`：构建 `CHAT_REQUEST` PDU，调用 `sendPDU()`
- `onMessageReceived()`：解析响应，emit `messageReceived` 信号
- `ChatWindow`：聊天气泡显示（自己右对齐，对方左对齐），回车发送

### 10.4 本章新知识点

- 离线消息的存储与补投策略
- Qt 自定义 Widget（聊天气泡）

---

上一章：[第九章 — 好友功能](ch09-friend.md) ｜ 下一章：[第十一章 — 文件管理](ch11-file-manager.md)
