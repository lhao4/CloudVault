# 第十三章　文件分享

> **状态**：✅ 已完成
>
> **本章目标**：用户将自己文件空间中的现有文件分享给在线好友，好友确认后，文件副本复制到自己的根目录。
>
> **验收标准**：
> - A 选择文件并选择好友 B 发起分享
> - B 收到分享通知并点击接受
> - 服务端将文件复制到 B 的个人根目录
> - B 刷新文件列表后可看到新文件

---

## 13.1 本章完成了什么

第十一章解决了“如何管理个人文件空间”，第十三章解决的是“如何把已有文件交给好友”。

当前实现已经完成：

- 服务端 `ShareHandler`
- 客户端 `ShareService`
- 文件分享弹窗 `ShareFileDialog`
- 发送方文件页“分享”按钮
- 接收方在线分享通知弹窗
- 接收后服务端复制文件到接收方根目录

本章当前边界：

- 只支持分享普通文件，不支持目录
- 只支持分享给在线好友
- 接收后固定复制到接收方根目录 `/`
- 不做离线分享通知持久化
- 不做“拒绝后回执给发送方”

---

## 13.2 数据流总览

```text
A 在文件页选中 /report.pdf
  ↓
MainWindow::openShareFileDialog()
  ↓
ShareFileDialog 选择好友 B
  ↓
ShareService::shareFile("/report.pdf", ["B"])
  ↓
发送 SHARE_REQUEST
  ↓
服务端 ShareHandler::handleShareRequest()
  - 校验 A 已登录
  - 校验目标用户存在
  - 校验 A/B 是好友
  - 校验文件存在且是普通文件
  - 校验 B 在线
  ↓
服务端推送 SHARE_NOTIFY 给 B
  ↓
B 收到通知，MainWindow 弹出确认框
  ↓
B 点击“接受”
  ↓
ShareService::acceptShare(A, "/report.pdf")
  ↓
发送 SHARE_AGREE_REQUEST
  ↓
服务端 ShareHandler::handleShareAgree()
  - 校验 B 已登录
  - 校验请求确实来自待处理分享
  - 调用 FileStorage::copyFileToUser()
  ↓
复制到 B 的根目录 /report.pdf
  ↓
返回 SHARE_AGREE_RESPOND
  ↓
B 刷新文件列表后看到副本
```

---

## 13.3 协议设计

当前实现使用 `common/include/common/protocol.h` 中已有的四个消息类型：

```cpp
SHARE_REQUEST
SHARE_NOTIFY
SHARE_AGREE_REQUEST
SHARE_AGREE_RESPOND
```

这一章继续沿用当前项目的风格：

- 同一个 `MessageType` 同时承担请求和响应
- 发送方身份由服务端会话反查，不信任客户端自报用户名

### 13.3.1 发起分享

**请求：`SHARE_REQUEST`**

```text
friend_count           : uint32
friends[friend_count]  : fixed_string(32)
file_path              : string
```

说明：

- 实际实现里没有再传 `from_user`
- 当前发送方身份由服务端 `SessionManager` 反查

**响应：`SHARE_REQUEST`**

```text
status  : uint8
message : string
```

成功时消息类似：

```text
已向 1 位好友发送分享请求
```

### 13.3.2 分享通知

**服务端推送：`SHARE_NOTIFY`**

```text
from_user : fixed_string(32)
file_path : string
```

客户端收到后弹出确认框：

```text
“A 向你分享了文件 /shared.txt，是否接收？”
```

### 13.3.3 接受分享

**请求：`SHARE_AGREE_REQUEST`**

```text
sender_name : fixed_string(32)
source_path : string
```

**响应：`SHARE_AGREE_RESPOND`**

```text
status  : uint8
message : string
```

成功时消息类似：

```text
已接收分享，文件已复制到 /shared.txt
```

---

## 13.4 服务端实现

### 13.4.1 FileStorage 扩展

`server/include/server/file_storage.h` /
`server/src/file_storage.cpp`

第十三章在第十一章的文件系统封装基础上增加了两个接口：

- `inspectPath()`
- `copyFileToUser()`

`inspectPath()` 负责：

- 解析并校验路径
- 判断是目录还是普通文件
- 返回逻辑路径、绝对路径和大小

`copyFileToUser()` 负责：

- 校验源路径合法
- 拒绝目录分享
- 将文件复制到接收方根目录
- 若接收方已存在同名文件则拒绝

这样分享逻辑仍然统一走 `FileStorage`，不会把路径拼接散落到 handler 里。

### 13.4.2 ShareHandler

`server/include/server/handler/share_handler.h` /
`server/src/handler/share_handler.cpp`

这是本章的核心处理器。

它内部依赖：

- `UserRepository`
- `FriendRepository`
- `SessionManager`
- `FileStorage`

#### handleShareRequest()

职责：

1. 校验当前连接已登录
2. 解析目标好友列表和源文件路径
3. 校验源文件存在且不是目录
4. 逐个校验目标好友：
   - 用户存在
   - 与发送方是好友
   - 当前在线
