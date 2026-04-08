# ===========================================================
# server/tests/CMakeLists.txt
# 构建服务端单元测试可执行文件 cloudvault_server_tests
#
# 测试框架：GoogleTest（GTest）
# 安装：sudo apt install libgtest-dev
# 运行方式：ctest --test-dir build --output-on-failure
# ===========================================================

# ── 查找 GoogleTest（系统包）─────────────────────────────
find_package(GTest REQUIRED)

# ── 收集测试源文件 ────────────────────────────────────────
file(GLOB_RECURSE TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# 骨架阶段：暂无测试文件，跳过目标创建
# 第六章实现 PDU 编解码和加密工具后会在此添加测试
if(NOT TEST_SOURCES)
    message(STATUS "cloudvault_server_tests: 暂无测试文件，跳过构建")
    return()
endif()

# ── 定义测试可执行文件 ────────────────────────────────────
set(SERVER_TEST_SUPPORT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/event_loop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/file_storage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/handler/file_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/message_dispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/session_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tcp_connection.cpp
)

add_executable(cloudvault_server_tests
    ${TEST_SOURCES}
    ${SERVER_TEST_SUPPORT_SOURCES}
)

# ── 链接测试框架和被测库 ──────────────────────────────────
target_link_libraries(cloudvault_server_tests
    PRIVATE
        GTest::gtest_main
        GTest::gtest
        cloudvault_common
        spdlog::spdlog
        Threads::Threads
)

# ── 头文件路径 ────────────────────────────────────────────
target_include_directories(cloudvault_server_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# ── 注册到 CTest ─────────────────────────────────────────
include(GoogleTest)
gtest_discover_tests(cloudvault_server_tests)
