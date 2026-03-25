# CloudVault v2 — 测试规范

> **版本**：v2.0 | **状态**：草案

---

## 1. 测试策略概览

```
┌─────────────────────────────────────────────────────┐
│                   测试金字塔                          │
│                                                     │
│              ┌───────────────┐                      │
│              │   手动测试     │  端到端冒烟测试        │
│              └───────┬───────┘                      │
│           ┌──────────┴──────────┐                   │
│           │    集成测试          │  数据库 / 网络       │
│           └──────────┬──────────┘                   │
│    ┌─────────────────┴─────────────────┐            │
│    │         单元测试（主体）            │             │
│    └───────────────────────────────────┘            │
└─────────────────────────────────────────────────────┘
```

| 层级 | 工具 | 覆盖目标 | 运行频率 |
|------|------|----------|----------|
| 单元测试 | GoogleTest 1.14 | common/ + server/handler/ ≥ 80% | 每次提交 |
| 集成测试 | GoogleTest + 真实 MySQL | server/ 整体流程 | 每次 PR |
| 手动测试 | 测试清单（本文 §5） | 全功能冒烟 | 每个 milestone |

---

## 2. 单元测试

### 2.1 目录结构

```
tests/
├── CMakeLists.txt
├── common/
│   ├── test_pdu.cpp          # PDUBuilder / PDUParser
│   ├── test_crypto.cpp       # hashPassword / verifyPassword
│   └── test_constants.cpp    # 常量合理性断言
└── server/
    ├── test_auth_handler.cpp
    ├── test_friend_handler.cpp
    ├── test_chat_handler.cpp
    ├── test_file_handler.cpp
    ├── test_session_manager.cpp
    ├── test_file_storage.cpp
    └── test_database.cpp     # 需要测试数据库（见 §3）
```

### 2.2 common — PDU 测试（`test_pdu.cpp`）

```cpp
// 构建 → 解析 往返
TEST(PDUBuilder, RoundTrip) {
    auto pdu = PDUBuilder{}
        .type(MessageType::LOGIN_REQ)
        .field<uint32_t>(1001)          // user_id
        .field<std::string>("alice")    // username
        .build();

    PDUParser parser;
    parser.feed(pdu.data(), pdu.size());
    ASSERT_TRUE(parser.hasComplete());

    auto msg = parser.pop();
    EXPECT_EQ(msg.header.message_type,
              static_cast<uint16_t>(MessageType::LOGIN_REQ));
    EXPECT_EQ(msg.body.size(), pdu.size() - 16);
}

// 头部字节序（小端）
TEST(PDUHeader, LittleEndian) {
    PDUHeader h{};
    h.total_length = 0x00000114;   // 276
    auto* p = reinterpret_cast<uint8_t*>(&h);
    EXPECT_EQ(p[0], 0x14);
    EXPECT_EQ(p[1], 0x01);
}

// 超大包拒绝
TEST(PDUParser, RejectOversizedPDU) {
    PDUHeader h{};
    h.total_length = 64 * 1024 * 1024 + 1;  // 超过 64MB
    std::vector<uint8_t> buf(16);
    memcpy(buf.data(), &h, 16);
    PDUParser parser;
    parser.feed(buf.data(), buf.size());
    EXPECT_TRUE(parser.hasError());
}

// 分片流式输入
TEST(PDUParser, FragmentedInput) {
    auto pdu = PDUBuilder{}.type(MessageType::PING).build();
    PDUParser parser;
    // 分两次喂入
    parser.feed(pdu.data(), 8);
    EXPECT_FALSE(parser.hasComplete());
    parser.feed(pdu.data() + 8, pdu.size() - 8);
    EXPECT_TRUE(parser.hasComplete());
}
```

### 2.3 common — 加密测试（`test_crypto.cpp`）

