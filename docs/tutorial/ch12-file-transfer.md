# 第十二章　文件传输（上传与下载）

> **状态**：✅ 已完成
>
> **本章目标**：实现分片上传、分片下载和进度反馈，并把能力接入第十一章文件页。
>
> **验收标准**：
> - 客户端可从本地选择文件上传到当前目录
> - 客户端可将远端文件下载到本地目录
> - 大于 4MB 的文件会按多块传输
> - 上传 / 下载过程中页面能显示进度
> - 下载后的文件内容与服务端文件一致

---

## 12.1 本章完成了什么

第十一章解决的是“如何管理目录结构”，第十二章开始解决“文件内容如何真正传上去、再取回来”。

当前实现已经打通：

- 客户端 `FileService` 上传初始化
- 客户端按 4MB 分片发送文件内容
- 服务端按连接维护上传上下文并落盘
- 客户端下载初始化
- 服务端按 4MB 分片推送文件内容
- 客户端本地落盘并回传下载完成确认
- 文件页上传 / 下载按钮与进度条

本章刻意没有提前做：

- 断点续传
- 校验和 / 秒传
- 多任务并发传输
- 文件分享

这些能力继续留到后续章节或后续迭代。

---

## 12.2 数据流总览

### 12.2.1 上传

```text
MainWindow 选择本地文件
  ↓
FileService::uploadFile(local, remote_dir)
  ↓
发送 UPLOAD_REQUEST
  - filename(固定 32 字节)
  - file_size(uint64)
  - dir_path(string)
  ↓
服务端 FileHandler::handleUploadRequest()
  ↓
FileStorage::prepareUploadTarget()
  ↓
服务端返回“已就绪”
  ↓
FileService 分片读取本地文件
  ↓
循环发送 UPLOAD_DATA
  - chunk_size(uint32)
  - chunk_data
  ↓
服务端写入目标文件
  ↓
全部写完后返回上传结果
  ↓
MainWindow 刷新当前目录
```

### 12.2.2 下载

```text
MainWindow 选择远端文件 + 本地保存目录
  ↓
FileService::downloadFile(remote_path, local_dir)
  ↓
发送 DOWNLOAD_REQUEST(file_path)
  ↓
服务端 FileHandler::handleDownloadRequest()
  ↓
FileStorage::inspectPath()
  ↓
服务端返回：
  - status
  - file_size
  - filename(固定 32 字节)
  ↓
服务端循环发送 DOWNLOAD_DATA
  - chunk_size(uint32)
  - chunk_data
  ↓
客户端边收边写入本地文件
  ↓
全部接收完成后回发 DOWNLOAD_DATA 确认
```

---

## 12.3 协议设计

本章沿用 [protocol.h](/mnt/d/CloudVault/common/include/common/protocol.h) 中已有的四个消息类型：

```cpp
UPLOAD_REQUEST
UPLOAD_DATA
DOWNLOAD_REQUEST
DOWNLOAD_DATA
```

当前实现继续沿用“同一个 `MessageType` 同时承担请求与响应”的风格。

### 12.3.1 上传初始化

**请求：`UPLOAD_REQUEST`**

```text
filename : fixed_string(32)
file_size: uint64
dir_path : string
```

**响应：`UPLOAD_REQUEST`**

```text
status  : uint8
message : string
```

说明：

- 当前协议要求文件名 UTF-8 编码后不超过 32 字节
- 服务端在这一阶段只做路径校验、目标冲突校验和文件句柄准备

### 12.3.2 上传数据块

**请求：`UPLOAD_DATA`**

```text
chunk_size : uint32
chunk_data : bytes
```

当前分片大小常量位于 [protocol.h](/mnt/d/CloudVault/common/include/common/protocol.h)：

```cpp
FILE_TRANSFER_CHUNK_SIZE = 4 * 1024 * 1024
```

**响应：`UPLOAD_DATA`**

```text
status  : uint8
message : string
```

只有在整文件接收结束后，服务端才会回一次最终结果。

### 12.3.3 下载初始化

**请求：`DOWNLOAD_REQUEST`**

```text
file_path : string
```

**响应：`DOWNLOAD_REQUEST`**

成功时：

```text
status   : uint8 (0)
file_size: uint64
filename : fixed_string(32)
```

失败时：

```text
status  : uint8 (!=0)
message : string
```

### 12.3.4 下载数据块

**服务端推送：`DOWNLOAD_DATA`**

```text
chunk_size : uint32
chunk_data : bytes
```

**客户端确认：`DOWNLOAD_DATA`**

```text
status  : uint8
message : string
```

客户端在本地文件完整落盘后，回一条确认消息，服务端只记录日志，不再做额外状态流转。

---

## 12.4 服务端实现

### 12.4.1 FileStorage 扩展

[file_storage.h](/mnt/d/CloudVault/server/include/server/file_storage.h) /
[file_storage.cpp](/mnt/d/CloudVault/server/src/file_storage.cpp)

第十二章没有新建第二套存储层，而是在第十一章基础上增加了两个传输辅助接口：

- `inspectPath()`
- `prepareUploadTarget()`

职责分别是：

- 下载前检查路径、文件类型和文件大小
- 上传前检查目标目录是否合法、目标文件是否冲突

路径安全仍然统一由 `resolvePath()` / `ensureInsideRoot()` 保证。

### 12.4.2 FileHandler 扩展

[file_handler.h](/mnt/d/CloudVault/server/include/server/handler/file_handler.h) /
[file_handler.cpp](/mnt/d/CloudVault/server/src/handler/file_handler.cpp)

