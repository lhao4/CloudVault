# ===========================================================
# server/CMakeLists.txt
# 构建服务端可执行文件 cloudhive_server
#
# 依赖关系：
#   cloudhive_server
#     ├── cloudhive_common（共享协议库，本地）
#     ├── spdlog          （日志库，FetchContent 自动下载）
#     ├── nlohmann/json   （JSON 配置解析，FetchContent 自动下载）
#     ├── OpenSSL::SSL    （TLS 加密）
#     ├── OpenSSL::Crypto （SHA-256，由 common 传递，此处仅作显式说明）
#     ├── MySQL::MySQL    （数据库客户端，系统安装）
#     └── Threads::Threads（std::thread 支持）
# ===========================================================

cmake_minimum_required(VERSION 3.20)
project(cloudhive_server VERSION 2.0 LANGUAGES CXX)

# ── C++ 标准 ─────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── FetchContent：自动下载第三方库 ───────────────────────
# FetchContent 是 CMake 3.14+ 内置的依赖管理方案。
# 原理：在 cmake 配置阶段（非编译阶段）从 Git 下载源码，
#       然后像本地子目录一样参与构建。
# 优点：不需要用户手动安装这些库，clone 项目后直接 cmake 即可。
# 缺点：首次配置需要联网，速度取决于网络。
include(FetchContent)

# ── spdlog（日志库）──────────────────────────────────────
# GIT_TAG 指定版本，保证所有人用同一个版本，避免版本差异导致的问题。
# 建议：每隔一段时间更新 GIT_TAG 到最新稳定版。
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
)

# ── nlohmann/json（JSON 解析库）──────────────────────────
# 用于读取 server.json 配置文件。
# header-only 库，所以编译极快。
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

# FetchContent_MakeAvailable()：让上面声明的所有库可用。
# 可以批量写在一起，CMake 会并行处理。
FetchContent_MakeAvailable(spdlog nlohmann_json)

# ── 系统库查找 ────────────────────────────────────────────
# OpenSSL：TLS 加密 + SHA-256
# 安装：sudo apt install libssl-dev
find_package(OpenSSL REQUIRED)

# Threads：std::thread、std::mutex 等需要链接 pthread（Linux）
# CMake 提供 Threads::Threads 作为跨平台的线程库 target。
find_package(Threads REQUIRED)

# MySQL：自定义查找模块（见 cmake/FindMySQL.cmake）
# 安装：sudo apt install libmysqlclient-dev
# CMAKE_MODULE_PATH：告诉 CMake 去哪里找自定义 Find 模块。
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
find_package(MySQL REQUIRED)

# ── 引入 common 静态库 ────────────────────────────────────
# add_subdirectory()：将另一个目录的 CMakeLists.txt 纳入当前构建系统。
# 第一个参数：common 目录的路径（相对于当前 CMakeLists.txt）。
# 第二个参数：binary_dir，指定 common 的编译产物放在哪里，
#   ${CMAKE_BINARY_DIR} 是当前 project 的构建目录（即 server/build/），
#   这样 common 的 .a 文件会放在 server/build/common/ 下，结构清晰。
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_BINARY_DIR}/common
)

# ── 收集源文件 ────────────────────────────────────────────
# GLOB_RECURSE：递归匹配指定目录下所有符合模式的文件。
# 注意：CMake 官方不推荐 GLOB（因为新增文件后不会自动重新运行 cmake），
#   但对于初学阶段，每次新增文件后重新运行一次 cmake 即可：
#     cmake -B build（重新配置）
#     cmake --build build（编译）
# 等项目稳定后可以改为逐行列出文件（更规范）。
file(GLOB_RECURSE SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# ── 定义可执行文件 target ─────────────────────────────────
add_executable(cloudhive_server ${SERVER_SOURCES})

# ── 头文件路径 ────────────────────────────────────────────
# PRIVATE：只有 cloudhive_server 自己编译时才需要这些路径，
#   不会传递给其他链接了 server 的 target（server 是最终产物，不会被别人链接）。
target_include_directories(cloudhive_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ── 链接依赖 ─────────────────────────────────────────────
# 所有依赖写在一起，CMake 会自动处理链接顺序。
# cloudhive_common 链接后，OpenSSL::Crypto 也会自动传递过来（因为 common 声明为 PUBLIC）。
target_link_libraries(cloudhive_server
    PRIVATE
        cloudhive_common        # 共享协议库
        spdlog::spdlog          # 日志
        nlohmann_json::nlohmann_json  # JSON 解析
        OpenSSL::SSL            # TLS
        OpenSSL::Crypto         # SHA-256（common 已 PUBLIC 传递，此处显式写出便于阅读）
        MySQL::MySQL            # 数据库客户端
        Threads::Threads        # std::thread / pthread
)

# ── 编译选项 ─────────────────────────────────────────────
# 为 server target 单独开启严格警告。
# -Wall：开启大多数警告
# -Wextra：开启额外警告
# -Wpedantic：要求严格符合 C++ 标准
# 这些在开发阶段很有价值，能提前发现潜在问题。
target_compile_options(cloudhive_server
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# ── 单元测试（可选）──────────────────────────────────────
# BUILD_TESTING 是 CMake 标准变量，默认为 ON。
# 构建时可以通过 -DBUILD_TESTING=OFF 关闭测试，加快编译速度。
# 用法：cmake -B build -DBUILD_TESTING=ON
#        cmake --build build
#        ctest --test-dir build --output-on-failure
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
