// =============================================================
// server/src/main.cpp
// 服务端程序入口
//
// 用法：
//   ./cloudvault_server [配置文件路径]
//   ./cloudvault_server config/server.json
//
// 若不传参数，默认读取 config/server.json
// =============================================================

#include "server/server_app.h"

#include <iostream>

int main(int argc, char* argv[]) {
    const std::string config_path = (argc > 1) ? argv[1] : "config/server.json";

    ServerApp app;

    if (!app.init(config_path)) {
        std::cerr << "[main] 初始化失败，退出。\n";
        return 1;
    }

    app.run();  // 阻塞直到收到 SIGINT / SIGTERM
    return 0;
}
