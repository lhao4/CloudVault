# CloudVault 云巢

> 面向小型团队的私有化云文件管理与即时通讯系统

---

## 这是什么

CloudVault 是一个 C/S 架构桌面应用，服务端运行在 Linux，客户端运行在 Windows（Qt）。你可以把它部署在自己的服务器上，实现：

- **文件管理**：上传、下载、目录管理、大文件分片传输
- **即时聊天**：1 对 1 聊天、离线消息、群聊
- **好友系统**：搜索用户、添加好友、文件分享

---

## 文档导航

### 项目实战教程

从零开始，逐步构建完整项目。适合刚学完 C++ 的初学者跟着做。

[开始教程 →](tutorial/index.md)

### 工程参考文档

描述系统"是什么样的"，开发过程中查阅用。

| 文档 | 内容 |
|------|------|
| [PRD 产品需求](reference/prd.md) | 功能需求、非功能需求、版本范围 |
| [系统架构](reference/architecture.md) | 整体架构、模块划分、数据流 |
| [API 协议](reference/api.md) | PDU 格式、消息类型、字段定义 |
| [数据库设计](reference/database.md) | 表结构、字段说明、索引设计 |
| [模块划分](reference/modules.md) | 服务端/客户端模块详细说明 |
| [UI 流程](reference/ui-flow.md) | 界面跳转与交互流程 |
| [测试方案](reference/testing.md) | 单元测试、集成测试、测试规范 |

---

## 技术栈

| 层 | 服务端 | 客户端 |
|----|--------|--------|
| 语言 | C++17 | C++17 + Qt 6 |
| 网络 | POSIX epoll | QTcpSocket |
| 数据库 | MySQL 8.0 | — |
| 构建 | CMake 3.20+ | CMake 3.20+ |
| 日志 | spdlog | qDebug |
| 部署 | 裸机 / Docker | Windows 安装包 |
