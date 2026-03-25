# 第六章　协议基础——PDU 与 common 层

> **状态**：✅ 已完成
>
> **本章目标**：实现 `common` 静态库中的协议编解码和密码哈希，并通过单元测试。
>
> **验收标准**：`ctest` 17 个测试用例全部通过。

---

## 6.1 为什么需要自定义协议

TCP 是**流式协议**，没有"消息边界"的概念。连续发送两条消息，接收方可能：

```
发送方连续发送：[消息A][消息B]

接收方可能收到：[消息A + 消息B]          ← 粘包：两条消息粘在一起
                [消息A 的前半段]          ← 半包：一条消息被截断
                [消息A 的前半段][消息A 的后半段 + 消息B]
```

解决方案：在每条消息前加固定长度的**头部（Header）**，头部里写明消息的总长度。接收方先读头部，得知消息总长度，再等待足够的字节。

---

## 6.2 PDU v2 格式

**PDU（Protocol Data Unit）**是本项目的基本传输单元：

```
┌─────────────────────────────────────┐
│          PDU Header（16 字节）       │
│  total_length : uint32_t            │  ← 整个 PDU 的总字节数（含头）
│  body_length  : uint32_t            │  ← body 部分的字节数
│  message_type : uint32_t            │  ← MessageType 枚举值
│  reserved     : uint32_t            │  ← 保留，填 0
├─────────────────────────────────────┤
│          Body（变长）                │
│  各消息类型自定义格式                 │
└─────────────────────────────────────┘
```

**字节序**：所有多字节字段均使用**网络字节序（大端）**。网络协议的惯例是大端，`htonl` / `ntohl` 负责在主机字节序和网络字节序之间转换。

### 6.2.1 头部结构体

```cpp
// #pragma pack(push, 1)：关闭字节对齐，禁止编译器插入 padding
// 保证结构体内存布局与网络帧完全一致
#pragma pack(push, 1)
struct PDUHeader {
    uint32_t total_length;   // 整个 PDU 的总字节数
    uint32_t body_length;    // body 部分的字节数
    uint32_t message_type;   // MessageType 枚举值
    uint32_t reserved;       // 保留，始终为 0
};
#pragma pack(pop)

static_assert(sizeof(PDUHeader) == 16, "PDUHeader must be exactly 16 bytes");
```

> **为什么 total_length 和 body_length 都要存？**
> 看起来 `body_length = total_length - 16` 是多余的，但这种冗余让接收方做了两次一致性校验，能更快发现损坏的数据流。

### 6.2.2 MessageType 枚举

消息类型按功能模块分组，高字节表示模块，低字节表示具体消息：

```cpp
enum class MessageType : uint32_t {
    PING  = 0x0001,  PONG  = 0x0002,  // 连接保活（第七章）
    LOGIN_REQUEST  = 0x0101, ...       // 认证（第八章）
    FIND_USER      = 0x0201, ...       // 好友（第九章）
    CHAT           = 0x0301, ...       // 聊天（第十章）
    MKDIR          = 0x0401, ...       // 文件管理（第十一章）
    UPLOAD_REQUEST = 0x0501, ...       // 文件传输（第十二章）
    SHARE_REQUEST  = 0x0601, ...       // 文件分享（第十三章）
};
```

---

## 6.3 PDUBuilder：链式 API 构建 PDU

`PDUBuilder` 内部维护一个 `body_` 字节数组，链式调用逐字段追加，最后 `build()` 拼接头部和 body 返回完整字节序列。

```cpp
// 用法示例
auto pdu = PDUBuilder(MessageType::LOGIN_REQUEST)
    .writeString(username)          // uint16 长度前缀 + UTF-8 字节
    .writeFixedString(hash, 64)     // 固定 64 字节，不足则补 0
    .writeUInt32(session_id)        // 4 字节，大端
    .build();                       // 返回 std::vector<uint8_t>
```

| 方法 | 写入字节数 | 说明 |
|------|-----------|------|
| `writeUInt8(v)` | 1 | 直接追加 |
| `writeUInt16(v)` | 2 | `htons` 转换 |
| `writeUInt32(v)` | 4 | `htonl` 转换 |
| `writeUInt64(v)` | 8 | 手动字节翻转 |
| `writeString(s)` | 2 + len | uint16 长度前缀 + 内容 |
| `writeFixedString(s, n)` | n | 截断或补零至 n 字节 |
| `writeBytes(p, n)` | n | 任意字节块 |

---

## 6.4 PDUParser：流式解析，处理粘包和半包

`PDUParser` 内部维护一个 `buffer_` 缓冲区，每次 `feed()` 追加接收到的原始字节，`tryParse()` 尝试从缓冲区头部取出一个完整 PDU。

```cpp
// 典型用法（在网络接收回调里）
parser_.feed(recv_buf, recv_len);

PDUHeader hdr;
std::vector<uint8_t> body;
while (parser_.tryParse(hdr, body)) {
    // hdr.message_type 已转换为主机字节序
    dispatch(hdr, body);
}
```

**`tryParse()` 内部流程：**

