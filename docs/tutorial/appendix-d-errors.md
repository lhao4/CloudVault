# 附录 D　常见错误汇总

> **状态**：🔲 持续更新（每次踩坑后记录在这里）

---

## 记录格式

每条错误按以下格式记录：

```
### 错误现象
### 出现场景（第几章、什么操作）
### 根本原因
### 解决方法
```

---

## CMake / 构建类

### `Could not find Qt6`

**场景**：第四章，客户端 `cmake -B build`

**原因**：`Qt6_DIR` 未设置，CMake 找不到 Qt6 安装位置

**解决**：
```bash
cmake -B build -DQt6_DIR="C:/Qt/6.7.0/msvc2022_64/lib/cmake/Qt6"
```
或设置系统环境变量 `Qt6_DIR`。

---

### `vtable for ClassName undefined`

**场景**：第五章，编译客户端

**原因**：含有 `Q_OBJECT` 宏的类的 .h 文件没有被 AUTOMOC 处理，通常是因为 .h 文件没有加入 `add_executable` 的文件列表

**解决**：在 `file(GLOB_RECURSE)` 中同时收集 `.h` 文件：
```cmake
file(GLOB_RECURSE CLIENT_SOURCES
    src/*.cpp
    src/*.h    # 必须包含头文件
)
```

---

### `FetchContent download failed`

**场景**：第四/五章，服务端首次 `cmake -B build`

**原因**：网络无法访问 GitHub

**解决**：配置代理，或使用镜像源

---

## 运行时错误类

*（随开发进度持续补充）*