5. 向可达目标发送 `SHARE_NOTIFY`
6. 把这条分享记进内存态 `pending_shares_`

当前只要至少有一个目标在线并成功通知，就返回成功。

#### handleShareAgree()

职责：

1. 校验当前连接已登录
2. 校验来源用户存在且双方仍是好友
3. 校验这条分享请求确实在 `pending_shares_` 中
4. 调用 `FileStorage::copyFileToUser()`
5. 复制成功后移除待处理记录

### 13.4.3 待处理分享请求

和第九章好友申请一样，这里也做了最小内存态校验。

结构大致是：

```cpp
std::unordered_map<std::string, std::unordered_set<std::string>> pending_shares_;
```

键的逻辑是：

```text
receiver_username -> { sender + '\n' + file_path }
```

用途：

- 防止客户端伪造“接受分享”请求
- 只允许对已收到的在线通知做确认

连接断开时，当前用户的待处理分享请求会被清掉，不做离线保留。

### 13.4.4 ServerApp 接入

`server/src/server_app.cpp` 已接入：

- `ShareHandler` 初始化
- `SHARE_REQUEST` 注册
- `SHARE_AGREE_REQUEST` 注册
- 连接关闭时调用 `share_handler_->handleConnectionClosed()`

---

## 13.5 客户端实现

### 13.5.1 ShareService

`client/src/service/share_service.h` /
`client/src/service/share_service.cpp`

本章新增客户端分享服务层，职责是：

- 发起分享请求
- 接收分享结果
- 接收服务端推送的分享通知
- 发起“接受分享”
- 接收接受分享结果

暴露的主要信号：

- `shareRequestSent`
- `shareRequestFailed`
- `incomingShareRequest`
- `shareAccepted`
- `shareAcceptFailed`

### 13.5.2 ShareFileDialog

`client/src/ui/share_file_dialog.h` /
`client/src/ui/share_file_dialog.cpp`

这个弹窗在本章真正接入主流程。

当前能力：

- 显示好友列表
- 搜索好友
- 多选好友
- 确认后把目标好友列表回传给主窗口

### 13.5.3 MainWindow 接入

`client/src/ui/main_window.h` /
`client/src/ui/main_window.cpp`

当前文件页新增了“分享”按钮：

- 只有选中普通文件时可用
- 目录不允许分享

发送方流程：

1. 在文件页选中文件
2. 点击“分享”
3. 弹出 `ShareFileDialog`
4. 选中目标好友
5. 调用 `ShareService::shareFile()`

接收方流程：

1. 收到 `incomingShareRequest(from, file_path)`
2. 主窗口弹出确认框
3. 用户点击“接受”
4. 调用 `ShareService::acceptShare()`
5. 成功后刷新根目录文件列表

### 13.5.4 LoginWindow 注入

`client/src/ui/login_window.h` /
`client/src/ui/login_window.cpp`

登录成功后，主窗口现在会同时拿到：

- `FriendService`
- `FileService`
- `ShareService`

这样分享能力仍复用同一条 TCP 连接，不会额外建连接。

---

## 13.6 验证结果

### 13.6.1 构建验证

客户端构建通过：

```bash
cmake -S client -B client/cmake-build-debug-server
cmake --build client/cmake-build-debug-server -j4
```

服务端构建通过：

```bash
cmake -S server -B server/build \
  -DBUILD_TESTING=OFF \
  -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON \
  -Dspdlog_DIR=/usr/lib/x86_64-linux-gnu/cmake/spdlog \
  -Dfmt_DIR=/usr/lib/x86_64-linux-gnu/cmake/fmt \
  -Dnlohmann_json_DIR=/usr/share/cmake/nlohmann_json

cmake --build server/build -j4
```

### 13.6.2 真实联调

已使用临时服务端和双客户端协议脚本完成联调：

1. A、B 注册并登录
2. 直接写入 `friend_relation`，建立好友关系
3. 在 A 的文件空间预置一个 `shared.txt`
4. A 向 B 发送分享请求
5. B 收到 `SHARE_NOTIFY`
6. B 发送 `SHARE_AGREE_REQUEST`
7. 服务端把文件复制到 B 的根目录
8. B 列目录确认 `/shared.txt` 存在且大小正确

联调结果：

```text
CH13_SHARE_OK share_a_xxx share_b_xxx 3072
```

这说明主流程已经闭环：

- 分享通知成功送达
- 接受成功
- 服务端复制成功
- 接收方文件列表可见

---

## 13.7 当前边界

这一章当前还没有做：

- 离线分享通知
- 拒绝分享回执
- 分享目录
- 分享到自定义目标目录
- 发送方查看分享历史

如果后续继续增强，建议顺序是：

1. 离线分享通知持久化
2. 拒绝 / 过期状态回执
3. 自定义保存目录
4. 目录分享或压缩后分享

---

上一章：[第十二章 — 文件传输](ch12-file-transfer.md) ｜ [返回教程目录](index.md)
