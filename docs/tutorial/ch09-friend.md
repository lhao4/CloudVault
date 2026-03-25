# 第九章　好友功能

> **状态**：✅ 已完成
>
> **本章目标**：支持搜索用户、发送好友申请、在线推送申请、同意好友、刷新好友列表、删除好友。
>
> **验收标准**：
> - 用户登录成功后进入主窗口
> - 可以搜索用户并看到在线状态
> - 两个在线客户端可以完成加好友
> - 双方好友列表刷新后保持一致
> - 删除好友后双方列表不再显示彼此

---

## 9.1 本章完成了什么

第八章解决了“用户能登录”，第九章开始解决“用户之间能建立关系”。

本章落地了：

- 显式的好友协议请求/响应/推送
- 服务端 `FriendRepository`
- 服务端 `FriendHandler`
- `SessionManager` 连接反查身份
- 客户端 `FriendService`
- 登录后的最小 `MainWindow`
- 主窗口中的好友页 `FriendPage`

当前实现范围：

- 支持在线好友申请
- 支持同意好友申请
- 支持刷新好友列表
- 支持删除好友
- 不支持离线好友申请持久化
- 不支持拒绝好友申请专门回包

---

## 9.2 数据流总览

```text
A 登录成功
  ↓
LoginWindow -> MainWindow -> FriendPage
  ↓
FriendService 发送 ADD_FRIEND_REQUEST(B)
  ↓
FriendHandler 检查：
  - A 已登录
  - B 存在
  - A/B 不是好友
  - B 当前在线
  ↓
服务端向 B 推送 FRIEND_REQUEST_PUSH(A)
  ↓
B 的 FriendPage 弹窗确认
  ↓
B 发送 AGREE_FRIEND_REQUEST(A)
  ↓
FriendHandler 双向写入 friend_relation
  ↓
返回 AGREE_FRIEND_RESPONSE 给 B
并向 A 推送 FRIEND_ADDED_PUSH(B)
  ↓
A/B 刷新好友列表，看到对方和在线状态
```

---

## 9.3 协议设计

本章不再复用一个消息类型同时表示请求与响应，而是改为显式区分：

```cpp
FIND_USER_REQUEST
FIND_USER_RESPONSE
ADD_FRIEND_REQUEST
ADD_FRIEND_RESPONSE
FRIEND_REQUEST_PUSH
AGREE_FRIEND_REQUEST
AGREE_FRIEND_RESPONSE
FRIEND_ADDED_PUSH
FLUSH_FRIENDS_REQUEST
FLUSH_FRIENDS_RESPONSE
DELETE_FRIEND_REQUEST
DELETE_FRIEND_RESPONSE
FRIEND_DELETED_PUSH
```

定义位于 [protocol.h](/mnt/d/CloudVault/common/include/common/protocol.h)。

### 9.3.1 查找用户

**`FIND_USER_REQUEST`**

```text
target_username : string
```

**`FIND_USER_RESPONSE`**

成功时：

```text
status   : uint8 (0)
username : string
online   : uint8
```

失败时：

```text
status  : uint8 (!=0)
message : string
```

### 9.3.2 发送好友申请

**`ADD_FRIEND_REQUEST`**

```text
target_username : string
```

**`ADD_FRIEND_RESPONSE`**

```text
status  : uint8
message : string
```

常见状态：

- `0`：请求已发送
- `2`：用户不存在
- `3`：对方不在线
- `4`：已经是好友

**`FRIEND_REQUEST_PUSH`**

服务端推送给目标方：

```text
from_username : string
```

### 9.3.3 同意好友申请

**`AGREE_FRIEND_REQUEST`**

```text
requester_username : string
```

**`AGREE_FRIEND_RESPONSE`**

```text
status  : uint8
message : string
```

**`FRIEND_ADDED_PUSH`**

服务端推送给原发起方：

```text
friend_username : string
```

### 9.3.4 刷新好友列表

**`FLUSH_FRIENDS_REQUEST`**

```text
Body: 空
```

**`FLUSH_FRIENDS_RESPONSE`**

成功时：

