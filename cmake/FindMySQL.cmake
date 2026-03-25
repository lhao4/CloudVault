# ===========================================================
# cmake/FindMySQL.cmake
# 自定义查找模块：在系统中定位 MySQL 客户端库
#
# 为什么需要自定义？
#   CMake 内置了对 Qt、OpenSSL、Boost 等常见库的 find_package 支持，
#   但没有内置 MySQL 的查找模块，所以我们自己写一个。
#
# 使用方式（在 server/CMakeLists.txt 中）：
#   list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake")
#   find_package(MySQL REQUIRED)
#   target_link_libraries(... MySQL::MySQL)
#
# 查找成功后会创建导入 target：MySQL::MySQL
# ===========================================================

# find_path()：在常见系统路径中查找包含 mysql.h 的目录。
# NAMES：要查找的头文件名。
# PATH_SUFFIXES：在标准路径下额外尝试的子目录。
find_path(MySQL_INCLUDE_DIR
    NAMES mysql/mysql.h
    PATH_SUFFIXES mysql
    PATHS
        /usr/include
        /usr/local/include
        /usr/include/mysql
        /usr/local/include/mysql
        /opt/homebrew/include          # macOS Homebrew
)

# find_library()：在常见系统路径中查找 mysqlclient 动态/静态库文件。
# NAMES：库文件的名字（不含 lib 前缀和 .so/.a 后缀）。
find_library(MySQL_LIBRARY
    NAMES mysqlclient mysqlclient_r
    PATHS
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu      # Debian/Ubuntu 64-bit
        /usr/lib/aarch64-linux-gnu     # ARM64
        /opt/homebrew/lib              # macOS Homebrew
)

# include(FindPackageHandleStandardArgs)：
#   CMake 标准宏，统一处理 REQUIRED / QUIET 等参数，
#   并在找到/找不到时输出标准格式的消息。
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
    REQUIRED_VARS MySQL_LIBRARY MySQL_INCLUDE_DIR
)

# 找到后创建导入 target MySQL::MySQL，
# 让调用者可以用 target_link_libraries(... MySQL::MySQL) 链接。
if(MySQL_FOUND AND NOT TARGET MySQL::MySQL)
    add_library(MySQL::MySQL UNKNOWN IMPORTED)
    set_target_properties(MySQL::MySQL PROPERTIES
        IMPORTED_LOCATION             "${MySQL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MySQL_INCLUDE_DIR}"
    )
endif()

# 隐藏内部变量，不在 cmake-gui 中显示
mark_as_advanced(MySQL_INCLUDE_DIR MySQL_LIBRARY)