```
缓冲区字节数 < 16？
  └─ 是 → return false（等待半包）

读取头部，解析 total_length 和 body_length
  └─ 合法性校验失败（total < 16 或 body > 64MB）？
       └─ 清空缓冲区，return false（协议错误）

缓冲区字节数 < total_length？
  └─ 是 → return false（等待半包）

提取 header 和 body，erase 已消耗字节
  └─ return true（粘包场景下下次调用继续解析）
```

---

## 6.5 crypto_utils：密码哈希

### 6.5.1 安全模型

```
注册流程：
  客户端  →  hashForTransport(password)  →  传输哈希（防明文）
  服务端  →  hashPassword(transport_hash)  →  存入数据库（带 salt）

登录流程：
  客户端  →  hashForTransport(password)  →  发送给服务端
  服务端  →  verifyPassword(recv_hash, stored_hash)  →  验证
```

### 6.5.2 存储格式

`hashPassword()` 返回 `"salt:hash"` 格式：

```
salt  = 16 字节随机数的十六进制字符串（32 字符）
hash  = SHA-256(salt + password) 的十六进制字符串（64 字符）
```

每次调用生成不同的 salt，相同密码在数据库里存的哈希不同——这是防**彩虹表攻击**的关键。

### 6.5.3 接口

```cpp
// 注册时（服务端）：生成随机 salt，返回 "salt:hash"
std::string hashPassword(const std::string& password);

// 登录时（服务端）：从 stored_hash 中提取 salt，重算并对比
bool verifyPassword(const std::string& password,
                    const std::string& stored_hash);

// 传输前（客户端）：对密码做一次 SHA-256
std::string hashForTransport(const std::string& password);
```

### 6.5.4 OpenSSL EVP 接口

直接调用 `SHA256()` 在 OpenSSL 3.x 中已被标记为不推荐，改用更通用的 `EVP_DigestXxx` 系列：

```cpp
EVP_MD_CTX* ctx = EVP_MD_CTX_new();
EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);   // 指定算法
EVP_DigestUpdate(ctx, input.data(), input.size()); // 喂入数据
EVP_DigestFinal_ex(ctx, digest, &digestLen);       // 生成摘要
EVP_MD_CTX_free(ctx);                              // 必须释放
```

EVP 的好处：将来换成 SHA-3 或 BLAKE2 只需改 `EVP_sha256()` 那一行。

---

## 6.6 单元测试

```bash
cmake -B ~/cv-server-build -S /mnt/d/CloudVault/server -G Ninja
cmake --build ~/cv-server-build
ctest --test-dir ~/cv-server-build --output-on-failure
```

测试覆盖（17 个用例）：

**test_pdu.cpp（9 个）**

| 测试名 | 验证内容 |
|--------|---------|
| `PDUBuilder.EmptyBody` | PING 空 body，头部字段正确 |
| `PDUBuilder.WithMultipleFields` | 多字段 body 总长度正确 |
| `PDUBuilder.FixedStringShorter` | 短字符串零填充 |
| `PDUBuilder.FixedStringLonger` | 长字符串截断 |
| `PDUParser.ExactOnePacket` | 完整包一次解析 |
| `PDUParser.HalfPacket` | 半包分两次喂入 |
| `PDUParser.StickyPackets` | 两个包粘连，各自解析 |
| `PDUBuilderParser.RoundTrip` | Builder 构建 → Parser 解析，内容一致 |

**test_crypto.cpp（9 个）**

| 测试名 | 验证内容 |
|--------|---------|
| `Crypto.HashForTransportDeterministic` | 相同输入 → 相同输出 |
| `Crypto.HashForTransportLength` | SHA-256 输出 64 字符 |
| `Crypto.HashForTransportDifferentInputs` | 不同密码哈希不同 |
| `Crypto.HashPasswordFormat` | 输出包含 `:` 分隔符 |
| `Crypto.HashPasswordDifferentEachCall` | 相同密码两次哈希不同（随机 salt）|
| `Crypto.VerifyCorrectPassword` | 正确密码验证通过 |
| `Crypto.VerifyWrongPassword` | 错误密码验证失败 |
| `Crypto.VerifyEmptyPassword` | 空密码可正常处理 |
| `Crypto.VerifyInvalidFormat` | 非法 stored_hash 返回 false |

---

## 6.7 本章新知识点

- **粘包/半包**：TCP 流式特性，解决方案是 Header + Length 字段
- **网络字节序**：大端，`htonl` / `ntohl` 在主机和网络字节序之间转换
- **`#pragma pack(1)`**：关闭结构体字节对齐，保证内存布局与网络帧一致
- **`static_assert`**：编译期断言，确保结构体大小不意外变化
- **SHA-256 EVP 接口**：OpenSSL 推荐用法，比直接调用 `SHA256()` 更灵活
- **salt + hash**：防彩虹表攻击的标准做法，每次注册使用不同随机 salt
- **GoogleTest 基础**：`TEST(Suite, Case)`、`EXPECT_EQ`、`ASSERT_TRUE`

---

上一章：[第五章 — 两端骨架](ch05-skeleton.md) ｜ 下一章：[第七章 — PING/PONG 联通](ch07-ping-pong.md)
