# ===========================================================
# server/tests/CMakeLists.txt
# 构建服务端单元测试可执行文件 cloudhive_server_tests
#
# 测试框架：GoogleTest（GTest）
# 运行方式：ctest --test-dir build --output-on-failure
# ===========================================================

# ── 下载 GoogleTest ──────────────────────────────────────
# GoogleTest 需要通过 FetchContent 引入。
# 注意：这里的 FetchContent_Declare 不需要重复调用，
#   但因为 tests/ 是独立的子目录，需要再次 include FetchContent。
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# gtest_force_shared_crt：Windows 上需要与 MSVC 运行时库保持一致，
# 设为 ON 可以避免链接错误。
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── 收集测试源文件 ────────────────────────────────────────
file(GLOB_RECURSE TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# ── 定义测试可执行文件 ────────────────────────────────────
add_executable(cloudhive_server_tests ${TEST_SOURCES})

# ── 链接测试框架和被测库 ──────────────────────────────────
# GTest::gtest_main：包含 main() 函数，这样我们的测试文件不需要写 main()。
# GTest::gtest：核心框架（EXPECT_*、ASSERT_* 等宏）。
# cloudhive_common：被测代码（protocol、crypto）。
target_link_libraries(cloudhive_server_tests
    PRIVATE
        GTest::gtest_main
        GTest::gtest
        cloudhive_common
)

# ── 头文件路径 ────────────────────────────────────────────
target_include_directories(cloudhive_server_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# ── 注册到 CTest ─────────────────────────────────────────
# include(GoogleTest) + gtest_discover_tests()：
#   自动发现测试文件中所有 TEST() / TEST_F() 并注册到 CTest。
#   这样 ctest 可以逐个运行每个测试用例，并单独报告通过/失败。
include(GoogleTest)
gtest_discover_tests(cloudhive_server_tests)
