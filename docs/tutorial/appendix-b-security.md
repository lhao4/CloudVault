# 附录 B　安全加固

> **状态**：🔲 待写（随项目开发持续补充）

---

## 计划内容

### B.1 密码安全链路

- 为什么不能明文传输密码
- 客户端哈希 + 服务端二次哈希（双重保护）
- Salt 的作用：防彩虹表攻击

### B.2 SQL 注入防护

**错误做法**（v1 的问题）：

```cpp
// 危险！用户输入的 username 可能包含 SQL 代码
std::string sql = "SELECT * FROM user_info WHERE username = '" + username + "'";
```

**正确做法**：

```cpp
// 预处理语句，参数与 SQL 语句完全分离
MYSQL_STMT* stmt = mysql_stmt_init(conn);
const char* sql = "SELECT * FROM user_info WHERE username = ?";
mysql_stmt_prepare(stmt, sql, strlen(sql));
// 绑定参数
mysql_stmt_bind_param(stmt, bind);
mysql_stmt_execute(stmt);
```

### B.3 路径遍历防护

- `../` 攻击原理
- `std::filesystem::weakly_canonical` 的作用
- 前缀检查的实现

### B.4 输入校验

- 用户名长度限制（≤32 字符）
- 文件名合法性检查（禁止特殊字符）
- PDU 大小限制（MAX_PDU_SIZE = 64MB）
