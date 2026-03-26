# 第十一章　文件管理

> **状态**：✅ 已完成
>
> **本章目标**：实现目录浏览、新建、重命名、移动、删除、搜索。
>
> **验收标准**：
> - 客户端文件页不再是静态演示数据
> - 可以浏览当前用户的个人文件空间
> - 可以新建文件夹、重命名、移动、删除
> - 支持按文件名搜索当前用户文件
> - 服务端能正确拒绝路径越权请求

---

## 11.1 本章完成了什么

第十章解决的是“好友之间能聊天”，第十一章开始解决“用户自己的文件空间怎么管理”。

本章落地了：

- 服务端 `FileStorage`
- 服务端 `FileHandler`
- 文件管理消息注册到 `ServerApp`
- 客户端 `FileService`
- 主界面文件页改为真实目录浏览
- 新建、重命名、移动、删除、搜索全链路打通

当前范围刻意控制在“文件管理”，不提前混入：

- 上传 / 下载
- 分块传输
- 文件分享

这些内容分别留到第十二章和第十三章。

---

## 11.2 数据流总览

```text
用户进入文件页
      ↓
MainWindow::navigateToFilePath("/")
      ↓
FileService::listFiles("/")
      ↓
TcpClient 发送 FLUSH_FILE
      ↓
ServerApp -> MessageDispatcher
      ↓
FileHandler::handleList()
      ↓
FileStorage::list()
      ↓
返回目录条目数组
      ↓
FileService 解析响应
      ↓
MainWindow 刷新文件列表
```

其他操作也遵循同样的结构：

```text
MainWindow
  ↓
FileService
  ↓
FileHandler
  ↓
FileStorage
  ↓
响应回到 MainWindow
```

---

## 11.3 协议设计

当前实现沿用 [protocol.h](/mnt/d/CloudVault/common/include/common/protocol.h) 中已有的文件管理消息类型：

```cpp
MKDIR
FLUSH_FILE
MOVE
DELETE_FILE
RENAME
SEARCH_FILE
```

注意：

- 当前代码没有再拆成 `REQUEST / RESPONSE` 两套枚举
- 同一个 `MessageType` 同时用于请求和响应
- 是否是请求还是响应，由发送方角色和 body 格式决定

这是本章的实际实现方式，后续如需更严格的协议分层，再统一重构即可。

### 11.3.1 列目录

**请求：`FLUSH_FILE`**

```text
dir_path : string
```

客户端传 `/` 表示用户根目录。

**响应：`FLUSH_FILE`**

成功时：

```text
status     : uint8 (0)
path       : string
entry_count: uint16
entries[]  :
    name        : string
    path        : string
    is_dir      : uint8
    size        : uint64
    modified_at : string
```

失败时：

```text
status  : uint8 (!=0)
message : string
```

### 11.3.2 新建文件夹

**请求：`MKDIR`**

```text
dir_name    : string
parent_path : string
```

**响应：`MKDIR`**

```text
status  : uint8
message : string
```

### 11.3.3 重命名

**请求：`RENAME`**

```text
target_path : string
new_name    : string
```

**响应：`RENAME`**

```text
status  : uint8
message : string
```

### 11.3.4 移动

**请求：`MOVE`**

```text
source_path      : string
destination_path : string
```

这里的 `destination_path` 表示目标目录路径。  
如果目标路径存在且是目录，服务端会把源文件 / 源目录移动进去。

**响应：`MOVE`**

```text
status  : uint8
message : string
```

### 11.3.5 删除

**请求：`DELETE_FILE`**

```text
target_path : string
```

**响应：`DELETE_FILE`**

```text
status  : uint8
message : string
```

### 11.3.6 搜索

**请求：`SEARCH_FILE`**

```text
keyword : string
```

注意这里没有再让客户端传 `username`。  
当前登录用户是谁，由服务端 `SessionManager` 反查，避免客户端伪造。

**响应：`SEARCH_FILE`**

成功时：

```text
status      : uint8 (0)
keyword     : string
result_count: uint16
entries[]   : 同 FLUSH_FILE
```

失败时：

```text
status  : uint8 (!=0)
message : string
```

---

## 11.4 服务端实现

### 11.4.1 FileStorage：文件系统操作封装

[file_storage.cpp](/mnt/d/CloudVault/server/src/file_storage.cpp) 是本章的核心。

它负责：

- 为每个用户维护独立根目录：`storage_root/username/`
- 目录列出
- 新建目录
- 重命名
- 移动
- 删除
- 递归搜索

当前实现不依赖数据库表 `file_info`，而是直接操作真实文件系统。

这样做的原因是：

- 第十一章只需要“管理目录结构”
- 上传 / 下载和分享还没开始做
- 真实文件系统已经足够支撑本章目标

数据库里的 `file_info` 表仍然保留，为后续章节扩展元数据管理预留。

### 11.4.2 路径安全：resolvePath()

最关键的是路径防护。

实现位置：

- [file_storage.h](/mnt/d/CloudVault/server/include/server/file_storage.h)
- [file_storage.cpp](/mnt/d/CloudVault/server/src/file_storage.cpp)

核心思路：

1. 先拿到用户根目录
2. 把客户端传来的逻辑路径去掉前导 `/`
3. 用 `std::filesystem::path(...).lexically_normal()` 规整路径
4. 拼接到用户根目录下
5. 再校验结果路径是否仍然位于用户根目录内部

也就是说，即使客户端恶意传：

```text
../../../etc/passwd
```

服务端也会直接拒绝，不会访问用户目录之外的文件。

### 11.4.3 FileHandler：协议解析与回包

