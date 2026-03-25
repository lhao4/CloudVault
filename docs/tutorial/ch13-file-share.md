# 第十三章　文件分享

> **状态**：🔲 待写
>
> **本章目标**：用户将文件分享给好友，好友接受后文件复制到自己的目录。
>
> **验收标准**：A 分享文件给 B，B 接受后，B 的文件空间中出现该文件副本。

---

## 本章大纲

### 13.1 分享流程

```
A（发送方）                 服务端                  B（接收方）
    │                         │                        │
    ├─ SHARE_FILE_REQUEST ───►│                        │
    │  （文件路径 + 目标用户B） │                        │
    │                         │ 校验文件存在             │
    │                         ├─ SHARE_FILE_NOTIFY ───►│
    │◄── SHARE_FILE_RESP ─────┤  （发送者 + 文件名）     │
    │  （已通知）              │                        │
    │                         │         ◄── SHARE_FILE_ACCEPT_REQUEST
    │                         │ 将文件复制到 B 的目录    │
    │                         ├─ SHARE_FILE_ACCEPT_RESP►│
```

### 13.2 服务端实现（待完成）

- `handleShareFile()`：校验发送方权限、文件存在性，向接收方推送通知
- `handleShareAgree()`：调用 `FileStorage::copyFile(src, dst)`，复制到接收方目录

### 13.3 客户端实现（待完成）

**发送方**：
- 右键菜单选择"分享给好友"
- 弹出好友选择对话框（`ShareFileDialog`）
- 选择目标好友，发送 `SHARE_FILE_REQUEST`

**接收方**：
- 收到 `SHARE_FILE_NOTIFY` 推送
- 弹出通知："A 向你分享了文件 xxx，是否接受？"
- 接受 → 发送 `SHARE_FILE_ACCEPT_REQUEST`
- 拒绝 → 不发送，通知消失

### 13.4 联调测试

1. A、B 两个客户端同时登录
2. A 右键选择文件，分享给 B
3. B 弹出接受对话框，点击接受
4. B 的文件列表刷新，可以看到新文件
5. 服务端文件系统中 B 的目录下出现文件副本

---

上一章：[第十二章 — 文件传输](ch12-file-transfer.md) ｜ [返回教程目录](index.md)
