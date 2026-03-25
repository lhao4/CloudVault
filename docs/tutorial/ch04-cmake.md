# 第四章　工程初始化与 CMake 配置

> **本章目标**：写好三个 CMakeLists.txt，让项目能够编译（即使现在还没有任何 .cpp 文件）。
>
> **验收标准**：运行 `cmake -B build` 不报错。

---

## 4.1 CMake 是什么，为什么用它

写完代码之后，需要一个工具把 .cpp 文件"翻译"成可执行程序。这个过程叫**构建（Build）**。

常见的构建方案有：

| 工具 | 特点 |
|------|------|
| `qmake` + `.pro` 文件 | Qt 专用，v1 用的就是这个，离开 Qt 就不能用 |
| **CMake** | 跨平台、跨编译器的通用构建系统，业界标准 |
| Makefile | 手写繁琐，跨平台困难 |

CMake 的工作方式分两步：

```
第一步（配置）：cmake -B build
  读取 CMakeLists.txt，在 build/ 目录生成真正的构建文件
  （Linux 生成 Makefile，Windows 生成 .sln，等等）

第二步（编译）：cmake --build build
  调用真正的编译器（g++/clang++/MSVC）编译代码
```

---

## 4.2 项目目录结构回顾

本章完成后，项目根目录结构如下：

```
CloudVault/
├── cmake/
│   └── FindMySQL.cmake      ← 自定义查找模块
├── common/
│   ├── CMakeLists.txt       ← 第一个要写的
│   ├── include/common/      ← 头文件放这里
│   └── src/                 ← 源文件放这里
├── server/
│   ├── CMakeLists.txt       ← 第二个要写的
│   ├── tests/
│   │   └── CMakeLists.txt   ← 第三个要写的
│   ├── include/server/
│   └── src/
└── client/
    ├── CMakeLists.txt       ← 第四个要写的
    ├── src/
    └── resources/
```

!!! note "为什么没有根目录的 CMakeLists.txt？"
    server（Linux）和 client（Windows）通常在不同机器上构建，
    各自独立构建比统一构建更符合实际开发流程。
    两者通过 `add_subdirectory(../common)` 共享协议库。

---

## 4.3 common/CMakeLists.txt 详解

`common` 是一个**静态库**，server 和 client 都会链接它。

### 什么是静态库？

```
common/src/protocol_codec.cpp  ──┐
common/src/crypto_utils.cpp    ──┤  编译  →  libcloudvault_common.a
                                 ┘

server 链接时：cloudvault_server + libcloudvault_common.a → 最终可执行文件
client 链接时：cloudvault_client + libcloudvault_common.a → 最终可执行文件
```

协议代码只写一份，两端都能用，这就是 common 库的价值。

### 关键概念：PUBLIC / PRIVATE / INTERFACE

这是 CMake 中最容易混淆的概念：

```
target_include_directories(A PUBLIC include/)

含义：
  编译 A 时，include/ 在搜索路径里           ✓
  编译链接了 A 的 B 时，include/ 也在搜索路径里  ✓（传递）

如果是 PRIVATE：
  编译 A 时，include/ 在搜索路径里           ✓
  编译 B 时，include/ 不在搜索路径里          ✗（不传递）
```

common 的头文件用 `PUBLIC`，这样 server 和 client 链接 common 后，
不需要再手动加 `target_include_directories`，就能 `#include "common/protocol.h"`。

### 完整文件

```cmake
cmake_minimum_required(VERSION 3.20)
project(cloudvault_common VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)

add_library(cloudvault_common STATIC
    src/protocol_codec.cpp
    src/crypto_utils.cpp
)

target_include_directories(cloudvault_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cloudvault_common
    PUBLIC
        OpenSSL::Crypto
)
```

---

## 4.4 server/CMakeLists.txt 详解

### FetchContent：自动下载依赖

v2 服务端新增了两个第三方库：`spdlog`（日志）和 `nlohmann/json`（配置解析）。

CMake 内置的 `FetchContent` 可以在配置阶段自动从 GitHub 下载它们：

```cmake
include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1          # 锁定版本，保证所有人用同一版本
)

FetchContent_MakeAvailable(spdlog)  # 下载并让其可用
```

!!! tip "第一次配置较慢"
    首次运行 `cmake -B build` 时，FetchContent 会下载这些库的源码（约几十 MB），
    之后会缓存在 `build/_deps/` 目录，再次配置不会重新下载。

### MySQL 查找模块

MySQL 客户端库（libmysqlclient）没有内置的 CMake 支持，
项目在 `cmake/FindMySQL.cmake` 里写了一个自定义查找模块：

```cmake
# 告诉 CMake 去 cmake/ 目录找自定义 Find 模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
find_package(MySQL REQUIRED)
```

查找成功后，`MySQL::MySQL` 这个 target 就可以用来链接了。

### 安装服务端系统依赖

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install libssl-dev libmysqlclient-dev
```

### 完整文件

```cmake
cmake_minimum_required(VERSION 3.20)
project(cloudvault_server VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(spdlog nlohmann_json)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
find_package(MySQL REQUIRED)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_BINARY_DIR}/common
)

