# 第十一章　文件管理

> **状态**：🔲 待写
>
> **本章目标**：实现目录浏览、新建、重命名、移动、删除、搜索。
>
> **验收标准**：客户端可以对服务端文件系统进行完整的目录操作，路径安全校验通过。

---

## 本章大纲

### 11.1 FileStorage 实现（待完成）

服务端文件存储的核心类，所有文件操作都通过它进行。

**最关键的方法 `resolvePath()`**（路径安全校验）：

```cpp
// 防止路径遍历攻击：如 ../../etc/passwd
std::filesystem::path FileStorage::resolvePath(
    const std::string& user_root,
    const std::string& relative)
{
    auto canonical = std::filesystem::weakly_canonical(
        std::filesystem::path(user_root) / relative);
    auto root = std::filesystem::weakly_canonical(user_root);

    // 确保结果路径以用户根目录为前缀
    if (!canonical.string().starts_with(root.string())) {
        throw SecurityException("Path traversal detected");
    }
    return canonical;
}
```

### 11.2 支持的文件操作

| 消息类型 | 操作 |
|---------|------|
| `FILE_LIST_REQUEST` | 列出目录内容 |
| `MKDIR_REQUEST` | 新建文件夹 |
| `DELETE_FILE_REQUEST` | 删除文件或文件夹 |
| `RENAME_FILE_REQUEST` | 重命名 |
| `MOVE_FILE_REQUEST` | 移动文件/文件夹 |
| `SEARCH_FILE_REQUEST` | 按文件名模糊搜索 |

### 11.3 客户端 FileService 与 UI（待完成）

- 文件列表展示：表格视图，显示名称、类型（文件/文件夹）
- 双击进入目录，面包屑导航显示当前路径
- 右键菜单：重命名、移动、删除

### 11.4 路径安全测试

联调测试时需要验证以下路径都被拒绝：

```
../../../etc/passwd
/etc/passwd（绝对路径）
../../other_user/secret.txt
```

### 11.5 本章新知识点

- `std::filesystem` 详解
- 路径遍历攻击与防护
- Qt 文件浏览 Widget

---

上一章：[第十章 — 即时聊天](ch10-chat.md) ｜ 下一章：[第十二章 — 文件传输](ch12-file-transfer.md)