```text
status       : uint8
friend_count : uint16
friends[]    : string + uint8(online)
```

失败时：

```text
status  : uint8
message : string
```

### 9.3.5 删除好友

**`DELETE_FRIEND_REQUEST`**

```text
target_username : string
```

**`DELETE_FRIEND_RESPONSE`**

```text
status  : uint8
message : string
```

**`FRIEND_DELETED_PUSH`**

服务端推送给被删除方：

```text
friend_username : string
```

---

## 9.4 数据库设计

本章沿用第八章初始化脚本中的 `friend_relation` 表，不再使用旧设计稿里带 `status` 字段的 `friend` 表。

当前表结构见 [init.sql](/mnt/d/CloudVault/server/sql/init.sql)：

```sql
CREATE TABLE IF NOT EXISTS friend_relation (
    id          INT      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    user_id     INT      NOT NULL,
    friend_id   INT      NOT NULL,
    created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_friendship (user_id, friend_id),
    KEY idx_user_id   (user_id),
    KEY idx_friend_id (friend_id)
);
```

本章采用“双向两条记录”的模型：

- A 与 B 成为好友时，写入：
  - `(A, B)`
  - `(B, A)`
- 删除好友时，双向删除
- 查询列表时，只查 `user_id = 当前用户`

这个设计的优点是：

- 查询好友列表简单
- 删除好友简单
- 不需要在应用层做复杂的 `UNION`

缺点是：

- 数据会多两条记录

对本项目当前阶段来说，这个权衡是合理的。

---

## 9.5 服务端实现

### 9.5.1 SessionManager 增强

为了让服务端不信任客户端上传的“当前用户名”，本章给 `SessionManager` 新增了：

```cpp
findByConnection(std::shared_ptr<TcpConnection>)
```

这样 `FriendHandler` 可以根据当前连接直接反查登录身份。

这意味着：

- 客户端请求体里只需要传“目标用户”
- 当前操作者是谁，由服务端会话决定

### 9.5.2 FriendRepository

[friend_repository.cpp](/mnt/d/CloudVault/server/src/db/friend_repository.cpp) 实现了：

- `areFriends(user_id, friend_id)`
- `addFriendPair(user_id, friend_id)`
- `removeFriendPair(user_id, friend_id)`
- `listFriends(user_id)`

实现要点：

- 全部使用 MySQL C API
- 写入/删除双向关系时使用事务
- 列表查询通过 `JOIN user_info` 直接返回好友用户名

### 9.5.3 FriendHandler

[friend_handler.cpp](/mnt/d/CloudVault/server/src/handler/friend_handler.cpp) 负责全部好友业务：

- `handleFindUser()`
- `handleAddFriend()`
- `handleAgreeFriend()`
- `handleFlushFriends()`
- `handleDeleteFriend()`

#### 查找用户

- 按用户名查 `user_info`
- 使用 `SessionManager::isOnline()` 判断是否在线

#### 发送好友申请

校验顺序：

1. 当前连接必须已登录
2. 不能添加自己
3. 目标用户必须存在
4. 双方不能已经是好友
5. 目标用户必须在线

如果通过校验：

- 给发起方返回 `ADD_FRIEND_RESPONSE`
- 给目标方推送 `FRIEND_REQUEST_PUSH`

#### 同意好友申请

校验顺序：

1. 当前连接必须已登录
2. 请求方必须存在
3. 不能把自己加成好友
4. 不能重复成为好友

通过校验后：

- `FriendRepository::addFriendPair()` 双向写库
- 给同意方返回 `AGREE_FRIEND_RESPONSE`
- 给发起方推送 `FRIEND_ADDED_PUSH`

#### 刷新好友列表

- 根据当前连接反查登录用户
- 查 `friend_relation`
- 对每个好友再用 `SessionManager` 判定在线状态
- 返回好友列表数组

#### 删除好友

- 检查目标存在
- 检查双方确实是好友
- 双向删除关系
- 给请求方返回 `DELETE_FRIEND_RESPONSE`
- 给目标方推送 `FRIEND_DELETED_PUSH`

### 9.5.4 ServerApp 接入

