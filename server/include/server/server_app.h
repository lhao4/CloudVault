// =============================================================
// server/include/server/server_app.h
// 服务端应用程序主类
//
// 负责：
//   1. 加载并校验 JSON 配置文件
//   2. 初始化 spdlog 日志系统（控制台 + 轮转文件）
//   3. TODO（第七章）：初始化 EventLoop、TcpServer、ThreadPool
//   4. TODO（第八章）：初始化 Database 连接池、SessionManager
//   5. 启动事件循环，阻塞直到收到 SIGINT / SIGTERM
//   6. 有序关闭所有资源（graceful shutdown）
// =============================================================

#pragma once

#include <string>

class ServerApp {
public:
    ServerApp();
    ~ServerApp();

    // 初始化所有模块；返回 false 表示初始化失败（如配置缺失、端口占用）
    bool init(const std::string& config_path);

    // 启动服务，阻塞直到收到 SIGINT / SIGTERM
    void run();

private:
    void shutdown();

    bool initialized_ = false;
};
