// =============================================================
// common/tests/test_pdu.cpp
// PDUBuilder 和 PDUParser 单元测试
// =============================================================

#include "common/protocol.h"
#include "common/protocol_codec.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>  // ntohl, ntohs
#include <cstring>      // std::memcpy

using namespace cloudvault;

// ── PDUBuilder 测试 ───────────────────────────────────────

// 构建一个空 body 的 PDU（如 PING），验证头部字段正确
TEST(PDUBuilder, EmptyBody) {
    PDUBuilder builder(MessageType::PING);
    const auto pdu = builder.build();

    // 空 body：总长度 = 头部 16 字节
    ASSERT_EQ(pdu.size(), PDU_HEADER_SIZE);

    // 解析头部验证字段（网络字节序 → 主机字节序）
    PDUHeader hdr{};
    std::memcpy(&hdr, pdu.data(), PDU_HEADER_SIZE);
    EXPECT_EQ(ntohl(hdr.total_length),
              static_cast<uint32_t>(PDU_HEADER_SIZE));
    EXPECT_EQ(ntohl(hdr.body_length),  0u);
    EXPECT_EQ(ntohl(hdr.message_type),
              static_cast<uint32_t>(MessageType::PING));
    EXPECT_EQ(ntohl(hdr.reserved), 0u);
}

// 构建带各种字段的 PDU，验证总长度计算正确
TEST(PDUBuilder, WithMultipleFields) {
    const std::string username = "alice";
    const uint32_t    session  = 42;

    auto pdu = PDUBuilder(MessageType::LOGIN_REQUEST)
        .writeString(username)    // 2 (len) + 5 (bytes) = 7
        .writeUInt32(session)     // 4
        .build();

    // body = 7 + 4 = 11 字节
    EXPECT_EQ(pdu.size(), PDU_HEADER_SIZE + 11u);
}

// writeFixedString 测试：截断和零填充
TEST(PDUBuilder, FixedStringShorter) {
    // "hi" 写入长度 8 的字段 → "hi" + 6 个零字节
    auto pdu = PDUBuilder(MessageType::PING)
        .writeFixedString("hi", 8)
        .build();
    EXPECT_EQ(pdu.size(), PDU_HEADER_SIZE + 8u);

    // 验证后 6 字节为 0
    for (size_t i = PDU_HEADER_SIZE + 2; i < PDU_HEADER_SIZE + 8; ++i) {
        EXPECT_EQ(pdu[i], 0u);
    }
}

TEST(PDUBuilder, FixedStringLonger) {
    // "hello world" (11 字节) 写入长度 5 的字段 → 截断为 "hello"
    auto pdu = PDUBuilder(MessageType::PING)
        .writeFixedString("hello world", 5)
        .build();
    EXPECT_EQ(pdu.size(), PDU_HEADER_SIZE + 5u);
    // 验证内容是 "hello"
    EXPECT_EQ(pdu[PDU_HEADER_SIZE + 0], 'h');
    EXPECT_EQ(pdu[PDU_HEADER_SIZE + 4], 'o');
}

// ── PDUParser 测试 ────────────────────────────────────────

// 喂入一个完整的 PDU，tryParse 返回 true
TEST(PDUParser, ExactOnePacket) {
    auto pdu = PDUBuilder(MessageType::PONG).build();

    PDUParser parser;
    parser.feed(pdu.data(), pdu.size());

    PDUHeader hdr{};
    std::vector<uint8_t> body;
    ASSERT_TRUE(parser.tryParse(hdr, body));

    EXPECT_EQ(hdr.message_type,
              static_cast<uint32_t>(MessageType::PONG));
    EXPECT_EQ(hdr.body_length, 0u);
    EXPECT_TRUE(body.empty());

    // 没有第二个包
    EXPECT_FALSE(parser.tryParse(hdr, body));
}

// 半包：分两次喂入，第一次不完整，第二次才能解析
TEST(PDUParser, HalfPacket) {
    auto pdu = PDUBuilder(MessageType::PING)
        .writeUInt32(0xDEADBEEF)
        .build();

    PDUParser parser;
    PDUHeader hdr{};
    std::vector<uint8_t> body;

    // 只喂入前半部分
    const size_t half = pdu.size() / 2;
    parser.feed(pdu.data(), half);
    EXPECT_FALSE(parser.tryParse(hdr, body));  // 数据不完整

    // 喂入剩余部分
    parser.feed(pdu.data() + half, pdu.size() - half);
    ASSERT_TRUE(parser.tryParse(hdr, body));   // 现在可以解析了

    EXPECT_EQ(hdr.message_type,
              static_cast<uint32_t>(MessageType::PING));
    EXPECT_EQ(hdr.body_length, 4u);
}

// 粘包：一次喂入两个完整 PDU，应能各自独立解析出来
TEST(PDUParser, StickyPackets) {
    auto pdu1 = PDUBuilder(MessageType::PING).build();
    auto pdu2 = PDUBuilder(MessageType::PONG).build();

    // 拼成一段连续字节（模拟粘包）
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), pdu1.begin(), pdu1.end());
    combined.insert(combined.end(), pdu2.begin(), pdu2.end());

    PDUParser parser;
    parser.feed(combined.data(), combined.size());

    PDUHeader hdr{};
    std::vector<uint8_t> body;

    ASSERT_TRUE(parser.tryParse(hdr, body));
    EXPECT_EQ(hdr.message_type,
              static_cast<uint32_t>(MessageType::PING));

    ASSERT_TRUE(parser.tryParse(hdr, body));
    EXPECT_EQ(hdr.message_type,
              static_cast<uint32_t>(MessageType::PONG));

    EXPECT_FALSE(parser.tryParse(hdr, body));  // 没有更多数据
}

// 端到端：Builder 构建的 PDU 经 Parser 解析后内容一致
TEST(PDUBuilderParser, RoundTrip) {
    const std::string username = "cloudvault_user";
    const uint32_t    code     = 200;

    auto pdu = PDUBuilder(MessageType::LOGIN_RESPONSE)
        .writeUInt32(code)
        .writeString(username)
        .build();

    PDUParser parser;
    parser.feed(pdu.data(), pdu.size());

    PDUHeader hdr{};
    std::vector<uint8_t> body;
    ASSERT_TRUE(parser.tryParse(hdr, body));

    EXPECT_EQ(hdr.message_type,
              static_cast<uint32_t>(MessageType::LOGIN_RESPONSE));

    // 验证 body：前 4 字节是 code（大端），后面是字符串（长度前缀 + 内容）
    ASSERT_GE(body.size(), 4u);
    uint32_t parsedCode{};
    std::memcpy(&parsedCode, body.data(), 4);
    EXPECT_EQ(ntohl(parsedCode), code);

    // 字符串：uint16 长度 + 内容
    ASSERT_GE(body.size(), 4u + 2u + username.size());
    uint16_t strLen{};
    std::memcpy(&strLen, body.data() + 4, 2);
    EXPECT_EQ(ntohs(strLen), static_cast<uint16_t>(username.size()));

    const std::string parsed(
        reinterpret_cast<const char*>(body.data() + 6),
        username.size());
    EXPECT_EQ(parsed, username);
}
