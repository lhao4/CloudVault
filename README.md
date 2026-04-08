# CloudVault 云巢

> 面向小型团队的私有化云文件管理与即时通讯系统

CloudVault 是一个完整的 C/S 架构桌面应用。服务端运行在 Linux，客户端运行在 Windows（Qt 6）。所有数据托管在自己的服务器上，无需依赖任何第三方云服务。

---

## 功能特性

### 即时通讯
- **私聊**：好友间实时文本消息，服务端转发投递
- **离线消息**：对方不在线时消息写入数据库，上线后自动补投
- **群聊**：创建群组、加入/退出群、实时广播与离线补投
- **聊天记录**：所有消息持久化，支持历史查询

### 好友系统
- 按用户名精确搜索用户
- 发送/接受好友申请
- 好友列表实时在线状态显示
- 删除好友

### 文件管理
- 个人云盘目录浏览、新建文件夹、重命名、移动、删除
- 文件名模糊搜索
- 大文件分片上传/下载（默认 4 MB 分片，支持 20 GB+ 文件）
- 实时传输进度条

### 文件分享
- 将云盘文件分享给在线好友
- 好友确认后文件复制到其个人云盘

### 安全
- 密码 SHA-256 + 随机 salt 哈希存储，永不明文传输
- 全部数据库操作使用 Prepared Statements，防 SQL 注入
- 文件路径严格校验，防路径遍历攻击

---

## 技术栈

| 层级 | 服务端 | 客户端 |
|------|--------|--------|
| 语言 | C++17 | C++17 + Qt 6 Widgets |
| 网络 | POSIX socket + epoll | QTcpSocket |
| 数据库 | MySQL 8.0（libmysqlclient） | — |
| 构建 | CMake 3.20+ | CMake 3.20+ |
| 日志 | spdlog | qDebug |
| 配置 | nlohmann/json | QJsonDocument |
| 密码 | OpenSSL SHA-256 | 共享 common 库 |
| 部署 | 裸机 / Docker Compose | Windows |

---

## 项目结构

```
CloudVault/
├── common/                  # 客户端与服务端共享协议库
│   ├── include/common/
│   │   ├── protocol.h       # PDU 头部、MessageType 枚举、常量
│   │   ├── protocol_codec.h # PDUBuilder / PDUParser
│   │   └── crypto_utils.h   # SHA-256 + salt 密码工具
│   └── src/
│
├── server/                  # 服务端（Linux，纯 C++）
│   ├── config/
│   │   └── server.example.json
│   ├── include/server/
│   │   ├── server_app.h
│   │   ├── event_loop.h     # epoll 事件循环
│   │   ├── tcp_server.h
│   │   ├── tcp_connection.h
│   │   ├── thread_pool.h
│   │   ├── session_manager.h
│   │   ├── message_dispatcher.h
│   │   ├── file_storage.h
│   │   ├── db/              # Repository 层
│   │   │   ├── database.h
│   │   │   ├── user_repository.h
│   │   │   ├── friend_repository.h
│   │   │   ├── chat_repository.h
│   │   │   └── group_repository.h
│   │   └── handler/         # 业务处理层
│   │       ├── auth_handler.h
│   │       ├── friend_handler.h
│   │       ├── chat_handler.h
│   │       ├── group_handler.h
│   │       ├── file_handler.h
│   │       └── share_handler.h
│   ├── sql/
│   │   └── init.sql         # 数据库初始化脚本
│   └── tests/
│
├── client/                  # 客户端（Windows，Qt 6）
│   ├── resources/
│   │   ├── resources.qrc
│   │   ├── icons/
│   │   └── styles/
│   │       └── style.qss    # 全局 QSS 样式表
│   └── src/
│       ├── app.h / app.cpp  # 应用装配层
│       ├── network/         # TcpClient + ResponseRouter
│       ├── service/         # 6 个 Service 类（业务层）
│       └── ui/              # 11 个面板/弹窗
│
├── docker-compose.mysql.yml # 项目专用 MySQL 容器
├── docs/                    # 完整文档站（MkDocs）
└── mkdocs.yml
```

---

## 快速开始

### 前置依赖

**服务端（Linux）**

| 依赖 | 版本要求 |
|------|---------|
| GCC / Clang | 支持 C++17 |
| CMake | ≥ 3.20 |
| MySQL 客户端库 | 8.0（libmysqlclient-dev） |
| OpenSSL | ≥ 1.1 |
| spdlog | ≥ 1.10 |
| nlohmann/json | ≥ 3.10 |

Ubuntu / Debian 一键安装：

```bash
sudo apt update
sudo apt install -y build-essential cmake libmysqlclient-dev \
    libssl-dev libspdlog-dev nlohmann-json3-dev
```

**客户端（Windows）**

| 依赖 | 版本要求 |
|------|---------|
| MSVC 或 MinGW | 支持 C++17 |
| CMake | ≥ 3.20 |
| Qt 6 | ≥ 6.2（Widgets + Network 模块） |

---

### 1. 启动数据库

项目提供开箱即用的 MySQL 容器，无需手动建库建表：

```bash
docker compose -f docker-compose.mysql.yml up -d
```

验证：

```bash
mysql -h127.0.0.1 -P3308 -ucloudvault_app -pcloudvault_dev_123 cloudvault \
    -e "SHOW TABLES;"
```

期望看到 6 张表：`user_info`、`friend`、`chat_group`、`group_member`、`chat_message`、`offline_message`。

---

### 2. 构建并运行服务端

