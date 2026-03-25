# ===========================================================
# common/tests/CMakeLists.txt
# cloudvault_common 单元测试
# ===========================================================

file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(cloudvault_common_tests ${TEST_SOURCES})

target_link_libraries(cloudvault_common_tests
    PRIVATE
        GTest::gtest_main
        GTest::gtest
        cloudvault_common
)

target_include_directories(cloudvault_common_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

include(GoogleTest)
gtest_discover_tests(cloudvault_common_tests)
