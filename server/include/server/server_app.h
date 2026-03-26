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
#include "server/handler/friend_handler.h"
#include "server/handler/file_handler.h"
#include "server/handler/share_handler.h"

#include <memory>
#include <string>

namespace cloudvault { class TcpConnection; }

class ServerApp {
public:
    ServerApp();
    ~ServerApp();

    bool init(const std::string& config_path);
    void run();

private:
    void shutdown();
    void registerHandlers();

    bool initialized_ = false;

    // 第七章：网络层
    std::unique_ptr<cloudvault::EventLoop>     event_loop_;
    std::unique_ptr<cloudvault::TcpServer>     tcp_server_;
    std::unique_ptr<cloudvault::ThreadPool>    thread_pool_;
    cloudvault::MessageDispatcher              dispatcher_;

    // 第八章：数据层 + 会话
    std::unique_ptr<cloudvault::Database>      db_;
    std::unique_ptr<cloudvault::FileStorage>   file_storage_;
    cloudvault::SessionManager                 sessions_;
    std::unique_ptr<cloudvault::AuthHandler>   auth_handler_;
    std::unique_ptr<cloudvault::FriendHandler> friend_handler_;
    std::unique_ptr<cloudvault::FileHandler>   file_handler_;
    std::unique_ptr<cloudvault::ShareHandler>  share_handler_;
};