```cpp
// 哈希 + 验证往返
TEST(Crypto, HashVerifyRoundTrip) {
    std::string pwd = "MyP@ssw0rd!";
    auto hash = crypto::hashPassword(pwd);
    EXPECT_TRUE(crypto::verifyPassword(pwd, hash));
    EXPECT_FALSE(crypto::verifyPassword("wrong", hash));
}

// 每次盐值不同
TEST(Crypto, DifferentSalts) {
    auto h1 = crypto::hashPassword("abc");
    auto h2 = crypto::hashPassword("abc");
    EXPECT_NE(h1, h2);  // salt 不同
}

// 格式为 hex(salt):hex(hash)，共 97 字节
TEST(Crypto, StoredHashFormat) {
    auto h = crypto::hashPassword("test");
    EXPECT_EQ(h.size(), 97u);
    auto colon = h.find(':');
    EXPECT_NE(colon, std::string::npos);
    EXPECT_EQ(colon, 32u);   // 16字节salt → 32个hex字符
}

// 传输哈希（无盐）确定性
TEST(Crypto, TransportHashDeterministic) {
    auto t1 = crypto::hashForTransport("abc");
    auto t2 = crypto::hashForTransport("abc");
    EXPECT_EQ(t1, t2);
    EXPECT_EQ(t1.size(), 64u);  // SHA-256 → 64 hex chars
}
```

### 2.4 server — SessionManager 测试

```cpp
TEST(SessionManager, OnlineOffline) {
    SessionManager sm;
    sm.addSession(42, conn1);
    EXPECT_TRUE(sm.isOnline(42));
    EXPECT_EQ(sm.getSession(42), conn1);

    sm.removeSession(42);
    EXPECT_FALSE(sm.isOnline(42));
    EXPECT_EQ(sm.getSession(42), nullptr);
}

TEST(SessionManager, ConcurrentAccess) {
    SessionManager sm;
    std::vector<std::thread> threads;
    for (int i = 0; i < 50; ++i) {
        threads.emplace_back([&, i] {
            sm.addSession(i, nullptr);
            sm.isOnline(i);
            sm.removeSession(i);
        });
    }
    for (auto& t : threads) t.join();
    // 不崩溃、无数据竞争
}
```

### 2.5 server — FileStorage 路径安全测试

```cpp
TEST(FileStorage, PathTraversalBlocked) {
    FileStorage fs("/srv/cloudvault");
    EXPECT_THROW(fs.safePath(1, "../etc/passwd"),
                 std::invalid_argument);
    EXPECT_THROW(fs.safePath(1, "a/../../secret"),
                 std::invalid_argument);
    EXPECT_THROW(fs.safePath(1, "/absolute/path"),
                 std::invalid_argument);
}

TEST(FileStorage, ValidPathAccepted) {
    FileStorage fs("/srv/cloudvault");
    auto p = fs.safePath(1, "docs/report.pdf");
    EXPECT_EQ(p, "/srv/cloudvault/1/docs/report.pdf");
}
```

### 2.6 server — Handler 单元测试（Mock 注入）

Handler 构造时注入依赖，可用 GoogleMock 替换：

```cpp
class MockUserRepository : public IUserRepository {
public:
    MOCK_METHOD(std::optional<UserRecord>, findByName,
                (const std::string&), (override));
    MOCK_METHOD(bool, create,
                (const std::string&, const std::string&), (override));
};

TEST(AuthHandler, LoginSuccess) {
    auto mockRepo = std::make_shared<MockUserRepository>();
    UserRecord rec{1001, "alice", crypto::hashPassword("pass"), 0};
    EXPECT_CALL(*mockRepo, findByName("alice"))
        .WillOnce(Return(std::make_optional(rec)));

    MockSessionManager sm;
    AuthHandler handler(mockRepo, sm);

    // 构造 LOGIN_REQ PDU
    auto pdu = PDUBuilder{}.type(MessageType::LOGIN_REQ)
        .field<std::string>("alice")
        .field<std::string>(crypto::hashForTransport("pass"))
        .build();

    auto conn = std::make_shared<MockConnection>();
    handler.handle(conn, PDUParser::parse(pdu));

    // 期望回复 LOGIN_RESP + STATUS_OK
    auto resp = conn->lastSent();
    EXPECT_EQ(resp.type, MessageType::LOGIN_RESP);
    EXPECT_EQ(resp.status, STATUS_OK);
}

TEST(AuthHandler, LoginWrongPassword) {
    // ... STATUS_WRONG_PASSWORD
}

TEST(AuthHandler, LoginDuplicateSession) {
    // ... STATUS_ALREADY_LOGGED_IN
}
```

