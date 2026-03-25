// =============================================================
// server/src/server_app.cpp
// ServerApp 实现（第七章：接入网络层）
// =============================================================

#include "server/server_app.h"
#include "server/tcp_connection.h"

#include "common/protocol.h"
#include "common/protocol_codec.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

using json = nlohmann::json;

// ── 全局 shutdown 标志（信号处理函数设置）────────────────────────
static std::atomic<bool> g_shutdown{false};

static void onSignal(int sig) {
    spdlog::info("收到信号 {}，准备关闭...", sig);
    g_shutdown.store(true);
}

// =============================================================
// 构造 / 析构
// =============================================================
ServerApp::ServerApp() = default;

ServerApp::~ServerApp() {
    shutdown();
}

// =============================================================
// init()：加载配置、初始化日志、启动网络层
// =============================================================
bool ServerApp::init(const std::string& config_path) {
    // ── 1. 读取配置文件 ───────────────────────────────────────
    std::ifstream cfg_file(config_path);
    if (!cfg_file.is_open()) {
        std::cerr << "[ServerApp] 无法打开配置文件：" << config_path << "\n";
        return false;
    }

    json cfg;
    try {
        cfg_file >> cfg;
    } catch (const json::exception& e) {
        std::cerr << "[ServerApp] 配置文件解析失败：" << e.what() << "\n";
        return false;
    }

    // ── 2. 初始化日志 ─────────────────────────────────────────
    using jp = json::json_pointer;

    const std::string log_level = cfg.value(jp("/log/level"), std::string("info"));
    const std::string log_file  = cfg.value(jp("/log/file"),  std::string("logs/server.log"));

    try {
        std::filesystem::create_directories(
            std::filesystem::path(log_file).parent_path());

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink    = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, 10 * 1024 * 1024, 3);

        auto logger = std::make_shared<spdlog::logger>(
            "server",
            spdlog::sinks_init_list{console_sink, file_sink});
        logger->set_level(spdlog::level::from_str(log_level));
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-5l%$] [tid %t] %v");
        spdlog::set_default_logger(logger);

    } catch (const spdlog::spdlog_ex& e) {
        std::cerr << "[ServerApp] 日志初始化失败：" << e.what() << "\n";
        return false;
    }

    // ── 3. 读取服务端参数 ─────────────────────────────────────
    const std::string host         = cfg.value(jp("/server/host"),         std::string("0.0.0.0"));
    const int         port         = cfg.value(jp("/server/port"),         5000);
    const int         thread_count = cfg.value(jp("/server/thread_count"), 4);
    const std::string storage_root = cfg.value(jp("/storage/root"),        std::string("./filesys"));
    const std::string db_host      = cfg.value(jp("/database/host"),       std::string("127.0.0.1"));
    const int         db_port      = cfg.value(jp("/database/port"),       3306);
    const std::string db_name      = cfg.value(jp("/database/name"),       std::string("cloudvault"));

    // ── 4. 打印启动横幅 ───────────────────────────────────────
    spdlog::info("╔══════════════════════════════════════╗");
    spdlog::info("║      CloudVault Server  v2.0         ║");
    spdlog::info("╚══════════════════════════════════════╝");
    spdlog::info("监听地址  : {}:{}", host, port);
    spdlog::info("工作线程  : {}", thread_count);
    spdlog::info("文件存储  : {}", storage_root);
    spdlog::info("数据库    : {}:{}/{}", db_host, db_port, db_name);
    spdlog::info("日志级别  : {}", log_level);

    // ── 5. 初始化网络层（第七章）─────────────────────────────
    try {
        event_loop_  = std::make_unique<cloudvault::EventLoop>();
        thread_pool_ = std::make_unique<cloudvault::ThreadPool>(
            static_cast<size_t>(thread_count));
        tcp_server_  = std::make_unique<cloudvault::TcpServer>(
            event_loop_.get(), host, port);
    } catch (const std::exception& e) {
        spdlog::error("网络层初始化失败：{}", e.what());
        return false;
    }

    // ── 6. 注册消息处理器 ────────────────────────────────────
    registerHandlers();

    // ── 7. 配置新连接回调 ────────────────────────────────────
    tcp_server_->setNewConnectionCallback(
        [this](std::shared_ptr<cloudvault::TcpConnection> conn) {
            spdlog::info("Client connected: {}", conn->peerAddr());

            // 设置消息回调：收到 PDU 时交给线程池处理
            conn->setMessageCallback(
                [this](std::shared_ptr<cloudvault::TcpConnection> c,
                       const cloudvault::PDUHeader& hdr,
                       std::vector<uint8_t> body) {
                    thread_pool_->enqueue([this, c, hdr,
                                           body = std::move(body)]() mutable {
                        dispatcher_.dispatch(c, hdr, body);
                    });
                });
        });

    // ── 8. 注册信号处理 ───────────────────────────────────────
    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    initialized_ = true;
    spdlog::info("初始化完成，按 Ctrl+C 关闭服务");
    return true;
}

// =============================================================
// registerHandlers()：注册所有消息类型的处理函数
// =============================================================
void ServerApp::registerHandlers() {
    // PING → 回复 PONG（保活心跳）
    dispatcher_.registerHandler(
        cloudvault::MessageType::PING,
        [](std::shared_ptr<cloudvault::TcpConnection> conn,
           const cloudvault::PDUHeader& /*hdr*/,
           const std::vector<uint8_t>& /*body*/) {
            spdlog::debug("PING from {}", conn->peerAddr());
            auto pong = cloudvault::PDUBuilder(cloudvault::MessageType::PONG).build();
            conn->send(std::move(pong));
        });

    // TODO（第八章）：LOGIN_REQUEST、REGISTER_REQUEST
    // TODO（第九章）：FIND_USER、ADD_FRIEND ...
}

// =============================================================
// run()：启动 TcpServer + 运行 EventLoop（阻塞）
// =============================================================
void ServerApp::run() {
    if (!initialized_) {
        spdlog::error("run() 在 init() 成功之前被调用");
        return;
    }

    tcp_server_->start();
    spdlog::info("服务已启动，等待连接...");

    // 在独立线程中轮询 shutdown 标志
    // EventLoop::run() 每 100ms 超时一次，可以检测 stop()
    std::thread watcher([this] {
        while (!g_shutdown.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        event_loop_->stop();
    });

    event_loop_->run();  // 阻塞，直到 stop() 被调用

    watcher.join();
    shutdown();
}

// =============================================================
// shutdown()：有序关闭
// =============================================================
void ServerApp::shutdown() {
    if (!initialized_) return;
    initialized_ = false;

    spdlog::info("正在关闭服务...");

    // event_loop_ 已在 run() 中 stop，资源由 unique_ptr 析构释放
    thread_pool_.reset();   // 等待所有工作线程完成当前任务
    tcp_server_.reset();
    event_loop_.reset();

    // TODO（第八章）：db_pool_.close()

    spdlog::info("CloudVault Server 已关闭，Bye.");
    spdlog::shutdown();
}
