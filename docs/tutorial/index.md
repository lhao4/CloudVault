# 项目实战开发手册

> **面向读者**：刚学完 C++、第一次做完整 C/S 项目的初学者。
>
> **教学目标**：客户端与服务端同步推进，不先搭完整框架，每步都可运行与测试。

---

## 教程进度

| 章节 | 内容概要 | 状态 |
|------|----------|------|
| [第一章　项目背景与需求分析](ch01-background.md) | 项目来源、定位、需求、v2 目标 | ✅ 已完成 |
| [第二章　技术选型与架构概览](ch02-tech-stack.md) | 技术栈决策、高层架构、模块边界 | ✅ 已完成 |
| [第三章　开发环境搭建](ch03-environment.md) | Linux/Windows 环境、工具安装与验证 | ✅ 已完成 |
| [第四章　工程初始化与 CMake 配置](ch04-cmake.md) | 项目骨架、构建系统、首次编译 | ✅ 已完成 |
| [第五章　两端骨架——让程序先跑起来](ch05-skeleton.md) | 服务端最小启动 + 客户端登录窗可运行 | ✅ 已完成 |
| [第六章　协议基础——PDU 与 common 层](ch06-protocol.md) | PDU、粘包拆包、crypto 与单测 | ✅ 已完成 |
| [第七章　建立连接——PING/PONG 联通](ch07-ping-pong.md) | TcpServer/TcpClient 联通与日志验证 | ✅ 已完成 |
| [第八章　注册与登录](ch08-auth.md) | MySQL + AuthHandler + AuthService 闭环 | ✅ 已完成 |
| [第九章　好友功能](ch09-friend.md) | 好友协议 + 三栏主界面 + 搜索/添加/列表/删除好友 | ✅ 已完成 |
| [第十章　即时聊天](ch10-chat.md) | 在线聊天与离线消息 | ✅ 待写 |
| [第十一章　文件管理](ch11-file-manager.md) | 目录浏览 + 新建/重命名/移动/删除/搜索 | ✅ 已完成 |
| [第十二章　文件传输（上传与下载）](ch12-file-transfer.md) | 分片上传下载与进度反馈 | 🔲 待写 |
| [第十三章　文件分享](ch13-file-share.md) | 好友间文件分享通知与接收复制 | ✅ 已完成 |

---

## 附录

| 附录 | 内容 | 状态 |
|------|------|------|
| [附录 A　测试与调试](appendix-a-testing.md) | 单测、集成测试、GDB 调试流程 | 🔲 待写 |
| [附录 B　安全加固](appendix-b-security.md) | SQL 注入对抗、密码链路、路径防护 | 🔲 待写 |
| [附录 C　CI/CD 与部署](appendix-c-cicd.md) | GitHub Actions、Docker 部署 | 🔲 待写 |
| [附录 D　常见错误汇总](appendix-d-errors.md) | 按章节记录实战踩坑与解决方式 | 🔲 持续更新 |
| [附录 E　Git 提交历史](appendix-e-git-log.md) | 里程碑提交索引 | 🔲 持续更新 |

---

## 知识补充文档

遇到不熟悉的工具或概念时查阅，不需要提前读完。

1. [Git 使用指南](knowledge/git-guide.md)
2. [CMake 构建系统详解](knowledge/cmake-guide.md)
3. [epoll 工作原理图解](knowledge/epoll-guide.md)
4. [Qt 信号槽机制详解](knowledge/qt-signals-slots.md)
5. [CI/CD 与 GitHub Actions 入门](knowledge/cicd-guide.md)
6. [MkDocs 文档站搭建](knowledge/mkdocs-github-pages.md)
