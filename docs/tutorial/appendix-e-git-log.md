# 附录 E　Git 提交历史

> **状态**：🔲 持续更新（每完成一个里程碑后在这里记录）

---

## 里程碑索引

每完成一章核心功能，打一个 Git tag，方便读者 checkout 到对应阶段查看代码。

| Tag | 对应章节 | 说明 |
|-----|---------|------|
| `chapter-04-cmake` | 第四章 | CMake 配置完成，项目可编译 |
| `chapter-05-skeleton` | 第五章 | 两端骨架可独立启动 |
| `chapter-06-protocol` | 第六章 | PDU 编解码完成，单测通过 |
| `chapter-07-ping-pong` | 第七章 | 客户端与服务端 TCP 联通 |
| `chapter-08-auth` | 第八章 | 注册登录闭环完成 |
| `chapter-09-friend` | 第九章 | 好友功能完成 |
| `chapter-10-chat` | 第十章 | 即时聊天完成 |
| `chapter-11-file-manager` | 第十一章 | 文件管理功能完成 |
| `chapter-12-file-transfer` | 第十二章 | 文件上传下载完成 |
| `chapter-13-file-share` | 第十三章 | 文件分享功能完成 |

---

## 如何打 Tag

每章完成后运行：

```bash
git tag chapter-XX-<name>
git push origin chapter-XX-<name>
```

## 如何 Checkout 到某个里程碑

```bash
# 查看某个 tag 的代码（只读，不修改）
git checkout chapter-05-skeleton

# 回到最新代码
git checkout master
```
