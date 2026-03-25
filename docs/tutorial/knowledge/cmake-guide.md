# CMake 构建系统详解

## 1. 为什么不用手写 g++ 命令

1. 依赖多时命令难维护。
2. 跨平台参数差异大。
3. IDE 集成能力弱。

## 2. 核心结构

```cmake
cmake_minimum_required(VERSION 3.20)
project(cloudvault LANGUAGES CXX)
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE cloudvault_common)
```

## 3. 常用 `target_*` 命令

1. `target_include_directories`
2. `target_link_libraries`
3. `target_compile_options`
4. `target_compile_definitions`

## 4. 依赖管理

1. `find_package`：使用系统已安装库。
2. `FetchContent`：构建期拉取第三方源码。

## 5. Qt 相关

1. `CMAKE_AUTOMOC`
2. `CMAKE_AUTOUIC`
3. `CMAKE_AUTORCC`

## 6. 构建类型

```bash
cmake -S Server -B build/server -DCMAKE_BUILD_TYPE=Debug
cmake -S Server -B build/server-rel -DCMAKE_BUILD_TYPE=Release
```
