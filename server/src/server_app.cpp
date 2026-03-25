// =============================================================
// server/src/server_app.cpp
// ServerApp 实现
// =============================================================

#include "server/server_app.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <thread>

using json = nlohmann::json;

// ── 全局 shutdown 标志（信号处理函数设置）────────────────────────
// 使用 atomic 保证多线程可见性；信号处理函数只做最简单的操作。
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
// init()：加载配置、初始化日志、打印启动信息
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
    // 使用 json_pointer 安全读取嵌套键值，键不存在时返回默认值
    using jp = json::json_pointer;

    const std::string log_level = cfg.value(jp("/log/level"), std::string("info"));
    const std::string log_file  = cfg.value(jp("/log/file"),  std::string("logs/server.log"));

    try {
        // 确保日志目录存在
        std::filesystem::create_directories(
            std::filesystem::path(log_file).parent_path());

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink    = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file,
            10 * 1024 * 1024,   // 单文件上限 10 MB
            3                   // 最多保留 3 个轮转文件
        );

        auto logger = std::make_shared<spdlog::logger>(
            "server",
            spdlog::sinks_init_list{console_sink, file_sink}
        );
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
    const int         thread_count = cfg.value(jp("/server/thread_count"), 8);
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

    // ── 5. 各模块初始化（占位，后续章节逐步填充）─────────────
    // TODO（第七章）：EventLoop、TcpServer、ThreadPool
    // TODO（第八章）：Database 连接池、SessionManager
    spdlog::warn("骨架阶段：网络层与数据库层尚未初始化");

    // ── 6. 注册信号处理 ───────────────────────────────────────
    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    initialized_ = true;
    spdlog::info("初始化完成，按 Ctrl+C 关闭服务");
    return true;
}

// =============================================================
// run()：主循环
// TODO（第七章）：替换为 event_loop_.run()
// =============================================================
void ServerApp::run() {
    if (!initialized_) {
        spdlog::error("run() 在 init() 成功之前被调用");
        return;
    }

    // 骨架阶段：轮询 shutdown 标志，等待信号
    while (!g_shutdown.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    shutdown();
}

// =============================================================
// shutdown()：有序关闭
// =============================================================
void ServerApp::shutdown() {
    if (!initialized_) return;
    initialized_ = false;

    spdlog::info("正在关闭服务...");

    // TODO（第七章）：event_loop_.stop()、tcp_server_.stop()
    // TODO（第八章）：db_pool_.close()

    spdlog::info("CloudVault Server 已关闭，Bye.");
    spdlog::shutdown();
}