新增入口：

- `handleUploadRequest()`
- `handleUploadData()`
- `handleDownloadRequest()`
- `handleDownloadData()`
- `handleConnectionClosed()`

其中最关键的是上传上下文：

```cpp
struct UploadContext {
    std::string username;
    std::string logical_path;
    std::filesystem::path absolute_path;
    std::ofstream stream;
    uint64_t expected_size;
    uint64_t received_size;
};
```

服务端会按连接保存当前上传任务：

- 上传初始化成功后创建 `UploadContext`
- 每收到一块 `UPLOAD_DATA` 就写入 `ofstream`
- `received_size == expected_size` 时关闭文件并返回成功
- 如果连接中途断开，`handleConnectionClosed()` 会清理未完成文件

这样可以避免：

- 上传写到一半留下脏文件
- 不同连接之间相互串流
- 客户端随意越权写入别人的目录

### 12.4.3 下载流程

下载没有单独维护复杂上下文，服务端在确认文件存在且可读后：

1. 先回 `DOWNLOAD_REQUEST` 初始化响应
2. 再用 `ifstream` 循环读取文件
3. 每次按 4MB 发送一条 `DOWNLOAD_DATA`

这是当前版本最直接、最可维护的实现。

### 12.4.4 ServerApp 注册

[server_app.cpp](/mnt/d/CloudVault/server/src/server_app.cpp) 已新增注册：

- `UPLOAD_REQUEST`
- `UPLOAD_DATA`
- `DOWNLOAD_REQUEST`
- `DOWNLOAD_DATA`

并且在连接关闭回调中追加了：

```cpp
file_handler_->handleConnectionClosed(c);
```

确保上传中断时能自动清理半文件。

---

## 12.5 客户端实现

### 12.5.1 FileService

[file_service.h](/mnt/d/CloudVault/client/src/network/file_service.h) /
[file_service.cpp](/mnt/d/CloudVault/client/src/network/file_service.cpp)

第十二章继续沿用第十一章的 `FileService`，不再拆新类。

新增能力：

- `uploadFile(local_path, remote_dir_path)`
- `downloadFile(remote_file_path, local_dir_path)`
- `uploadProgress(...)`
- `uploadFinished(...)`
- `uploadFailed(...)`
- `downloadProgress(...)`
- `downloadFinished(...)`
- `downloadFailed(...)`

客户端内部维护两个轻量状态：

- `UploadContext`
- `DownloadContext`

#### 上传实现

上传流程：

1. 本地打开文件
2. 发送 `UPLOAD_REQUEST`
3. 服务端返回“已就绪”后
4. 客户端通过 `uploadNextChunk()` 按 4MB 连续发送
5. 每发一块就 `emit uploadProgress(...)`
6. 最终等待服务端返回 `UPLOAD_DATA` 结果

这里刻意用 `QTimer::singleShot(0, ...)` 继续下一块发送，而不是一次性同步读完整文件，目的是避免 UI 线程长时间卡死。

#### 下载实现

下载流程：

1. 发送 `DOWNLOAD_REQUEST`
2. 解析文件大小与文件名
3. 在本地选择目录下创建目标文件
4. 每收到一块 `DOWNLOAD_DATA` 就立刻写盘
5. 更新下载进度
6. 全部接收完成后回传成功确认

为了避免覆盖本地已有文件，当前实现会自动生成一个不冲突的新文件名。

### 12.5.2 MainWindow 文件页

[main_window.h](/mnt/d/CloudVault/client/src/ui/main_window.h) /
[main_window.cpp](/mnt/d/CloudVault/client/src/ui/main_window.cpp)

文件页新增了：

- 工具栏里的“上传”按钮
- 选中文件后的“下载”按钮
- 上传进度条
- 下载进度条
- 页内传输状态提示

当前交互：

- 点击“上传”后选择本地文件，上传到当前目录
- 选中一个远端文件后点击“下载”，选择本地目录并开始接收
- 目录列表仍然由第十一章的 `listFiles()` 负责
- 上传完成后会自动刷新当前目录

---

## 12.6 验证结果

### 12.6.1 构建验证

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

### 12.6.2 真实联调

已使用临时服务端和协议脚本验证：

1. 注册临时账号并登录
2. 上传一个 5.24MB 文件
3. 服务端按多块接收并落盘
4. 列目录确认远端文件大小正确
5. 下载同一文件
6. 客户端按多块接收并拼接
7. 对下载结果做 SHA-256 校验，与原始文件一致

本次联调结果：

```text
CH12_TRANSFER_OK ... 5243201 195357a35406253e5e038e9c33c7f23c971ab47f7ad3e2723e3373738a52e4ab
```

这说明：

- 上传分片顺序正确
- 服务端落盘结果正确
- 下载分片顺序正确
- 客户端本地落盘结果正确

---

## 12.7 当前边界与后续演进

本章已经完成“可用的文件传输主链路”，但还没有做下面这些增强项：

- 断点续传
- 多文件并行传输
- 失败重试
- 分片校验和
- 上传 / 下载任务列表
- 暂停 / 恢复

如果后续继续增强，建议顺序是：

1. 先做断点续传
2. 再做任务列表与失败重试
3. 最后补校验和和多任务并发

---

上一章：[第十一章 — 文件管理](ch11-file-manager.md) ｜ 下一章：[第十三章 — 文件分享](ch13-file-share.md)
