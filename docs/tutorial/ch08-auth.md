# 第八章　注册与登录

> **状态**：🔲 待写
>
> **本章目标**：客户端输入用户名/密码，服务端落库并返回登录结果，实现完整的注册登录闭环。
>
> **验收标准**：
> - 客户端注册成功，MySQL 中 `user_info` 表有对应记录
> - 客户端登录成功，收到 `user_id`
> - 重复注册返回错误提示
> - 密码错误返回错误提示

---

## 本章大纲

### 8.1 数据库层（本章首次引入）

**`user_info` 表结构**：

```sql
CREATE TABLE user_info (
    user_id       INT          NOT NULL AUTO_INCREMENT PRIMARY KEY,
    username      VARCHAR(32)  NOT NULL UNIQUE,
    password_hash VARCHAR(128) NOT NULL,
    is_online     TINYINT      NOT NULL DEFAULT 0,
    created_at    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

需要实现的类：

- `Database`：MySQL 连接池，RAII 方式借用连接
- `UserRepository`：`insertUser()`、`findByName()`、`setOnline()`

### 8.2 密码安全链路

```
用户输入明文密码
      ↓
客户端：crypto::hashForTransport(password)  → 传输哈希
      ↓ TLS 加密传输
服务端：crypto::hashPassword(transport_hash) → 带 salt 存储哈希
      ↓ 存入 user_info.password_hash
登录时：crypto::verifyPassword(input, stored) → bool
```

### 8.3 服务端 AuthHandler（待完成）

- `handleRegister()`：解析 body → 检查用户名重复 → 存库 → 创建用户目录 → 回包
- `handleLogin()`：解析 body → 查用户 → 验证密码 → 注册 Session → 回包

### 8.4 客户端 AuthService（待完成）

- `registerUser()` / `login()`：构建 PDU，调用 `tcpClient->sendPDU()`
- `onRegisterRespond()` / `onLoginRespond()`：解析响应，emit 对应信号
- `LoginWindow` 连接信号，更新 UI 状态

### 8.5 联调测试步骤

1. 启动 MySQL，创建数据库和 `user_info` 表
2. 启动服务端
3. 启动客户端，注册新用户
4. 检查 MySQL：`SELECT * FROM user_info;`
5. 登录，验证收到 `user_id`

### 8.6 本章新知识点

- MySQL C API（`mysql_stmt_*` 预处理语句）
- RAII 连接池
- 密码哈希与 salt 机制

---

上一章：[第七章 — PING/PONG 联通](ch07-ping-pong.md) ｜ 下一章：[第九章 — 好友功能](ch09-friend.md)
