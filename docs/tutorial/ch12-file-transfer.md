# 第十二章　文件传输（上传与下载）

> **状态**：🔲 待写
>
> **本章目标**：实现分片上传下载与进度反馈，支持 20GB 大文件。
>
> **验收标准**：
> - 小文件（<1MB）上传下载正确
> - 大文件（>100MB）分片稳定完成，进度条正常更新
> - 上传中断后重连可继续（断点续传）

---

## 本章大纲

### 12.1 为什么要分片传输

单个 PDU 不能无限大（限制为 64MB），大文件必须切片发送。

分片传输的好处：
- 支持超大文件（理论无上限）
- 可以显示传输进度
- 支持断点续传（记录已传分片，重连后跳过）

### 12.2 上传协议流程

```
客户端                              服务端
   │                                 │
   ├─ UPLOAD_INIT（文件名、大小、路径）─►│ 打开文件句柄，记录上传上下文
   │◄─────────── UPLOAD_INIT_RESP ───┤ 返回成功/失败
   │                                 │
   ├─ UPLOAD_CHUNK（offset, data）───►│ 按 offset 写入数据
   ├─ UPLOAD_CHUNK ...               │
   ├─ UPLOAD_CHUNK ...               │
   │◄─────────── UPLOAD_FINISH_RESP ─┤ 全部写完，关闭文件，校验
```

### 12.3 下载协议流程

```
客户端                              服务端
   │                                 │
   ├─ DOWNLOAD_REQUEST（路径）────────►│ 检查文件存在，返回文件大小
   │◄────── DOWNLOAD_INIT_RESP ───────┤
   │                                 │
   │◄────── DOWNLOAD_CHUNK（data）────┤ 服务端主动推送分片
   │◄────── DOWNLOAD_CHUNK ...        │
   │◄────── DOWNLOAD_FINISH ──────────┤ 传输完成
```

### 12.4 断点续传设计（待完成）

客户端在本地记录已上传的分片序号，断线重连后发送 `UPLOAD_RESUME` 消息，
服务端根据已写入的文件大小告知从哪个 offset 继续。

### 12.5 进度条更新（待完成）

```cpp
// FileService 每收到一个 DOWNLOAD_CHUNK 就 emit 进度信号
emit downloadProgress(received_bytes, total_bytes);

// FileWidget 连接信号更新进度条
connect(fileService_, &FileService::downloadProgress,
        progressBar_, [this](int64_t recv, int64_t total) {
            progressBar_->setValue(recv * 100 / total);
        });
```

---

上一章：[第十一章 — 文件管理](ch11-file-manager.md) ｜ 下一章：[第十三章 — 文件分享](ch13-file-share.md)
