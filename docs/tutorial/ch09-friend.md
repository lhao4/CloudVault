# 第九章　好友功能

> **状态**：🔲 待写
>
> **本章目标**：支持搜索用户、发送好友请求、处理请求、拉取好友列表。
>
> **验收标准**：两个客户端实例互加好友，双方好友列表一致。

---

## 本章大纲

### 9.1 数据库层

**`friend` 表结构**：

```sql
CREATE TABLE friend (
    id         INT      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    user_id    INT      NOT NULL,
    friend_id  INT      NOT NULL,
    status     TINYINT  NOT NULL DEFAULT 0,  -- 0=待确认, 1=已添加
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_pair (user_id, friend_id)
);
```

`FriendRepository`：`areFriends()`、`insertRequest()`、`acceptRequest()`、`removeFriend()`、`getAllFriends()`

### 9.2 服务端 FriendHandler（待完成）

| 消息类型 | 处理逻辑 |
|---------|---------|
| `FIND_USER_REQUEST` | 查 `user_info`，返回是否存在及在线状态 |
| `ONLINE_USERS_REQUEST` | 从 `SessionManager` 获取在线列表 |
| `ADD_FRIEND_REQUEST` | 检查是否已是好友，转发请求给目标用户 |
| `AGREE_ADD_FRIEND_REQUEST` | 写 DB，通知双方 |
| `FLUSH_FRIEND_REQUEST` | 返回全部好友列表（含在线状态） |
| `DELETE_FRIEND_REQUEST` | 删除双向好友关系 |

### 9.3 客户端 FriendService 与 UI（待完成）

- 好友列表刷新：登录成功后自动拉取
- 添加好友：搜索用户对话框 → 发送请求 → 等待对方确认
- 请求通知：服务端主动推送，弹出确认对话框
- 好友列表 UI：显示用户名 + 在线状态绿点

### 9.4 在线推送机制

好友请求不是客户端轮询，而是服务端**主动推送**：

```
A 发送 ADD_FRIEND_REQUEST（目标：B）
      ↓
服务端 FriendHandler
  ├── B 在线 → SessionManager::sendTo(B, ADD_FRIEND_REQUEST)
  └── B 离线 → 暂存（v2 暂不处理离线好友请求）
      ↓
B 的客户端收到推送 → ResponseRouter 路由 → FriendService 处理 → emit incomingFriendRequest
      ↓
FriendWidget 弹出确认对话框
```

---

上一章：[第八章 — 注册与登录](ch08-auth.md) ｜ 下一章：[第十章 — 即时聊天](ch10-chat.md)
