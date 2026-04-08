# 第十章　即时聊天

> **状态**：✅ 已完成
>
> **本章目标**：实现好友间实时消息、离线消息补投，以及消息页真实联动。
>
> **验收标准**：
> - 两端在线时，消息实时送达
> - 一端离线时，消息写入离线表；对方上线后自动补投
> - 选择好友后可以拉取历史消息
> - 主界面消息页不再展示静态样例，而是真实聊天内容

---

## 10.1 本章完成了什么

第九章解决了“好友关系建立”，第十章开始解决“好友之间真的能聊天”。

本章落地了：

- `chat_message` 持久化聊天记录
- `offline_message` 存储未投递私聊消息
- 服务端 `ChatRepository`
- 服务端 `ChatHandler`
- 登录成功后的离线消息补投
- 客户端 `ChatService`
- `MainWindow` 消息页真实接入会话与历史记录

当前实现范围：

- 支持一对一私聊
- 支持离线消息补投
- 支持历史消息加载
- 支持联系人列表预览、时间和未读数更新
- 群聊链路已在当前代码库中补入，但独立群聊历史协议仍未完成

---

## 10.2 数据流总览

```text
A 选择好友 B
  ↓
MainWindow -> ChatService::loadHistory(B)
  ↓
服务端查询 chat_message
  ↓
返回 GET_HISTORY
  ↓
MainWindow 渲染历史消息

A 发送消息给 B
  ↓
ChatService 发送 CHAT
  ↓
ChatHandler 校验：
  - A 已登录
  - B 存在
  - A/B 是好友
  ↓
写入 chat_message
  ↓
若 B 在线：
  直接向 B 推送 CHAT
若 B 离线：
  写入 offline_message

B 登录成功
  ↓
AuthHandler 查询 offline_message
  ↓
逐条向 B 推送 CHAT
  ↓
标记 delivered=1
```

---

## 10.3 数据库设计

当前初始化脚本位于 `server/sql/init.sql`。

### 10.3.1 聊天记录表

```sql
CREATE TABLE IF NOT EXISTS chat_message (
    msg_id      BIGINT       NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_id   INT          NOT NULL,
    receiver_id INT          NOT NULL,
    content     TEXT         NOT NULL,
    created_at  DATETIME(3)  NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
);
```

用途：

- 持久化私聊消息
- 支持历史消息查询

### 10.3.2 离线消息表

```sql
CREATE TABLE IF NOT EXISTS offline_message (
    id          BIGINT       NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_id   INT          NOT NULL,
    receiver_id INT          NOT NULL,
    msg_type    INT          NOT NULL,
    content     TEXT         NOT NULL,
    created_at  DATETIME(3)  NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    delivered   TINYINT      NOT NULL DEFAULT 0
);
```

用途：

- 对方离线时缓存私聊消息
- 用户登录后批量补投

---

## 10.4 协议设计

第十章沿用当前 `common` 层已有的两个消息类型：

```cpp
CHAT
GET_HISTORY
```

定义位于 `common/include/common/protocol.h`。

### 10.4.1 CHAT

**客户端 → 服务端**

```text
target_username : string
content         : string
```

**服务端 → 客户端**

```text
from_username : string
to_username   : string
content       : string
created_at    : string
```

说明：

- 发送方身份不信任客户端 body，而是由服务端通过当前连接反查
- 服务端不回显给发送方，发送方 UI 先本地追加
- 接收方在线时实时推送；离线时写入 `offline_message`

### 10.4.2 GET_HISTORY

**客户端 → 服务端**

```text
peer_username : string
```

**服务端 → 客户端**

成功时：

```text
status   : uint8 (0)
peer     : string
count    : uint16
items[]  : sender + receiver + content + created_at
```

失败时：

```text
status  : uint8 (!=0)
peer    : string
message : string
```

---

## 10.5 服务端实现

### 10.5.1 ChatRepository

`server/include/server/db/chat_repository.h` /
`server/src/db/chat_repository.cpp`

负责：

- 写入 `chat_message`
- 写入 `offline_message`
- 查询未投递离线消息
- 标记离线消息已投递
- 查询两人的历史聊天记录

### 10.5.2 ChatHandler

`server/include/server/handler/chat_handler.h` /
`server/src/handler/chat_handler.cpp`

当前实现了两个入口：

- `handleChat()`
- `handleGetHistory()`

#### 私聊发送

处理流程：

1. 从 `SessionManager` 反查当前登录用户
2. 解析目标用户名和消息内容
3. 校验目标存在且双方是好友
4. 写入 `chat_message`
5. 如果目标在线：
   - 直接推送 `CHAT`
6. 如果目标离线：
   - 写入 `offline_message`

#### 历史查询

处理流程：

1. 校验当前连接已登录
2. 校验目标用户存在
3. 校验双方是好友
4. 查询 `chat_message`
5. 返回 `GET_HISTORY`

### 10.5.3 登录后的离线消息补投

补投逻辑放在 `server/src/handler/auth_handler.cpp`：

- 用户登录成功
- 先返回 `LOGIN_RESPONSE`
- 再查询该用户的 `offline_message`
- 逐条推送 `CHAT`
- 最后标记 `delivered=1`

这样可以保证客户端先完成登录态切换，再接收离线消息。

### 10.5.4 ServerApp 接入

`server/src/server_app.cpp` 已注册：

- `CHAT`
- `GET_HISTORY`

---

## 10.6 客户端实现

### 10.6.1 ChatService

`client/src/service/chat_service.h` /
`client/src/service/chat_service.cpp`

负责：

- 发送 `CHAT`
- 请求 `GET_HISTORY`
- 解析服务端推送的私聊消息
- 解析历史消息响应

核心信号：

- `messageReceived`
- `historyLoaded`
- `historyLoadFailed`

### 10.6.2 MainWindow 消息页

`client/src/ui/main_window.cpp`

消息页不再是静态样例，而是：

- 选择好友后自动拉历史
- 实时渲染消息气泡
- 发送消息后本地先追加
- 收到对方消息时自动更新当前会话或未读数
- 联系人列表预览显示最近一条消息

当前消息页行为：

- 左栏联系人列表：显示最近消息预览 / 时间 / 未读数
- 中栏：显示真实会话滚动区
- 输入框：发送文本消息
- 右栏：仍保留联系人详情和共享文件摘要

---

## 10.7 联调步骤

### 10.7.1 在线实时聊天

1. 启动服务端
2. 启动两个客户端 A / B
3. A、B 登录并互为好友
4. A 选中 B，发送消息
5. B 实时收到消息

### 10.7.2 离线消息补投

1. B 下线
2. A 向 B 发送消息
3. 服务端将消息写入 `offline_message`
4. B 重新登录
5. 登录成功后自动收到离线消息

### 10.7.3 历史消息

1. A 与 B 多轮聊天
2. 切换联系人或重新登录
3. 再次选中对方
4. 消息页自动加载历史记录

---

## 10.8 本章知识点

- 实时消息推送
- 离线消息补投
- 聊天记录持久化
- Qt 聊天消息列表渲染
- 联系人列表中的预览/未读数联动

---

## 10.9 小结

到本章结束，CloudVault 已经具备最基础的一对一聊天能力：

- 在线实时私聊
- 离线消息补投
- 历史消息加载
- 主界面消息页真实联动

下一章将继续实现文件管理能力。

---

上一章：[第九章 — 好友功能](ch09-friend.md) ｜ 下一章：[第十一章 — 文件管理](ch11-file-manager.md)