```bash
# 进入服务端目录，配置构建
cmake -S server -B server/build -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build server/build -j$(nproc)

# 复制并编辑配置文件
cp server/config/server.example.json server/config/server.json
# 编辑 server.json，填写数据库密码等参数（见下方配置说明）

# 设置数据库密码环境变量
export CLOUDVAULT_DB_PASSWORD=cloudvault_dev_123

# 运行
./server/build/cloudvault_server server/config/server.json
```

---

### 3. 构建并运行客户端

在 Windows 上，使用 Qt Creator 或命令行：

```powershell
cmake -S client -B client/build -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"

cmake --build client/build --config Release -j4

# 运行
.\client\build\Release\cloudvault_client.exe
```

客户端启动后在登录界面填写服务端 IP 和端口（默认 `5001`）即可连接。

---

## 配置文件

### 服务端 `server/config/server.json`

```json
{
    "server": {
        "host": "0.0.0.0",
        "port": 5001,
        "thread_count": 8
    },
    "database": {
        "host": "127.0.0.1",
        "port": 3308,
        "name": "cloudvault",
        "user": "cloudvault_app",
        "password_env": "CLOUDVAULT_DB_PASSWORD"
    },
    "storage": {
        "root": "/data/cloudvault/filesys"
    },
    "log": {
        "level": "info",
        "file": "../logs/server.log"
    }
}
```

| 字段 | 说明 |
|------|------|
| `server.port` | 监听端口，客户端填写此端口连接 |
| `server.thread_count` | 工作线程数，建议设为 CPU 核心数 |
| `database.password_env` | 数据库密码从此环境变量读取，不写入配置文件 |
| `storage.root` | 用户文件存储根目录，确保服务进程有读写权限 |

---

## 数据库

项目使用 6 张表：

| 表名 | 用途 |
|------|------|
| `user_info` | 用户账号、密码哈希、昵称、签名、在线状态 |
| `friend` | 好友申请与确认关系 |
| `chat_group` | 群组信息（群名、群主） |
| `group_member` | 群成员关系 |
| `chat_message` | 私聊与群聊消息持久化 |
| `offline_message` | 离线消息队列 |

初始化脚本位于 `server/sql/init.sql`，Docker 容器启动时自动执行，无需手动导入。

---

## 运行测试

```bash
# 构建时开启测试
cmake -S server -B server/build -DBUILD_TESTING=ON
cmake --build server/build -j$(nproc)

# 运行服务端单元测试
ctest --test-dir server/build --output-on-failure

# common 库测试
cmake -S common -B common/build -DBUILD_TESTING=ON
cmake --build common/build
ctest --test-dir common/build --output-on-failure
```

---

## 架构概览

```
客户端（Windows）
  UI Panel / Dialog
      ↕ Qt 信号槽
  Service 层（AuthService / FriendService / ChatService / GroupService / FileService / ShareService）
      ↕ 发包 / 收包
  Network 层（TcpClient + ResponseRouter）
      ↕
      TCP 长连接 + 自定义二进制 PDU
      ↕
服务端（Linux）
  TcpServer + epoll EventLoop
      ↓
  ThreadPool（工作线程）
      ↓
  MessageDispatcher（按 MessageType 路由）
      ↓
  Handler（Auth / Friend / Chat / Group / File / Share）
      ↓
  Repository（MySQL） + FileStorage（文件系统）
```

### PDU 格式

所有消息使用 16 字节固定头部：

```
0        4        8        12       16
┌────────┬────────┬────────┬────────┐
│ total  │  body  │  msg   │ reserv │
│ length │ length │  type  │  ed    │
└────────┴────────┴────────┴────────┘
         Body（body_length 字节）
```

所有字段网络字节序（大端），Body 使用自定义二进制编码。

---

## 文档

完整文档见 `docs/` 目录，使用 MkDocs 构建：

| 文档 | 路径 |
|------|------|
| 产品需求文档（PRD） | `docs/reference/prd.md` |
| 系统架构 | `docs/reference/architecture.md` |
| API / 协议规范 | `docs/reference/api.md` |
| 数据库设计 | `docs/reference/database.md` |
| 模块划分 | `docs/reference/modules.md` |
| 客户端 UI 规范 | `docs/reference/client-ui-spec.md` |
| 项目规范（命名/提交） | `docs/reference/conventions.md` |
| 测试方案 | `docs/reference/testing.md` |
| 实战教程（13 章） | `docs/tutorial/` |

本地预览文档站：

```bash
pip install mkdocs-material
mkdocs serve
```

---

## 项目现状

| 功能模块 | 状态 |
|---------|------|
| 注册 / 登录 / 登出 / 资料同步 | ✅ 完整 |
| 好友系统（搜索、申请、同意、删除、列表） | ✅ 完整 |
| 私聊（实时 + 离线消息 + 历史记录） | ✅ 完整 |
| 群组（创建、加入、退出、列表） | ✅ 完整 |
| 群聊（实时广播 + 离线补投） | ✅ 完整 |
| 文件管理（目录、重命名、移动、删除、搜索） | ✅ 完整 |
| 文件传输（分片上传/下载 + 进度反馈） | ✅ 完整 |
| 文件分享（发送通知 + 接受复制） | ✅ 完整 |
| TLS 加密传输 | 规划中 |
| 客户端自动化测试 | 规划中 |

---

## 开发规范

- 代码风格：C++17，类名 `PascalCase`，成员变量 `snake_case_` 尾下划线
- 提交规范：[Conventional Commits](https://www.conventionalcommits.org/)（`feat/fix/refactor/docs/style/chore`）
- 分支策略：`master` 稳定，`feat/<name>` 功能开发，`fix/<name>` 修复

详见 `docs/reference/conventions.md`。
