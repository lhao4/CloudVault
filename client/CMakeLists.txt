# ===========================================================
# client/CMakeLists.txt
# 构建客户端可执行文件 cloudhive_client
#
# 平台：Windows（也可在 Linux 上构建用于开发调试）
# Qt 版本：Qt 6（需通过 Qt 官方安装器安装）
#
# Qt 安装器下载：https://www.qt.io/download-qt-installer
# 安装时勾选：Qt 6.x → Desktop (MSVC 2022 64-bit 或 MinGW 64-bit)
#
# 构建前需要设置 Qt6_DIR 环境变量，指向 Qt 安装目录下的 cmake 文件夹，例如：
#   Windows MSVC：C:/Qt/6.7.0/msvc2022_64/lib/cmake/Qt6
#   Windows MinGW：C:/Qt/6.7.0/mingw_64/lib/cmake/Qt6
# 也可以在 cmake 配置时传入：
#   cmake -B build -DQt6_DIR="C:/Qt/6.7.0/msvc2022_64/lib/cmake/Qt6"
# ===========================================================

cmake_minimum_required(VERSION 3.20)
project(cloudhive_client VERSION 2.0 LANGUAGES CXX)

# ── C++ 标准 ─────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt 自动化工具 ─────────────────────────────────────────
# Qt 使用了 C++ 标准之外的机制（信号槽、.ui 文件、资源文件），
# 需要三个预处理器在编译前处理源码：
#
# CMAKE_AUTOMOC：自动运行 MOC（Meta-Object Compiler）。
#   Qt 的信号槽（signals/slots）不是标准 C++，
#   MOC 会扫描带有 Q_OBJECT 宏的头文件，生成额外的 moc_*.cpp 文件。
#   开启后，CMake 会自动检测哪些文件需要 MOC 处理。
#
# CMAKE_AUTOUIC：自动运行 UIC（User Interface Compiler）。
#   Qt Designer 设计的 .ui 文件（XML 格式的界面描述）
#   会被 UIC 转换为 C++ 头文件（ui_*.h）。
#
# CMAKE_AUTORCC：自动运行 RCC（Resource Compiler）。
#   .qrc 资源文件（图片、图标等）会被编译进可执行文件。
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── 查找 Qt6 ─────────────────────────────────────────────
# COMPONENTS：只引入用到的 Qt 模块，减少编译时间。
#   Widgets：QWidget、QPushButton 等 GUI 控件
#   Network：QTcpSocket、QSslSocket 等网络类
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# ── 引入 common 静态库 ────────────────────────────────────
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_BINARY_DIR}/common
)

# ── 收集源文件 ────────────────────────────────────────────
# 同时收集 .cpp、.h、.ui、.qrc 文件。
# .h 文件加进来是为了让 AUTOMOC 正确识别需要处理的头文件。
# .ui 和 .qrc 文件交给 AUTOUIC 和 AUTORCC 处理。
file(GLOB_RECURSE CLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

file(GLOB_RECURSE CLIENT_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/*.qrc
)

file(GLOB_RECURSE CLIENT_UI_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui
)

# ── 定义可执行文件 ────────────────────────────────────────
# WIN32：Windows 平台下以 GUI 模式启动，不弹出黑色控制台窗口。
#   在 Linux/macOS 下此选项被忽略。
add_executable(cloudhive_client
    WIN32
    ${CLIENT_SOURCES}
    ${CLIENT_RESOURCES}
    ${CLIENT_UI_FILES}
)

# ── 头文件路径 ────────────────────────────────────────────
# CMAKE_CURRENT_BINARY_DIR：当前构建目录（build/）。
#   AUTOUIC 生成的 ui_*.h 文件放在这里，
#   必须加入搜索路径，否则 #include "ui_login_window.h" 会找不到文件。
target_include_directories(cloudhive_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}     # AUTOUIC 生成的 ui_*.h
)

# ── 链接依赖 ─────────────────────────────────────────────
# Qt6::Widgets：GUI 控件
# Qt6::Network：QTcpSocket
# cloudhive_common：共享协议库（会自动传递 OpenSSL::Crypto）
target_link_libraries(cloudhive_client
    PRIVATE
        Qt6::Widgets
        Qt6::Network
        cloudhive_common
)

# ── Windows 部署辅助 ──────────────────────────────────────
# windeployqt：Qt 提供的工具，自动复制运行所需的 Qt DLL 到输出目录。
# 只在 Windows 下执行，其他平台忽略。
# 构建完成后在 build/Release/ 目录运行 windeployqt cloudhive_client.exe 即可。
if(WIN32)
    add_custom_command(TARGET cloudhive_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
            "提示：运行 windeployqt $<TARGET_FILE:cloudhive_client> 复制 Qt 依赖 DLL"
    )
endif()