### 2.7 覆盖率要求

| 模块 | 行覆盖率目标 | 分支覆盖率目标 |
|------|------------|--------------|
| common/pdu | ≥ 95% | ≥ 90% |
| common/crypto | ≥ 95% | ≥ 90% |
| server/session_manager | ≥ 90% | ≥ 85% |
| server/file_storage | ≥ 90% | ≥ 85% |
| server/handlers/ | ≥ 80% | ≥ 75% |
| server/database/ | ≥ 70%（集成补充） | — |

生成覆盖率报告：
```bash
cmake -DENABLE_COVERAGE=ON ..
make && make test
make coverage   # 生成 lcov HTML 报告至 build/coverage/
```

---

## 3. 集成测试

### 3.1 测试数据库配置

```bash
# 创建独立测试库（不影响开发/生产）
mysql -u root -p <<'EOF'
CREATE DATABASE cloudvault_test CHARACTER SET utf8mb4;
CREATE USER 'cv_test'@'localhost' IDENTIFIED BY 'test_only';
GRANT ALL ON cloudvault_test.* TO 'cv_test'@'localhost';
EOF
```

测试配置文件 `tests/config/test_server.json`：
```json
{
  "database": {
    "host": "127.0.0.1",
    "port": 3306,
    "name": "cloudvault_test",
    "user": "cv_test"
  },
  "storage": { "root": "/tmp/cv_test_storage" }
}
```

每个测试套件通过 `SetUpTestSuite` / `TearDownTestSuite` 初始化和清理：
```cpp
class DatabaseTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        db = Database::create(testConfig());
        db->exec("SOURCE " + schemaPath());
    }
    static void TearDownTestSuite() {
        db->exec("DROP DATABASE cloudvault_test");
        db.reset();
    }
    void TearDown() override {
        // 每条测试后清空数据（保留表结构）
        db->exec("DELETE FROM offline_message");
        db->exec("DELETE FROM chat_message");
        db->exec("DELETE FROM group_member");
        db->exec("DELETE FROM chat_group");
        db->exec("DELETE FROM friend");
        db->exec("DELETE FROM user_info");
    }
};
```

### 3.2 数据库集成测试用例

```
UserRepository
  ✓ createUser — 成功创建，password_hash 格式正确
  ✓ createUser — 重复用户名返回 false
  ✓ findByName — 找到 / 找不到
  ✓ setOnline / setOffline — 状态字段更新
  ✓ getOnlineUsers — 仅返回在线用户

FriendRepository
  ✓ addRequest — 插入 pending 记录
  ✓ acceptRequest — 状态变为 accepted
  ✓ getFriends — 仅返回 accepted 双向关系
  ✓ deleteFriend — 双向删除

ChatRepository
  ✓ saveMessage — 1对1 消息持久化
  ✓ saveMessage — 群聊消息持久化（CHECK 约束）
  ✓ getOfflineMessages — 返回未投递消息
  ✓ markDelivered — 清除离线消息
```

### 3.3 网络层集成测试

启动真实 `TcpServer`（随机端口），用 `QTcpSocket` 或 POSIX socket 连接：

```
连接生命周期
  ✓ 客户端连接 → 服务端 accept → 30s 无登录 → 自动断开
  ✓ 客户端异常断开 → 服务端清理 SessionManager 在线状态
  ✓ 同一账号二次登录 → STATUS_ALREADY_LOGGED_IN

认证流程
  ✓ 正确账号密码 → STATUS_OK + user_id
  ✓ 错误密码 → STATUS_WRONG_PASSWORD
  ✓ 不存在用户 → STATUS_USER_NOT_FOUND

聊天转发
  ✓ A 登录 → B 登录 → A 发消息 → B 收到
  ✓ A 登录 → B 未登录 → A 发消息 → B 登录后收到离线消息

文件上传/下载
  ✓ 100KB 文件上传 → MD5 校验一致
  ✓ 4MB 分片上传（2片）→ 合并后 MD5 校验一致
```

---

## 4. 性能测试

### 4.1 并发连接测试

```bash
# 使用 stress_client 工具（tests/tools/stress_client.cpp）
./stress_client --host 127.0.0.1 --port 9000 \
    --clients 50 --duration 60 --mode chat
```

