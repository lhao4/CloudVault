# CloudVault 测试规范

> 版本：v2.0 | 状态：与当前仓库结构对齐

---

## 1. 测试分层

| 层级 | 目标 |
|------|------|
| 单元测试 | `common` 编解码、密码学、`server` 基础模块 |
| 集成测试 | 数据库、协议、登录/聊天/文件等端到端流程 |
| 手动测试 | 客户端 UI、资料同步、好友与群聊冒烟 |

---

## 2. 当前仓库测试入口

现有测试目录：

```text
common/tests/
server/tests/
```

构建：

```bash
cmake -S server -B server/build -DBUILD_TESTING=ON
cmake --build server/build -j4
ctest --test-dir server/build --output-on-failure
```

说明：

- 服务端测试与主目标共用 `server/build`
- 客户端测试和 UI 验证依赖本机 Qt6 开发环境

---

## 3. 数据库集成测试

### 3.1 推荐数据库环境

推荐直接使用项目专用容器：

```bash
docker compose -f docker-compose.mysql.yml up -d
```

默认连接参数：

| 项目 | 值 |
|------|----|
| Host | `127.0.0.1` |
| Port | `3308` |
| Database | `cloudvault` |
| User | `cloudvault_app` |
| Password | `cloudvault_dev_123` |

### 3.2 基本连通性检查

```bash
mysql -h127.0.0.1 -P3308 -ucloudvault_app -pcloudvault_dev_123 cloudvault -e "SHOW TABLES;"
```

期望看到：

- `user_info`
- `friend`
- `chat_group`
- `group_member`
- `chat_message`
- `offline_message`

### 3.3 推荐覆盖点

`UserRepository`

- 注册用户成功
- 重复用户名失败
- `findByName()` 能返回 `nickname/signature/avatar_path`
- `updateProfile()` 能正确更新资料
- `setOnline()` 能正确切换在线状态

`FriendRepository`

- `insertRequest()` 插入待处理申请
- `acceptRequest()` 将 `status` 从 `0` 更新为 `1`
- `hasPendingRequest()` 判定正确
- `listFriends()` 只返回 `status=1` 的好友
- `removeFriend()` 能双向删除

`ChatRepository`

- `storePrivateMessage()` 私聊持久化成功
- `storeGroupMessage()` 群聊持久化成功
- `fetchPrivateHistory()` 顺序正确
- `fetchGroupHistory()` 顺序正确
- `queueOfflineMessage()` / `markMessagesDelivered()` 行为正确

`GroupRepository`

- `createGroup()` 自动把群主插入成员表
- `addMember()` / `removeMember()` 正常
- `deleteGroup()` 能级联清理成员
- `listUserGroups()` / `listMembers()` / `listOnlineMembers()` 返回正确

---

## 4. 服务端协议集成测试

推荐验证以下流程：

### 4.1 认证

- 注册新用户
- 正确密码登录
- 错误密码登录失败
- 同账号重复登录失败
- 更新昵称和签名成功
- 登出后在线状态恢复为离线

### 4.2 好友

- 查找存在用户
- 发送好友申请
- 对方同意后双方好友列表可见
- 删除好友后双方列表消失
- 离线好友申请在登录后重放

### 4.3 私聊

- 在线私聊实时送达
- 离线私聊登录后重放
- 私聊历史查询返回完整顺序

### 4.4 群组

- 创建群组
- 创建者自动成为成员
- 获取群列表
- 加入群组
- 退出群组
- 群主退出时群组解散
- 在线群聊广播
- 离线群聊登录后重放

说明：

- 群聊历史仓储层已完成
- 对外独立“群聊历史协议”尚未定义完成，当前不作为客户端联调必测项

---

## 5. 客户端手动测试清单

### 5.1 登录与资料页

| # | 操作 | 预期 |
|---|------|------|
| 1 | 正确账号登录 | 进入主窗口 |
| 2 | 错误密码登录 | 提示失败 |
| 3 | 修改昵称/签名并保存 | 本地草稿与服务端资料同步成功 |

### 5.2 好友与私聊

| # | 操作 | 预期 |
|---|------|------|
| 4 | 查找用户 | 显示是否在线 |
| 5 | 发送好友申请 | 对方收到请求或离线重放 |
| 6 | 同意好友申请 | 双方好友列表更新 |
| 7 | 发送私聊消息 | 在线实时、离线重放 |
| 8 | 打开私聊会话 | 历史消息加载成功 |

### 5.3 群组与群聊

| # | 操作 | 预期 |
|---|------|------|
| 9 | 打开群组列表 | 能拉到群组列表 |
| 10 | 创建群组 | 成功创建并进入群聊 |
| 11 | 退出群组 | 群列表更新 |
| 12 | 在群聊页发消息 | 在线成员实时收到 |

### 5.4 文件与分享

| # | 操作 | 预期 |
|---|------|------|
| 13 | 列目录/新建目录/重命名/移动/删除 | 状态栏反馈正确 |
| 14 | 上传文件 | 进度显示正确 |
| 15 | 下载文件 | 文件落地成功 |
| 16 | 分享文件给好友 | 对方收到并可接受 |

---

## 6. 环境注意事项

- 服务端构建：当前已在 WSL 环境验证可编译
- 客户端构建：需要本机安装 Qt6，并提供 `Qt6Config.cmake`
- 项目专用 MySQL：默认使用 `3308`，避免与其他容器或本机数据库冲突
