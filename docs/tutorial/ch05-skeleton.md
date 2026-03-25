# 第五章　两端骨架——让程序先跑起来

> **状态**：🔲 待写
>
> **本章目标**：
>
> 1. 服务端启动后输出 `[info] Server started on port 5000`
> 2. 客户端弹出登录窗口，进入 Qt 事件循环
>
> **验收标准**：两个程序都能独立编译、独立启动，不崩溃。

---

## 本章大纲

### 5.1 服务端骨架

- `config.h / config.cpp`：从 `server.json` 读取配置（端口、数据库、路径等）
- `main.cpp`：加载配置 → 初始化 spdlog 日志 → 启动阻塞循环

**关键代码（待完成）**：

```cpp
// server/src/main.cpp
int main(int argc, char* argv[]) {
    // 1. 加载配置
    // 2. 初始化日志
    // 3. 打印启动信息
    // 4. 阻塞（暂用 pause()，后续替换为 EventLoop）
}
```

### 5.2 客户端骨架

- `main.cpp`：`QApplication` 初始化与事件循环
- `login_window.h / login_window.cpp`：最简登录窗口（只有输入框和按钮，不发网络请求）

**关键代码（待完成）**：

```cpp
// client/src/main.cpp
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    LoginWindow window;
    window.show();
    return app.exec();
}
```

### 5.3 本章踩坑记录

待实际编写时补充，预期常见问题：

1. `vtable for LoginWindow undefined` — AUTOMOC 配置问题
2. `Qt6Config.cmake not found` — Qt6_DIR 未设置
3. `nlohmann/json` FetchContent 下载失败 — 网络问题，需配置代理

### 5.4 本章新知识点

- [CMake 构建系统详解](knowledge/cmake-guide.md)
- [Qt 信号槽机制详解](knowledge/qt-signals-slots.md)
- spdlog 基础用法

---

上一章：[第四章 — 工程初始化与 CMake 配置](ch04-cmake.md) ｜ 下一章：[第六章 — 协议基础](ch06-protocol.md)
