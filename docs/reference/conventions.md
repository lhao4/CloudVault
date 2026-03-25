# 项目规范

> 所有代码和文件必须遵守本规范，保持整个项目风格一致。

---

## 1. 项目目录结构

```
CloudVault/
├── common/                         # 客户端与服务端共享的协议库
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── common/
│   │       ├── constants.h
│   │       ├── message_types.h
│   │       ├── protocol.h
│   │       ├── protocol_codec.h
│   │       └── crypto_utils.h
│   └── src/
│       ├── protocol_codec.cpp
│       └── crypto_utils.cpp
│
├── server/                         # 服务端（Linux，纯 C++）
│   ├── CMakeLists.txt
│   ├── config/
│   │   └── server.json.example
│   ├── include/
│   │   └── server/
│   │       ├── server_app.h
│   │       ├── config.h
│   │       ├── event_loop.h
│   │       ├── tcp_server.h
│   │       ├── tcp_connection.h
│   │       ├── thread_pool.h
│   │       ├── session_manager.h
│   │       ├── file_storage.h
│   │       ├── db/
│   │       │   ├── database.h
│   │       │   ├── user_repository.h
│   │       │   └── friend_repository.h
│   │       └── handler/
│   │           ├── message_dispatcher.h
│   │           ├── auth_handler.h
│   │           ├── friend_handler.h
│   │           ├── chat_handler.h
│   │           └── file_handler.h
│   ├── src/
│   │   ├── main.cpp
│   │   ├── server_app.cpp
│   │   ├── config.cpp
│   │   ├── event_loop.cpp
│   │   ├── tcp_server.cpp
│   │   ├── tcp_connection.cpp
│   │   ├── thread_pool.cpp
│   │   ├── session_manager.cpp
│   │   ├── file_storage.cpp
│   │   ├── db/
│   │   │   ├── database.cpp
│   │   │   ├── user_repository.cpp
│   │   │   └── friend_repository.cpp
│   │   └── handler/
│   │       ├── message_dispatcher.cpp
│   │       ├── auth_handler.cpp
│   │       ├── friend_handler.cpp
│   │       ├── chat_handler.cpp
│   │       └── file_handler.cpp
│   └── tests/
│       ├── CMakeLists.txt
│       ├── test_pdu.cpp
│       └── test_crypto.cpp
│
├── client/                         # 客户端（Windows，Qt 6）
│   ├── CMakeLists.txt
│   ├── resources/
│   │   ├── images.qrc
│   │   └── images/
│   │       ├── file.png
│   │       └── folder.png
│   └── src/
│       ├── main.cpp
│       ├── app.h
│       ├── app.cpp
│       ├── network/
│       │   ├── tcp_client.h
│       │   ├── tcp_client.cpp
│       │   ├── response_router.h
│       │   └── response_router.cpp
│       ├── service/
│       │   ├── auth_service.h
│       │   ├── auth_service.cpp
│       │   ├── friend_service.h
│       │   ├── friend_service.cpp
│       │   ├── chat_service.h
│       │   ├── chat_service.cpp
│       │   ├── file_service.h
│       │   └── file_service.cpp
│       └── ui/
│           ├── login_window.h
│           ├── login_window.cpp
│           ├── login_window.ui
│           ├── main_window.h
│           ├── main_window.cpp
│           ├── main_window.ui
│           ├── friend_widget.h
│           ├── friend_widget.cpp
│           ├── friend_widget.ui
│           ├── file_widget.h
│           ├── file_widget.cpp
│           ├── file_widget.ui
│           ├── chat_window.h
│           ├── chat_window.cpp
│           ├── chat_window.ui
│           ├── online_user_dialog.h
│           ├── online_user_dialog.cpp
│           ├── online_user_dialog.ui
│           ├── share_file_dialog.h
│           ├── share_file_dialog.cpp
│           └── share_file_dialog.ui
│
├── docs/                           # 文档（MkDocs）
├── .github/
│   └── workflows/
│       └── docs.yml
└── mkdocs.yml
```

