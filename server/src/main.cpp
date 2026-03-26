// =============================================================
// server/src/main.cpp
// 服务端程序入口
//
// 用法：
//   ./cloudvault_server [配置文件路径]
//   ./cloudvault_server server/config/server.json
//
// 若不传参数，按约定优先读取 server/config/server.json
// =============================================================

#include "server/server_app.h"

#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    std::string config_path;
    if (argc > 1) {
        config_path = argv[1];
    } else {
        const std::filesystem::path exe_dir = std::filesystem::path(argv[0]).parent_path();
        const std::filesystem::path candidates[] = {
            "server/config/server.json",
            "config/server.json",
            (exe_dir / "../config/server.json").lexically_normal(),
        };
        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(candidate)) {
                config_path = candidate.string();
                break;
            }
        }
        if (config_path.empty()) {
            config_path = "server/config/server.json";
        }
    }

    ServerApp app;

    if (!app.init(config_path)) {
        std::cerr << "[main] 初始化失败，退出。\n";
        return 1;
    }

    app.run();  // 阻塞直到收到 SIGINT / SIGTERM
    return 0;
}
