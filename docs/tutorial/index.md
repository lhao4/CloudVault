# CloudVault 云巢 — 项目实战开发手册（功能推进版）

> 面向读者：刚学完 C++、第一次做完整 C/S 项目的初学者。  
> 教学目标：**客户端与服务端同步推进，不先搭完整框架，每步都可运行与测试**。

## 目录

| 章节 | 内容概要 | 状态 |
|------|----------|------|
| [第一章　项目背景与需求分析](#第一章项目背景与需求分析) | 项目来源、定位、需求、v2目标（Bug分析移出） | ✅ 保留（精简） |
| [第二章　技术选型与架构概览](#第二章技术选型与架构概览) | 只保留高层决策，不展开实现细节 | ✅ 保留（精简） |
| [第三章　开发环境搭建](#第三章开发环境搭建) | Linux/Windows 环境、工具安装与验证 | ✅ 保留 |
| [第四章　工程初始化与 CMake 配置](#第四章工程初始化与-cmake-配置) | 项目骨架、构建系统、首次编译 | ✅ 保留（精简） |
| [第五章　两端骨架——让程序先跑起来](#第五章两端骨架让程序先跑起来) | 服务端最小启动 + 客户端登录窗可运行 | 🔲 新写 |
| [第六章　协议基础——PDU 与 common 层](#第六章协议基础pdu-与-common-层) | PDU、粘包拆包、crypto 与单测 | 🔲 新写 |
| [第七章　建立连接——PING/PONG 联通](#第七章建立连接pingpong-联通) | TcpServer/TcpClient 联通与日志验证 | 🔲 新写 |
| [第八章　注册与登录](#第八章注册与登录) | MySQL + AuthHandler + AuthService 闭环 | 🔲 新写 |
| [第九章　好友功能](#第九章好友功能) | 搜索/添加/列表/删除好友 | 🔲 新写 |
| [第十章　即时聊天](#第十章即时聊天) | 在线聊天与离线消息 | 🔲 新写 |
| [第十一章　文件管理](#第十一章文件管理) | 目录与文件基础管理能力 | 🔲 新写 |
| [第十二章　文件传输（上传与下载）](#第十二章文件传输上传与下载) | 分片上传下载与进度反馈 | 🔲 新写 |
| [第十三章　文件分享](#第十三章文件分享) | 好友间文件分享与接收 | 🔲 新写 |
| [附录 A　测试与调试](#附录-a测试与调试) | 单测、集成测试、调试流程 | 🔲 保留占位 |
| [附录 B　安全加固](#附录-b安全加固) | SQL 注入、密码、路径安全 | 🔲 保留占位 |
| [附录 C　CI/CD 与部署](#附录-ccicd-与部署) | CI/CD 与部署路线 | 🔲 保留占位 |
| [附录 D　常见错误汇总](#附录-d常见错误汇总) | 实战踩坑记录 | 🔲 保留占位 |
| [附录 E　Git 提交历史](#附录-egit-提交历史) | 里程碑提交索引 | 🔲 保留占位 |

## 知识补充文档（独立文件）

1. [v1 Bug 分析](./v1-bugs.md)
2. [Git 使用指南](./knowledge/git-guide.md)
3. [CMake 构建系统详解](./knowledge/cmake-guide.md)
4. [epoll 工作原理图解](./knowledge/epoll-guide.md)
5. [Qt 信号槽机制详解](./knowledge/qt-signals-slots.md)
6. [CI/CD 与 GitHub Actions 入门](./knowledge/cicd-guide.md)

## 第一章　项目背景与需求分析

### 1.1 项目来源

CloudVault v2 基于 v1 重构，目标不是“代码能跑”而是“工程可维护、可测试、可扩展”。

### 1.2 产品定位

面向小团队私有部署，整合即时通信与文件管理。

### 1.3 核心功能需求

1. 用户系统：注册、登录、登出。
2. 好友系统：搜索、添加、删除、列表。
3. 聊天系统：在线消息与离线消息。
4. 文件系统：目录操作、上传下载、分享。

### 1.4 非功能需求

1. 安全：密码哈希、SQL 注入防护、路径遍历防护。
2. 性能：支持并发连接与分片传输。
3. 可维护性：模块化、可测试、可回归。

### 1.5 v1 与 v2 对比

保留对比表用于理解重构方向，细节以功能章节为准。

> 详细的 v1 Bug 分析，见 [v1-bugs.md](./v1-bugs.md)。

## 第二章　技术选型与架构概览

### 2.1 技术选型（仅结论）

1. 服务端：C++17 + epoll + MySQL C API。
2. 客户端：Qt6 Widgets + QTcpSocket。
3. 共享层：`common` 静态库（PDU + crypto）。
4. 构建系统：CMake。

### 2.2 架构概览（高层）

1. 客户端：`ui -> service -> network`。
2. 服务端：`network -> dispatcher -> handler -> db/storage`。
3. 两端通过 PDU v2 通信。

### 2.3 本章边界说明

1. 不在本章展开 PDU 字段细节（第六章介绍）。
2. 不在本章展开密码链路和路径安全实现（第八、十一章介绍）。
3. 不在本章展开数据库表结构（按功能在第八到十三章逐步引入）。

## 第三章　开发环境搭建

### 3.1 Linux 服务端环境

1. GCC/CMake/Ninja/OpenSSL/MySQL 开发库安装。
2. 验证 `cmake --version`、`mysql --version`。

### 3.2 Windows 客户端环境

1. Qt6 + CMake + 编译器环境。
2. Qt Creator 或 VS Code CMake Tools。

### 3.3 Git 基础流程

1. 日常流程：`pull -> add -> commit -> push`。
2. 详细说明见 [knowledge/git-guide.md](./knowledge/git-guide.md)。

## 第四章　工程初始化与 CMake 配置

### 4.1 目录结构初始化

1. `common/` 共享库。
2. `Server/` 服务端。
3. `Client/` 客户端。
4. `docs/` 文档。

### 4.2 CMake 最小可用配置

1. `common/CMakeLists.txt` 构建静态库。
2. `Server/CMakeLists.txt` 链接 common 与第三方依赖。
3. `Client/CMakeLists.txt` 集成 Qt 自动化构建。

### 4.3 首次编译与检查

```bash
cmake -S Server -B build/server -DBUILD_TESTING=ON
cmake --build build/server -j
ctest --test-dir build/server --output-on-failure
```

### 4.4 本章边界说明

数据库设计将在第八到十三章随功能逐步建立，本章只要求数据库可连接，不展开 schema 细节。

## 第五章　两端骨架——让程序先跑起来

### 5.1 本章目标与验收标准

1. 服务端启动后输出 `Started on port ...`。
2. 客户端可弹出登录窗口并进入事件循环。

### 5.2 服务端骨架

1. `Config` 类：成员变量 `port_`、`root_path_`、`db_*`。
2. `config.cpp`：从 JSON 读取配置（首次引入 `nlohmann/json`）。
3. `main.cpp`：加载配置、初始化日志、启动阻塞循环。
4. 本节要求给出 `config.h/config.cpp/main.cpp` 完整代码并按变量逐项说明。

### 5.3 客户端骨架

1. `Client/CMakeLists.txt`：解释 Qt6、AUTOMOC、AUTOUIC、AUTORCC。
2. `main.cpp`：`QApplication` 初始化与事件循环。
3. `LoginWindow`：成员变量、布局、状态切换。
4. 本节要求给出 `login_window.h/.cpp` 完整代码并按成员变量逐项说明。

### 5.4 本章踩坑记录

1. `vtable for LoginWindow undefined`（MOC 配置问题）。
2. `Qt6Config.cmake not found`（Qt 环境未配置）。

### 5.5 本章新知识点

1. [CMake 构建系统详解](./knowledge/cmake-guide.md)
2. [Qt 信号槽机制详解](./knowledge/qt-signals-slots.md)

## 第六章　协议基础——PDU 与 common 层

### 6.1 本章目标

实现客户端和服务端共用协议层，并通过单元测试。

### 6.2 引入问题

1. 为什么 TCP 会出现粘包和半包。
2. 为什么需要自定义协议头。

### 6.3 PDU v2 设计

1. 16 字节头字段：`total_length/body_length/message_type/status_code/reserved`。
2. body 编码规则：字符串采用 `uint16_t len + bytes`。
3. `MessageType` 按模块分段编号。

### 6.4 `PDUHeader` 与常量定义

1. `constants.h` 中枚举与状态码设计。
2. `pdu.h` 中结构体与 `#pragma pack` 的使用理由。

### 6.5 `PDUBuilder` 实现

1. Builder 模式与链式 API。
2. `appendU8/U16/U32/U64/appendString/appendBytes` 逐个解释。

### 6.6 `PDUParser` 实现

1. `feed()` 累积字节流。
2. `hasComplete()` 判断完整包。
3. `pop()` 弹出并消费一个包。

### 6.7 `crypto_utils` 实现

1. `hashForTransport`：客户端发送前哈希。
2. `hashPassword`：服务端带盐存储。
3. `verifyPassword`：登录校验。

### 6.8 单元测试

1. `test_pdu.cpp`：粘包、半包、超大包。
2. `test_crypto.cpp`：哈希一致性与验证。

### 6.9 本章新知识点

1. 小端序与字节布局。
2. GoogleTest 基础。

## 第七章　建立连接——PING/PONG 联通

### 7.1 本章目标

客户端连接服务端，发送 `PING`，收到 `PONG`，两端日志可见。

### 7.2 服务端 TCP 层

1. `epoll` 原理、LT/ET 模式选择。
2. `TcpServer`：`server_fd_`、`epoll_fd_`、`acceptLoop()`。
3. `TcpConnection`：`recv_buf_`、`send_buf_`、`parser_`。
4. 在 `MessageDispatcher` 注册 `PING -> PONG`。

### 7.3 客户端 TCP 层

1. `TcpClient`：连接、发送、读包、发信号。
2. `ResponseRouter`：注册与路由回调。
3. `LoginWindow`：连接成功后自动发 `PING`。

### 7.4 联调测试

1. 客户端日志显示收到 `PONG`。
2. 服务端日志显示接收 `PING` 并回包。

### 7.5 本章新知识点

1. [epoll 工作原理图解](./knowledge/epoll-guide.md)

## 第八章　注册与登录

### 8.1 本章目标

客户端输入用户名/密码后，服务端落库并返回登录结果。

### 8.2 数据库层（首次引入表设计）

1. `user_info` 表：`user_id/username/password_hash/is_online/created_at`。
2. `Connection` RAII 封装。
3. `ConnectionPool`：`pool_`、`free_`、`mutex_`、`cv_`。
4. `UserRepository`：`create/findByName/setOnline/setOffline`。

### 8.3 服务端 `AuthHandler`

1. 成员变量：`db_pool_`、`session_manager_`、`file_storage_`。
2. `handleRegister()`：解析 body、校验重复、写库、建目录、回包。
3. `handleLogin()`：校验密码、维护会话、回传 `user_id`。
4. 在 `MessageDispatcher` 注册认证消息类型。

### 8.4 客户端 `AuthService`

1. `registerUser()/login()` 发包。
2. `onRegisterResp()/onLoginResp()` 解包。
3. 发信号给 `LoginWindow` 更新 UI。

### 8.5 联调测试

1. 客户端注册成功。
2. 客户端登录成功并收到 `user_id`。
3. MySQL 中 `user_info` 数据正确。

## 第九章　好友功能

### 9.1 本章目标

支持搜索用户、发送好友请求、处理请求、拉取好友列表。

### 9.2 数据库层

1. `friend` 表结构与双向关系策略。
2. `FriendRepository`：关系查询、创建请求、确认请求、列表查询。

### 9.3 服务端 `FriendHandler`

1. 处理搜索、添加、接受、拒绝、删除、列表请求。
2. 在线通知通过 `SessionManager` 定向推送。

### 9.4 客户端 `FriendService` 与 UI

1. 列表刷新。
2. 添加好友对话框。
3. 请求通知处理。

### 9.5 联调测试

两个客户端互加好友并看到一致列表。

## 第十章　即时聊天

### 10.1 本章目标

实现好友间实时消息与离线消息补投。

### 10.2 数据库层

1. `chat_message` 表。
2. `offline_message` 表。
3. `ChatRepository`：保存、查询离线、标记已投递。

### 10.3 服务端 `ChatHandler`

1. 在线转发。
2. 离线落库。
3. 登录后自动补投或主动拉取。

### 10.4 客户端 `ChatService` 与聊天界面

1. 发送与接收。
2. 聊天气泡显示。
3. 回车发送与失败提示。

### 10.5 联调测试

1. 两端在线实时聊天。
2. 一端离线时消息补投成功。

## 第十一章　文件管理

### 11.1 本章目标

实现目录浏览、新建、重命名、移动、删除、搜索。

### 11.2 `FileStorage` 实现

1. `safePath()` 路径安全校验。
2. `listDir/mkdir/remove/rename/move/search`。

### 11.3 服务端 `FileHandler`

处理 `FILE_LIST/MKDIR/DELETE/RENAME/MOVE/SEARCH`。

### 11.4 客户端 `FileService` 与 UI

1. 文件树展示。
2. 右键操作菜单。

### 11.5 联调测试

验证路径安全与多操作一致性。

## 第十二章　文件传输（上传与下载）

### 12.1 本章目标

实现分片上传下载与进度反馈。

### 12.2 协议流程

1. 上传：`UPLOAD_INIT -> UPLOAD_CHUNK x N -> UPLOAD_FINISH`。
2. 下载：`DOWNLOAD_REQ -> DOWNLOAD_CHUNK x N -> DOWNLOAD_FINISH`。

### 12.3 服务端实现

1. `UploadContext` 管理传输上下文。
2. 分片写入与完整性校验。

### 12.4 客户端实现

1. 分片读取本地文件。
2. 进度条更新。
3. 失败重试与恢复。

### 12.5 联调测试

1. 小文件正确上传下载。
2. 大文件分片稳定完成。

## 第十三章　文件分享

### 13.1 本章目标

用户可将文件分享给好友，好友接受后复制到自己的目录。

### 13.2 服务端实现

1. `SHARE_FILE_REQ`：校验文件并通知目标用户。
2. `SHARE_FILE_ACCEPT_REQ`：执行复制并回包。

### 13.3 客户端实现

1. 选择好友并发送分享。
2. 接收分享通知并执行接受/拒绝。

### 13.4 联调测试

双端验证分享与接收流程。

## 附录 A　测试与调试

后续补充：单元测试、集成测试、端到端冒烟测试、GDB 调试流程。

## 附录 B　安全加固

后续补充：SQL 注入对抗、密码链路、防路径遍历、输入校验。

## 附录 C　CI/CD 与部署

1. [CI/CD 与 GitHub Actions 入门](./knowledge/cicd-guide.md)
2. 本项目 workflow、镜像构建与部署流程后续补充。

## 附录 D　常见错误汇总

后续补充：按章节记录真实问题、原因、修复方式。

## 附录 E　Git 提交历史

后续补充：按里程碑维护提交索引。