验收标准：
- 50 并发客户端，持续 60 秒
- 消息转发 P99 延迟 < 100ms（局域网）
- 服务端内存增长 < 50MB
- 无崩溃、无内存泄漏（Valgrind 运行 5 分钟验证）

### 4.2 大文件传输测试

```bash
# 生成测试文件
dd if=/dev/urandom of=/tmp/test_1gb.bin bs=1M count=1024

# 上传测试
./file_bench --host 127.0.0.1 --port 9000 \
    --file /tmp/test_1gb.bin --chunk-size 4194304
```

验收标准：
- 1GB 文件上传成功，SHA-256 一致
- 断点续传：上传 50% 时中断，重连后继续，最终文件完整
- 20GB 文件（分批测试）：服务端内存无异常增长

---

## 5. 手动测试清单

### 5.1 用户系统

| # | 操作 | 预期结果 | 通过 |
|---|------|----------|------|
| 1 | 注册新用户（合法用户名）| 注册成功，进入主界面 | ☐ |
| 2 | 注册重复用户名 | 提示用户名已存在 | ☐ |
| 3 | 注册超长用户名（>32字符）| 客户端阻止提交 | ☐ |
| 4 | 登录正确账号密码 | 登录成功 | ☐ |
| 5 | 登录错误密码 | 提示密码错误 | ☐ |
| 6 | 同一账号两端同时登录 | 第二次登录被拒绝 | ☐ |
| 7 | 正常登出 | 服务端标记离线 | ☐ |

### 5.2 好友系统

| # | 操作 | 预期结果 | 通过 |
|---|------|----------|------|
| 8 | 搜索存在的用户 | 显示用户名和在线状态 | ☐ |
| 9 | 搜索不存在的用户 | 提示用户不存在 | ☐ |
| 10 | 向在线用户发好友请求 | 对方实时收到请求通知 | ☐ |
| 11 | 向离线用户发好友请求 | 对方上线后收到请求 | ☐ |
| 12 | 同意好友请求 | 双方好友列表互相显示 | ☐ |
| 13 | 删除好友 | 双方好友列表消失 | ☐ |

### 5.3 聊天系统

| # | 操作 | 预期结果 | 通过 |
|---|------|----------|------|
| 14 | 向在线好友发消息 | 对方实时收到 | ☐ |
| 15 | 向离线好友发消息 | 对方上线后收到离线消息 | ☐ |
| 16 | 发送空消息 | 客户端阻止发送 | ☐ |
| 17 | 创建群聊并邀请成员 | 成员收到入群通知 | ☐ |
| 18 | 群内发消息 | 所有在线群成员收到 | ☐ |
| 19 | 退出群聊 | 不再收到群消息 | ☐ |

### 5.4 文件管理

| # | 操作 | 预期结果 | 通过 |
|---|------|----------|------|
| 20 | 浏览根目录 | 显示文件和文件夹列表 | ☐ |
| 21 | 新建文件夹 | 列表中出现新文件夹 | ☐ |
| 22 | 进入/返回子目录 | 路径正确切换 | ☐ |
| 23 | 上传小文件（< 1MB）| 上传成功，列表出现文件 | ☐ |
| 24 | 下载文件 | 本地文件与服务端 MD5 一致 | ☐ |
| 25 | 重命名文件/文件夹 | 名称更新，路径正确 | ☐ |
| 26 | 删除文件 | 文件从列表和磁盘消失 | ☐ |
| 27 | 删除非空文件夹 | 递归删除成功 | ☐ |
| 28 | 搜索文件名 | 返回匹配结果 | ☐ |
| 29 | 上传 1GB 大文件（分片）| 进度条更新，完成后 MD5 一致 | ☐ |
| 30 | 大文件上传中断后续传 | 从断点继续，最终完整 | ☐ |

### 5.5 文件分享

| # | 操作 | 预期结果 | 通过 |
|---|------|----------|------|
| 31 | 分享文件给在线好友 | 好友实时收到分享通知 | ☐ |
| 32 | 好友接受分享 | 文件出现在好友文件空间 | ☐ |
| 33 | 分享文件给离线好友 | 好友上线后收到通知 | ☐ |

### 5.6 安全性

