# 第三章　开发环境搭建

> **本章目标**：把服务端（Linux）和客户端（Windows）的开发环境搭好，确保后续能正常编译。
>
> **验收标准**：`cmake --version`、`g++ --version`、`mysql --version` 均有正常输出；Qt Creator 能新建并运行一个空 Qt 项目。

---

## 3.1 Linux 服务端环境

### 安装编译工具链

```bash
sudo apt update
sudo apt install -y \
    gcc \
    g++ \
    cmake \
    ninja-build \
    git
```

验证安装：

```bash
g++ --version       # 应显示 GCC 11+ 或更高
cmake --version     # 应显示 3.20+
ninja --version
git --version
```

!!! tip "CMake 版本太旧怎么办？"
    Ubuntu 20.04 默认的 CMake 版本较旧。可以从 CMake 官方安装最新版：
    ```bash
    pip3 install cmake --upgrade
    # 或从 https://cmake.org/download/ 下载二进制包
    ```

### 安装项目依赖库

```bash
sudo apt install -y \
    libssl-dev \          # OpenSSL（TLS + SHA-256）
    libmysqlclient-dev \  # MySQL 客户端库
    mysql-server          # MySQL 服务（开发用）
```

验证 MySQL：

```bash
mysql --version         # 应显示 8.0+
sudo systemctl start mysql
sudo mysql -u root      # 能进入 MySQL 命令行即正常
```

### 推荐编辑器：VS Code（远程开发）

如果服务端跑在远程 Linux 机器或 WSL 上，推荐用 VS Code + Remote-SSH / WSL 扩展：

1. 安装 VS Code
2. 安装扩展：`C/C++`、`CMake Tools`、`Remote - SSH`（或 `WSL`）
3. 连接到 Linux 环境后，CMake Tools 会自动检测 `CMakeLists.txt`

---

## 3.2 Windows 客户端环境

### 安装 Qt 6

1. 前往 [Qt 官方下载页](https://www.qt.io/download-qt-installer) 下载 Qt 在线安装器
2. 运行安装器，登录 Qt 账号（免费注册）
3. 勾选安装组件：
   ```
   Qt 6.7.x（或最新 LTS 版本）
   └── Desktop
       ├── MSVC 2022 64-bit    ← 推荐（需要安装 Visual Studio）
       └── MinGW 64-bit        ← 备选（不需要 Visual Studio）
   ```
4. 同时勾选：`CMake`、`Ninja`（Qt 安装器自带，省去单独安装）

### 安装编译器

**方案 A：MSVC（推荐）**

安装 [Visual Studio 2022 Community](https://visualstudio.microsoft.com/)，勾选工作负载：
- "使用 C++ 的桌面开发"

**方案 B：MinGW**

通过 Qt 安装器附带的 MinGW 即可，无需额外安装。

### 配置 Qt6_DIR 环境变量

CMake 需要知道 Qt6 安装在哪里，设置方式：

```powershell
# 在 Windows 系统环境变量中添加（以 MSVC 为例）
Qt6_DIR = C:\Qt\6.7.0\msvc2022_64\lib\cmake\Qt6
```

或者每次 cmake 时传入：

```powershell
cmake -B build -DQt6_DIR="C:\Qt\6.7.0\msvc2022_64\lib\cmake\Qt6"
```

### 推荐编辑器：Qt Creator 或 VS Code

**Qt Creator**（推荐新手）：
- Qt 安装器自带
- 自动识别 CMakeLists.txt，配置简单
- 内置 Qt Designer（可视化设计 .ui 界面）

**VS Code**：
- 安装扩展：`C/C++`、`CMake Tools`
- 在 `settings.json` 中设置 `cmake.configureSettings.Qt6_DIR`

---

## 3.3 Git 基础配置

```bash
# 设置用户信息（首次使用 Git 必须配置）
git config --global user.name "你的名字"
git config --global user.email "你的邮箱"

# 设置默认分支名
git config --global init.defaultBranch master

# 验证
git config --list
```

日常开发流程：

```bash
git pull                    # 拉取最新代码
# ... 写代码 ...
git add <文件>              # 暂存修改
git commit -m "feat: ..."   # 提交（遵循 Conventional Commits）
git push                    # 推送到远程
```

详细说明见：[Git 使用指南](knowledge/git-guide.md)

---

## 3.4 验收清单

完成本章后，逐项检查：

- [ ] Linux：`g++ --version` 显示 GCC 11+
- [ ] Linux：`cmake --version` 显示 3.20+
- [ ] Linux：`mysql --version` 显示 8.0+
- [ ] Linux：`openssl version` 有正常输出
- [ ] Windows：Qt Creator 能新建并运行空 Qt Widgets 项目
- [ ] Windows：`cmake --version` 有正常输出
- [ ] Git：`git config user.name` 有正常输出

---

## 3.5 本章小结

- [x] 服务端 Linux 环境安装完毕
- [x] 客户端 Windows + Qt6 环境安装完毕
- [x] Git 基础配置完成

下一章：[第四章 — 工程初始化与 CMake 配置](ch04-cmake.md)
