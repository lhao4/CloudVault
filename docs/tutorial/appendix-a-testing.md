# 附录 A　测试与调试

> **状态**：🔲 待写（随项目开发持续补充）

---

## 计划内容

### A.1 单元测试

- GoogleTest 基础用法
- 测试 `PDUBuilder` / `PDUParser`
- 测试 `crypto_utils`
- Mock 对象的使用场景

### A.2 集成测试

- 服务端启动 + 客户端连接的自动化测试
- 数据库集成测试（真实 MySQL）

### A.3 手动联调测试

- 服务端日志的解读
- 客户端调试技巧（Qt Creator 调试器）

### A.4 GDB 调试服务端

```bash
# 编译时开启调试信息
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 使用 GDB 运行
gdb ./build/cloudhive_server
(gdb) run
(gdb) bt      # 崩溃后查看调用栈
```

### A.5 常见崩溃类型

| 崩溃类型 | 常见原因 | 排查方式 |
|---------|---------|---------|
| Segmentation Fault | 空指针解引用、越界访问 | GDB bt |
| Deadlock | 多线程锁顺序不一致 | gdb thread apply all bt |
| 内存泄漏 | 裸指针未释放 | valgrind |
