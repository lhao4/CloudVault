# CloudVault

基于 Qt 的 C/S 文件管理与即时通信示例项目，包含客户端与服务端两个子工程。

## 功能概览

- 用户注册、登录
- 在线用户查询
- 添加/删除好友
- 好友聊天
- 目录浏览与刷新
- 新建文件夹
- 文件上传
- 文件分享（文件/目录）

## 项目结构

```text
CloudVault/
├── QtProject/
│   ├── Client/   # Qt 客户端
│   └── Server/   # Qt 服务端
└── .gitignore
```

## 运行环境

- Qt 5/6（项目使用 `qmake`，`Client.pro` / `Server.pro`）
- C++11 编译器（MinGW / GCC / Clang 均可）
- MySQL（服务端使用 Qt SQL 的 `QMYSQL` 驱动）

## 数据库准备（MySQL）

服务端默认连接参数见 `QtProject/Server/operatedb.cpp`：

- host: `localhost`
- port: `3307`
- db: `mydbqt`
- user: `root`
- password: `123456`

可先执行以下 SQL 初始化：

```sql
CREATE DATABASE IF NOT EXISTS mydbqt DEFAULT CHARACTER SET utf8mb4;
USE mydbqt;

CREATE TABLE IF NOT EXISTS user_info (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(32) NOT NULL UNIQUE,
  pwd VARCHAR(64) NOT NULL,
  online TINYINT NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS friend (
  user_id INT NOT NULL,
  friend_id INT NOT NULL,
  PRIMARY KEY (user_id, friend_id)
);
```

## 配置文件

客户端与服务端均使用 `client.config`：

- `QtProject/Client/client.config`
- `QtProject/Server/client.config`

默认内容：

```text
127.0.0.1
5000
./filesys
```

含义依次为：`IP`、`端口`、`服务端文件根目录`。

## 构建与运行

### 方式一：Qt Creator（推荐）

1. 打开 `QtProject/Server/Server.pro`，构建并运行服务端
2. 打开 `QtProject/Client/Client.pro`，构建并运行客户端

### 方式二：命令行（qmake）

服务端：

```bash
cd QtProject/Server
qmake Server.pro
make
./Server
```

客户端：

```bash
cd QtProject/Client
qmake Client.pro
make
./Client
```

在 Windows + MinGW 下，可直接运行生成的 `Server.exe` / `Client.exe`。

## 注意事项

- 先启动服务端，再启动客户端
- 确保 MySQL 已启动且参数与 `operatedb.cpp` 一致
- 若提示找不到 `QMYSQL` 驱动，需要在 Qt 环境中安装/配置对应插件
- `filesys` 目录用于服务端文件存储，会在业务流程中自动创建用户目录