---

## 2. 命名规范

### 2.1 目录名

全部使用 **`snake_case`（小写 + 下划线）**。

| 正确 | 错误 |
|------|------|
| `server/` | `Server/` |
| `client/` | `Client/` |
| `common/` | `Common/` |
| `db/` | `DB/` |
| `handler/` | `Handler/` |

### 2.2 C++ 文件名

头文件和源文件均使用 **`snake_case`**。

| 类型 | 格式 | 示例 |
|------|------|------|
| 头文件 | `snake_case.h` | `tcp_server.h`、`auth_handler.h` |
| 源文件 | `snake_case.cpp` | `tcp_server.cpp`、`auth_handler.cpp` |
| Qt UI 文件 | `snake_case.ui` | `login_window.ui`、`file_widget.ui` |
| 测试文件 | `test_<模块名>.cpp` | `test_pdu.cpp`、`test_crypto.cpp` |

### 2.3 C++ 代码命名

| 元素 | 规范 | 示例 |
|------|------|------|
| 类名 | `PascalCase` | `TcpServer`、`AuthHandler` |
| 函数/方法名 | `camelCase` | `handleLogin()`、`sendPDU()` |
| 成员变量 | `snake_case_` + 下划线后缀 | `listen_fd_`、`thread_pool_` |
| 局部变量 | `snake_case` | `user_name`、`pdu_header` |
| 常量 | `UPPER_SNAKE_CASE` | `MAX_NAME_LEN`、`DEFAULT_PORT` |
| 枚举值 | `PascalCase` | `MessageType::LoginRequest` |
| 命名空间 | `snake_case` | `namespace crypto`、`namespace constants` |

### 2.4 配置文件

| 类型 | 格式 | 示例 |
|------|------|------|
| JSON 配置 | `snake_case.json` | `server.json` |
| 示例配置 | `<name>.json.example` | `server.json.example` |
| Qt 资源文件 | `snake_case.qrc` | `images.qrc` |

---

## 3. 头文件包含规范

服务端和客户端头文件统一放在 `include/<模块名>/` 下，包含时带模块前缀：

```cpp
// server 模块
#include "server/tcp_server.h"
#include "server/handler/auth_handler.h"
#include "server/db/database.h"

// common 模块（服务端和客户端均使用）
#include "common/protocol.h"
#include "common/crypto_utils.h"
```

---

## 4. CMake Target 命名

| Target | 名称 |
|--------|------|
| common 静态库 | `cloudvault_common` |
| server 可执行文件 | `cloudvault_server` |
| client 可执行文件 | `cloudvault_client` |
| 测试可执行文件 | `cloudvault_server_tests` |

---

## 5. Git 提交规范

使用 **Conventional Commits** 格式：

```
<type>(<scope>): <subject>
```

| type | 含义 |
|------|------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `refactor` | 重构（不改变外部行为） |
| `docs` | 文档变更 |
| `test` | 添加或修改测试 |
| `chore` | 构建系统、依赖更新等杂项 |
| `style` | 代码格式（不影响逻辑） |

**scope** 对应模块：`common`、`server`、`client`、`auth`、`friend`、`chat`、`file`、`docs`

示例：

```
feat(auth): add login request handler
fix(server): fix fd leak on client disconnect
docs(tutorial): add chapter 5 skeleton implementation
chore(cmake): upgrade to CMake 3.25 minimum version
```

---

## 6. 分支规范

| 分支 | 用途 |
|------|------|
| `master` | 稳定版本，每章功能完成后合并 |
| `dev` | 日常开发主分支 |
| `feat/<name>` | 功能开发，如 `feat/auth`、`feat/file-upload` |
| `fix/<name>` | Bug 修复 |
| `docs/<name>` | 仅文档改动 |