file(GLOB_RECURSE SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(cloudvault_server ${SERVER_SOURCES})

target_include_directories(cloudvault_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cloudvault_server
    PRIVATE
        cloudvault_common
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        OpenSSL::SSL
        OpenSSL::Crypto
        MySQL::MySQL
        Threads::Threads
)

target_compile_options(cloudvault_server
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
```

---

## 4.5 server/tests/CMakeLists.txt 详解

### 为什么需要单独的测试 CMakeLists.txt？

`tests/` 是一个独立的子目录，有自己的 CMakeLists.txt，通过 `add_subdirectory(tests)` 引入。

这样做的好处：
- 构建时可以用 `-DBUILD_TESTING=OFF` 关闭测试，加快编译速度
- 测试代码与业务代码分离，结构清晰

### GoogleTest 基础

```cpp
// server/tests/test_pdu.cpp 示例
#include <gtest/gtest.h>
#include "common/protocol_codec.h"

// TEST(测试套件名, 测试用例名)
TEST(PDUBuilderTest, BuildLoginRequest) {
    PDUBuilder builder(MessageType::LoginRequest);
    builder.writeFixedString("alice", 32);

    auto bytes = builder.build();

    // EXPECT_*：失败后继续执行后续断言
    // ASSERT_*：失败后立即终止当前测试
    EXPECT_EQ(bytes.size(), 16 + 32);  // 头部 16 字节 + 用户名 32 字节
}
```

运行测试：

```bash
cd server
cmake -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## 4.6 client/CMakeLists.txt 详解

### Qt 的三个自动化工具

Qt 扩展了 C++ 语法（信号槽、.ui 文件、资源文件），需要三个工具在编译前预处理代码：

```
源文件             预处理器          生成文件
─────────────────────────────────────────────
login_window.h  → MOC  →  moc_login_window.cpp  （信号槽实现）
login_window.ui → UIC  →  ui_login_window.h      （界面布局代码）
images.qrc      → RCC  →  qrc_images.cpp         （图片二进制数据）
```

只需在 CMakeLists.txt 中开启三个选项，CMake 会自动调用这些工具：

```cmake
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
```

### 查找 Qt6

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
```

`find_package` 需要知道 Qt6 安装在哪里。有两种方式告诉它：

**方式一：设置环境变量**（推荐，一劳永逸）

```powershell
# Windows PowerShell，加入系统环境变量
[System.Environment]::SetEnvironmentVariable(
    "Qt6_DIR",
    "C:\Qt\6.7.0\msvc2022_64\lib\cmake\Qt6",
    "User"
)
```

**方式二：cmake 命令行参数**（临时使用）

```bash
cmake -B build -DQt6_DIR="C:/Qt/6.7.0/msvc2022_64/lib/cmake/Qt6"
```

### WIN32 关键字

```cmake
add_executable(cloudvault_client WIN32 ${CLIENT_SOURCES} ...)
```

`WIN32` 告诉 Windows 这是一个 GUI 程序，启动时不弹出黑色命令行窗口。
在 Linux/macOS 下这个关键字会被自动忽略，不影响跨平台编译。

---

## 4.7 首次编译验证

### 服务端（Linux）

```bash
# 安装系统依赖
sudo apt update && sudo apt install libssl-dev libmysqlclient-dev

# 进入 server 目录
cd server

# 配置（首次会下载 FetchContent 依赖，需要网络）
cmake -B build -G Ninja

# 编译
cmake --build build

# 此时还没有任何 .cpp 源文件，所以不会产生最终可执行文件，
# 但 cmake 配置阶段不报错即代表 CMakeLists.txt 正确。
```

!!! tip "安装 Ninja"
    `-G Ninja` 使用 Ninja 作为构建后端，比默认的 Make 快很多。
    安装：`sudo apt install ninja-build`

    不安装也可以去掉 `-G Ninja`，使用默认的 Make。

### 客户端（Windows）

```powershell
# 进入 client 目录
cd client

# 配置（需要已安装 Qt6 并设置好 Qt6_DIR）
cmake -B build -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build build --config Release
```

### 常见报错

| 报错信息 | 原因 | 解决方法 |
|---------|------|---------|
| `Could not find OpenSSL` | 未安装 libssl-dev | `sudo apt install libssl-dev` |
| `Could not find MySQL` | 未安装 libmysqlclient-dev | `sudo apt install libmysqlclient-dev` |
| `Could not find Qt6` | Qt6_DIR 未设置 | 设置 Qt6_DIR 环境变量 |
| `CMake 3.x or higher is required` | CMake 版本太旧 | `sudo apt install cmake` 或从官网下载新版 |

---

## 4.8 本章小结

本章完成了：

- [x] `common/CMakeLists.txt`：静态库，PUBLIC 传递头文件和 OpenSSL
- [x] `server/CMakeLists.txt`：FetchContent 管理 spdlog/json，自定义 FindMySQL
- [x] `server/tests/CMakeLists.txt`：GoogleTest 测试框架
- [x] `client/CMakeLists.txt`：Qt6 三大自动化工具，WIN32 GUI 模式
- [x] `cmake/FindMySQL.cmake`：自定义 MySQL 查找模块

下一章开始写第一行业务代码：[第五章 — 两端骨架，让程序先跑起来](ch05-skeleton.md)

---

## 本章新知识点

- [CMake 构建系统详解](../knowledge/cmake-guide.md)