[file_handler.cpp](/mnt/d/CloudVault/server/src/handler/file_handler.cpp) 负责：

- `handleList()`
- `handleMkdir()`
- `handleRename()`
- `handleMove()`
- `handleDelete()`
- `handleSearch()`

这里的设计和第九章、第十章一致：

- 当前用户身份来自 `SessionManager::findByConnection()`
- 不信任客户端自报“我是谁”
- 协议解析失败直接返回错误响应
- 文件系统异常统一转换成业务错误消息

### 11.4.4 ServerApp 接入

[server_app.cpp](/mnt/d/CloudVault/server/src/server_app.cpp) 在初始化阶段新增：

1. 创建 `FileStorage`
2. 创建 `FileHandler`
3. 注册文件管理消息

已注册消息：

- `FLUSH_FILE`
- `MKDIR`
- `RENAME`
- `MOVE`
- `DELETE_FILE`
- `SEARCH_FILE`

---

## 11.5 客户端实现

### 11.5.1 FileService

[file_service.cpp](/mnt/d/CloudVault/client/src/network/file_service.cpp) 负责：

- 构建文件管理请求 PDU
- 解析文件列表与搜索结果
- 解析新建 / 重命名 / 移动 / 删除操作结果
- 向 UI 发出信号

核心接口：

```cpp
listFiles(path)
createDirectory(parent_path, name)
renamePath(target_path, new_name)
movePath(source_path, destination_path)
deletePath(target_path)
search(keyword)
```

### 11.5.2 MainWindow 文件页

[main_window.cpp](/mnt/d/CloudVault/client/src/ui/main_window.cpp) 中原本的文件页只是静态卡片，现在已经换成真实数据驱动。

当前文件页具备：

- 顶部路径显示
- 搜索框（回车执行搜索）
- 返回上级目录
- 刷新当前目录
- 新建文件夹
- 文件 / 文件夹列表
- 选中项详情
- 重命名
- 移动
- 删除
- 双击目录进入下一级

### 11.5.3 当前 UI 语义

文件页当前管理的是：

- **当前登录用户自己的文件空间**

不是：

- 某个好友的共享空间

原因很简单：

- 第十一章目标是文件管理
- 文件分享属于第十三章
- 如果现在就把文件页绑定到“选中的好友”，会提前把分享模型耦合进来

因此，当前三栏主界面虽然保留了左侧联系人列表外壳，但文件页实际操作的是当前用户自己的工作区。

---

## 11.6 数据模型说明

[init.sql](/mnt/d/CloudVault/server/sql/init.sql) 里已经有 `file_info` 表：

```sql
CREATE TABLE IF NOT EXISTS file_info (
    file_id     INT          NOT NULL AUTO_INCREMENT PRIMARY KEY,
    owner_id    INT          NOT NULL,
    file_name   VARCHAR(255) NOT NULL,
    file_size   BIGINT       NOT NULL DEFAULT 0,
    file_path   VARCHAR(512) NOT NULL,
    parent_id   INT          NOT NULL DEFAULT 0,
    is_dir      TINYINT      NOT NULL DEFAULT 0,
    created_at  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

但本章没有真正使用这张表。

当前阶段的权衡是：

- **目录结构以真实文件系统为准**
- **元数据表先保留，不提前强行接入**

这样本章能先把目录操作链路稳定下来，后续再在上传 / 下载 / 分享阶段决定要不要把元数据真正纳入主路径。

---

## 11.7 联调步骤

### 11.7.1 启动服务端

```bash
export CLOUDVAULT_DB_PASSWORD=cloudvault_dev_123
/mnt/d/CloudVault/server/build/cloudvault_server /mnt/d/CloudVault/server/config/server.json
```

### 11.7.2 启动客户端

```bash
/mnt/d/CloudVault/client/cmake-build-debug-server/cloudvault_client
```

### 11.7.3 登录后验证文件页

建议按顺序测：

1. 进入“文件”页
2. 确认根目录能正常列出
3. 新建 `docs`
4. 双击进入 `docs`
5. 新建 `drafts`
6. 返回上一级
7. 重命名 `docs`
8. 搜索 `doc`
9. 删除测试目录

### 11.7.4 路径安全测试

可以用抓包或手工构造异常请求，验证这些路径应被拒绝：

```text
../../../etc/passwd
/../../secret
../../other_user/data
```

---

## 11.8 构建验证

客户端：

```bash
cmake -S client -B client/cmake-build-debug-server
cmake --build client/cmake-build-debug-server -j4
```

服务端：

```bash
cmake -S server -B server/build \
  -DBUILD_TESTING=OFF \
  -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON \
  -Dspdlog_DIR=/usr/lib/x86_64-linux-gnu/cmake/spdlog \
  -Dfmt_DIR=/usr/lib/x86_64-linux-gnu/cmake/fmt \
  -Dnlohmann_json_DIR=/usr/share/cmake/nlohmann_json

cmake --build server/build -j4
```

---

## 11.9 本章知识点

- `std::filesystem` 目录遍历与路径规整
- 路径遍历攻击与根目录边界校验
- Qt 列表控件驱动的文件浏览 UI
- 客户端操作型协议封装
- 服务端文件系统异常到业务错误的映射

---

## 11.10 小结

到本章结束，CloudVault 已经具备最基本的个人文件管理能力：

- 浏览目录
- 新建文件夹
- 重命名
- 移动
- 删除
- 搜索

下一章将在这个基础上继续实现真正的文件上传与下载。

---

上一章：[第十章 — 即时聊天](ch10-chat.md) ｜ 下一章：[第十二章 — 文件传输](ch12-file-transfer.md)
