// =============================================================
// server/include/server/server_app.h
// 服务端应用程序主类
//
// 负责：
//   1. 加载并校验 JSON 配置文件
//   2. 初始化 spdlog 日志系统（控制台 + 轮转文件）
//   3. （第七章）初始化 EventLoop、TcpServer、ThreadPool、MessageDispatcher
//   4. TODO（第八章）：初始化 Database 连接池、SessionManager
//   5. 启动事件循环，阻塞直到收到 SIGINT / SIGTERM
//   6. 有序关闭所有资源（graceful shutdown）
// =============================================================

#pragma once

#include "server/event_loop.h"
#include "server/tcp_server.h"
#include "server/thread_pool.h"
#include "server/message_dispatcher.h"
#include "server/session_manager.h"
#include "server/db/database.h"
#include "server/file_storage.h"
#include "server/handler/auth_handler.h"
#include "server/handler/chat_handler.h"
#include "server/handler/friend_handler.h"
#include "server/handler/group_handler.h"
#include "server/handler/file_handler.h"
#include "server/handler/share_handler.h"

#include <memory>
#include <string>

namespace cloudvault { class TcpConnection; }

/**
 * @brief 服务端应用总控类。
 *
 * 负责配置加载、日志初始化、网络与数据模块装配、
 * 消息路由注册、事件循环启动与优雅关闭。
 */
class ServerApp {
public:
    /**
     * @brief 构造 ServerApp。
     */
    ServerApp();
    /**
     * @brief 析构 ServerApp。
     */
    ~ServerApp();

    /**
     * @brief 初始化服务端组件。
     * @param config_path 配置文件路径。
     * @return true 表示初始化成功。
     */
    bool init(const std::string& config_path);
    /**
     * @brief 启动服务主循环。
     */
    void run();

private:
    /**
     * @brief 执行有序关闭流程。
     */
    void shutdown();
    /**
     * @brief 注册协议消息路由。
     */
    void registerHandlers();

    /// @brief 初始化完成标记。
    bool initialized_ = false;

    /// @brief 事件循环（IO 线程）。
    std::unique_ptr<cloudvault::EventLoop>     event_loop_;
    /// @brief TCP 监听服务器。
    std::unique_ptr<cloudvault::TcpServer>     tcp_server_;
    /// @brief 工作线程池。
    std::unique_ptr<cloudvault::ThreadPool>    thread_pool_;
    /// @brief 消息分发器。
    cloudvault::MessageDispatcher              dispatcher_;

    /// @brief MySQL 数据库连接池。
    std::unique_ptr<cloudvault::Database>      db_;
    /// @brief 文件系统存储抽象。
    std::unique_ptr<cloudvault::FileStorage>   file_storage_;
    /// @brief 在线会话管理器。
    cloudvault::SessionManager                 sessions_;
    /// @brief 认证处理器。
    std::unique_ptr<cloudvault::AuthHandler>   auth_handler_;
    /// @brief 聊天处理器。
    std::unique_ptr<cloudvault::ChatHandler>   chat_handler_;
    /// @brief 好友处理器。
    std::unique_ptr<cloudvault::FriendHandler> friend_handler_;
    /// @brief 群组处理器。
    std::unique_ptr<cloudvault::GroupHandler>  group_handler_;
    /// @brief 文件处理器。
    std::unique_ptr<cloudvault::FileHandler>   file_handler_;
    /// @brief 分享处理器。
    std::unique_ptr<cloudvault::ShareHandler>  share_handler_;
};
