# 第六章　协议基础——PDU 与 common 层

> **状态**：🔲 待写
>
> **本章目标**：实现 `common` 静态库中的协议编解码和密码哈希，并通过单元测试。
>
> **验收标准**：`ctest` 所有测试用例通过。

---

## 本章大纲

### 6.1 为什么需要自定义协议

- TCP 是流式协议，没有"消息边界"的概念
- 连续发送两条消息，接收方可能一次性收到，也可能分多次收到
- 这就是**粘包**和**半包**问题

**示例**：

```
发送方连续发送：[消息A][消息B]
接收方可能收到：[消息A+消息B]     ← 粘包
                [消息A的一半]     ← 半包
                [消息A的一半][消息A的另一半+消息B]
```

解决方案：在每条消息前加固定长度的**头部**，头部里写明消息总长度。

### 6.2 PDU v2 格式

```
┌─────────────────────────────────────┐
│          PDU Header（16 字节）       │
│  total_length : uint32_t            │
│  body_length  : uint32_t            │
│  message_type : uint32_t            │
│  reserved     : uint32_t            │
├─────────────────────────────────────┤
│          Body（变长）                │
│  各消息类型自定义格式                 │
└─────────────────────────────────────┘
```

### 6.3 PDUBuilder 实现（待完成）

链式 API，逐字段写入 body：

```cpp
auto pdu = PDUBuilder(MessageType::LoginRequest)
    .writeFixedString(username, 32)
    .writeFixedString(password_hash, 64)
    .build();
```

### 6.4 PDUParser 实现（待完成）

持续接收字节流，检测完整包：

```cpp
parser_.feed(data, len);
PDUHeader header;
std::vector<uint8_t> body;
while (parser_.tryParse(header, body)) {
    // 处理一个完整的包
}
```

### 6.5 crypto_utils 实现（待完成）

```cpp
// 注册时：生成 salt + 哈希
std::string hash = crypto::hashPassword("my_password");

// 登录时：验证
bool ok = crypto::verifyPassword("my_password", stored_hash);
```

### 6.6 单元测试（待完成）

- `test_pdu.cpp`：模拟粘包、半包、超大包场景
- `test_crypto.cpp`：哈希一致性与验证失败场景

### 6.7 本章新知识点

- 字节序（大端/小端）
- `#pragma pack` 的作用
- GoogleTest 基础用法

---

上一章：[第五章 — 两端骨架](ch05-skeleton.md) ｜ 下一章：[第七章 — PING/PONG 联通](ch07-ping-pong.md)