| # | 操作 | 预期结果 | 通过 |
|---|------|----------|------|
| 34 | 文件路径含 `../` | 服务端拒绝，客户端无崩溃 | ☐ |
| 35 | SQL 注入（用户名含单引号）| 正常处理，不产生 SQL 错误 | ☐ |
| 36 | 超大 PDU（构造 >64MB 包）| 服务端断开连接，不崩溃 | ☐ |
| 37 | 未登录直接发文件请求 | 服务端返回 STATUS_NOT_LOGGED_IN | ☐ |

---

## 6. CMake 集成

### 6.1 `tests/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# 公共编译选项
function(add_cv_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name}
        PRIVATE GTest::gtest_main GTest::gmock_main
                cloudvault_common cloudvault_server_lib)
    target_include_directories(${name}
        PRIVATE ${CMAKE_SOURCE_DIR}/common/include
                ${CMAKE_SOURCE_DIR}/server/include)
    gtest_discover_tests(${name})
endfunction()

add_cv_test(test_pdu    common/test_pdu.cpp)
add_cv_test(test_crypto common/test_crypto.cpp)
add_cv_test(test_session_manager server/test_session_manager.cpp)
add_cv_test(test_file_storage    server/test_file_storage.cpp)
add_cv_test(test_auth_handler    server/test_auth_handler.cpp)
add_cv_test(test_friend_handler  server/test_friend_handler.cpp)
add_cv_test(test_chat_handler    server/test_chat_handler.cpp)
add_cv_test(test_file_handler    server/test_file_handler.cpp)

# 集成测试（需要数据库，可选运行）
option(ENABLE_INTEGRATION_TESTS "Enable DB integration tests" OFF)
if(ENABLE_INTEGRATION_TESTS)
    add_cv_test(test_database server/test_database.cpp)
    target_compile_definitions(test_database
        PRIVATE CV_TEST_DB_CONFIG="${CMAKE_SOURCE_DIR}/tests/config/test_server.json")
endif()
```

### 6.2 运行命令

```bash
# 构建并运行所有单元测试
cmake -B build -DBUILD_TESTING=ON
cmake --build build --target all
ctest --test-dir build --output-on-failure

# 运行集成测试
cmake -B build -DBUILD_TESTING=ON -DENABLE_INTEGRATION_TESTS=ON
cmake --build build
ctest --test-dir build -R "test_database" --output-on-failure

# 生成覆盖率报告（需 gcc + lcov）
cmake -B build -DBUILD_TESTING=ON -DENABLE_COVERAGE=ON
cmake --build build
ctest --test-dir build
cmake --build build --target coverage
# 报告输出至 build/coverage/index.html
```

---

## 7. CI 流水线（GitHub Actions 示意）

```yaml
# .github/workflows/ci.yml
name: CI

on: [push, pull_request]

jobs:
  unit-test:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: sudo apt-get install -y libssl-dev cmake ninja-build
      - name: Build & Test
        run: |
          cmake -B build -G Ninja -DBUILD_TESTING=ON
          cmake --build build
          ctest --test-dir build --output-on-failure

  integration-test:
    runs-on: ubuntu-22.04
    services:
      mysql:
        image: mysql:8.0
        env:
          MYSQL_ROOT_PASSWORD: rootpwd
          MYSQL_DATABASE: cloudvault_test
        ports: ["3306:3306"]
    steps:
      - uses: actions/checkout@v4
      - name: Build & Integration Test
        env:
          CV_DB_PASSWORD: rootpwd
        run: |
          cmake -B build -G Ninja \
            -DBUILD_TESTING=ON \
            -DENABLE_INTEGRATION_TESTS=ON
          cmake --build build
          ctest --test-dir build --output-on-failure
```

---

## 8. 已知测试限制

| 限制 | 说明 | 计划 |
|------|------|------|
| 客户端 UI 测试 | Qt Widget 自动化测试复杂，暂不覆盖 | v2.1 引入 Qt Test |
| 大文件性能测试 | 20GB 文件测试需专用环境 | 手动测试 + CI 不跑 |
| TLS 握手测试 | 需要证书管理，集成测试暂跳过 | v2.1 |
| 跨平台测试 | 服务端仅在 Linux 测试 | 设计上不支持 Windows |