[server_app.cpp](/mnt/d/CloudVault/server/src/server_app.cpp) 已注册以下好友消息：

- `FIND_USER_REQUEST`
- `ADD_FRIEND_REQUEST`
- `AGREE_FRIEND_REQUEST`
- `FLUSH_FRIENDS_REQUEST`
- `DELETE_FRIEND_REQUEST`

---

## 9.6 客户端实现

### 9.6.1 FriendService

[friend_service.cpp](/mnt/d/CloudVault/client/src/network/friend_service.cpp) 负责：

- 构建好友请求 PDU
- 解析响应和推送
- 对 UI 发出 Qt 信号

核心信号包括：

- `userFound`
- `userNotFound`
- `friendRequestSent`
- `incomingFriendRequest`
- `friendAdded`
- `friendsRefreshed`
- `friendDeleted`

### 9.6.2 LoginWindow → MainWindow

第八章中登录成功后，客户端还停留在 `LoginWindow`。  
本章登录成功后会：

1. 创建最小 `MainWindow`
2. 隐藏登录窗口
3. 进入好友页
4. 自动调用一次 `flushFriends()`

### 9.6.3 MainWindow

[main_window.cpp](/mnt/d/CloudVault/client/src/ui/main_window.cpp) 是一个最小主窗口壳。

当前它只做一件事：

- 把 `FriendPage` 作为中央区域显示出来

为第十章聊天功能保留扩展空间。

### 9.6.4 FriendPage

[friend_page.cpp](/mnt/d/CloudVault/client/src/ui/friend_page.cpp) 提供最小可用好友交互：

- 搜索用户
- 发送好友申请
- 收到在线申请后弹确认框
- 刷新好友列表
- 删除好友

当前好友列表项会显示：

- 用户名
- 在线/离线状态

---

## 9.7 在线推送机制

本章的好友申请不是轮询拉取，而是服务端主动推送：

```text
A 发送 ADD_FRIEND_REQUEST(B)
      ↓
服务端校验 B 在线
      ↓
向 B 推送 FRIEND_REQUEST_PUSH(A)
      ↓
B 弹出确认框
      ↓
B 同意后发送 AGREE_FRIEND_REQUEST(A)
      ↓
服务端双向写库
      ↓
向 A 推送 FRIEND_ADDED_PUSH(B)
```

### 当前边界

本章只支持“在线申请”：

- B 在线：可以收到推送并处理
- B 离线：A 会收到失败提示

离线申请持久化与补投递，留到后续需要时再扩展。

---

## 9.8 联调步骤

### 9.8.1 启动两个客户端和一个服务端

确保：

- 服务端已连接 MySQL
- 两个客户端都能登录
- 两个客户端都进入 `MainWindow`

### 9.8.2 搜索用户

在客户端 A 搜索 B：

- 如果 B 存在，应显示用户名和在线状态
- 如果不存在，应显示“用户不存在”

### 9.8.3 发送好友申请

A 给 B 发送好友申请：

- A 看到“好友请求已发送”
- B 弹出确认框

### 9.8.4 同意好友申请

B 点击同意：

- B 看到“已同意好友请求”
- A 收到被同意通知
- A/B 好友列表刷新后都出现对方

### 9.8.5 删除好友

A 删除 B：

- A 看到删除成功
- B 收到被删除推送
- 双方刷新后列表里都不再出现对方

### 9.8.6 离线场景

关闭 B 后，A 再尝试添加：

- 应收到“对方当前不在线”

---

## 9.9 本章知识点

- 显式请求/响应/推送协议设计
- 会话身份由服务端连接反查，而不是客户端自报
- 双向好友关系建模
- Qt 主窗口切换与页面承载
- 服务端主动推送到在线用户

---

## 9.10 小结

到本章结束，CloudVault 已经具备最基本的社交关系能力：

- 查找用户
- 发好友申请
- 在线同意好友
- 刷新好友列表
- 删除好友

下一章将在好友体系基础上继续做一对一即时聊天。

---

上一章：[第八章 — 注册与登录](ch08-auth.md) ｜ 下一章：[第十章 — 即时聊天](ch10-chat.md)
