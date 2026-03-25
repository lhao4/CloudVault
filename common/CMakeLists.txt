# ===========================================================
# common/CMakeLists.txt
# 构建共享协议静态库 cloudvault_common
#
# 什么是静态库？
#   静态库（.a / .lib）是一堆 .cpp 编译后的 .o 文件打包在一起的集合。
#   server 和 client 都链接这个库，这样协议代码只需要写一份。
# ===========================================================

# cmake_minimum_required：声明本项目要求的最低 CMake 版本。
# 低于这个版本的 CMake 会直接报错，避免使用了新语法却在旧环境编译。
cmake_minimum_required(VERSION 3.20)

# project()：定义项目名称、版本号、使用的编程语言。
# LANGUAGES CXX 表示只用 C++（不用 C、Fortran 等）。
project(cloudvault_common VERSION 2.0 LANGUAGES CXX)

# ── C++ 标准设置 ──────────────────────────────────────────
# CMAKE_CXX_STANDARD：指定使用 C++17。
# CMAKE_CXX_STANDARD_REQUIRED ON：如果编译器不支持 C++17 则报错，而非静默降级。
# CMAKE_CXX_EXTENSIONS OFF：禁用编译器专属扩展（如 GCC 的 gnu++17），
#   保证代码在不同编译器下行为一致。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── 查找系统依赖 ─────────────────────────────────────────
# find_package()：在系统中查找已安装的第三方库。
# REQUIRED：找不到则立即报错（而非忽略）。
# OpenSSL 用于 SHA-256 哈希（密码安全）。
# 安装方式（Linux）：sudo apt install libssl-dev
# 安装方式（Windows）：通过 Qt 安装器附带，或 winget install OpenSSL
find_package(OpenSSL REQUIRED)

# ── 定义静态库 target ─────────────────────────────────────
# add_library(<名称> STATIC <源文件列表>)
# STATIC：编译为静态库（.a）。
# 这里列出所有需要编译的 .cpp 文件。
# 注意：头文件（.h）不需要列在这里，只需要 .cpp。
add_library(cloudvault_common STATIC
    src/protocol_codec.cpp
    src/crypto_utils.cpp
)

# ── 头文件搜索路径 ────────────────────────────────────────
# target_include_directories()：告诉编译器去哪里找 #include 的头文件。
#
# PUBLIC 的含义：
#   - 编译 cloudvault_common 本身时，会把 include/ 加入搜索路径。
#   - 链接了 cloudvault_common 的其他 target（server、client）
#     也会自动继承这个路径。
#   这样 server/client 中写 #include "common/protocol.h" 就能找到文件。
#
# ${CMAKE_CURRENT_SOURCE_DIR}：当前 CMakeLists.txt 所在目录的绝对路径，
#   即 CloudVault/common/。
target_include_directories(cloudvault_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ── 链接依赖库 ────────────────────────────────────────────
# target_link_libraries()：声明本 target 依赖哪些库。
#
# PUBLIC 的含义：
#   OpenSSL::Crypto 不仅编译 common 时需要，
#   链接了 common 的 server 也会自动链接 OpenSSL::Crypto。
#   这样 server 的 CMakeLists.txt 就不需要再重复写一遍 OpenSSL。
#
# OpenSSL::Crypto：find_package(OpenSSL) 成功后，
#   CMake 会自动创建 OpenSSL::SSL 和 OpenSSL::Crypto 两个导入 target。
#   我们只用 Crypto（包含 SHA-256），不需要 SSL（TLS 握手）。
target_link_libraries(cloudvault_common
    PUBLIC
        OpenSSL::Crypto
)
