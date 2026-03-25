// =============================================================
// common/include/common/protocol_codec.h
// PDUBuilder：链式 API 构建二进制 PDU
// PDUParser ：流式解析，自动处理粘包 / 半包
// =============================================================

#pragma once

#include "common/protocol.h"

#include <cstdint>
#include <string>
#include <vector>

namespace cloudvault {

// ── PDUBuilder ────────────────────────────────────────────
// 用法（链式调用）：
//
//   auto pdu = PDUBuilder(MessageType::LoginRequest)
//       .writeString(username)
//       .writeFixedString(password_hash, 64)
//       .build();
//
// build() 返回完整的 PDU 字节序列（header + body），
// 所有多字节字段均以网络字节序（大端）写入。
class PDUBuilder {
public:
    explicit PDUBuilder(MessageType type);

    // 写入各种基本类型（网络字节序）
    PDUBuilder& writeUInt8 (uint8_t  val);
    PDUBuilder& writeUInt16(uint16_t val);
    PDUBuilder& writeUInt32(uint32_t val);
    PDUBuilder& writeUInt64(uint64_t val);

    // 长度前缀字符串：写入 uint16 长度 + UTF-8 字节
    // 若字符串超过 65535 字节，截断至 65535
    PDUBuilder& writeString(const std::string& s);

    // 固定宽度字段：写入前 maxLen 字节，不足则补 0
    // 常用于固定长度的用户名、哈希值等字段
    PDUBuilder& writeFixedString(const std::string& s, size_t maxLen);

    // 写入任意字节块
    PDUBuilder& writeBytes(const void* data, size_t len);

    // 生成完整 PDU = 16 字节头 + body
    std::vector<uint8_t> build() const;

private:
    MessageType           type_;
    std::vector<uint8_t>  body_;
};

// ── PDUParser ─────────────────────────────────────────────
// TCP 是流式协议，数据可能：
//   粘包：一次 recv 收到多个完整 PDU
//   半包：一次 recv 只收到一个 PDU 的一部分
//
// PDUParser 维护一个内部缓冲区，将接收到的字节持续喂入，
// 每次调用 tryParse() 尝试取出一个完整的 PDU。
//
// 用法：
//   parser.feed(recv_buf, recv_len);
//   PDUHeader hdr;
//   std::vector<uint8_t> body;
//   while (parser.tryParse(hdr, body)) {
//       dispatch(hdr, body);   // hdr 已转换为主机字节序
//   }
class PDUParser {
public:
    // 将接收到的原始字节追加到内部缓冲区
    void feed(const void* data, size_t len);

    // 若缓冲区内有完整的 PDU，填充 header（主机字节序）和 body，
    // 消耗对应字节，返回 true；否则返回 false（等待更多数据）。
    bool tryParse(PDUHeader& header, std::vector<uint8_t>& body);

    // 清空缓冲区（连接断开时调用）
    void reset();

private:
    std::vector<uint8_t> buffer_;
};

} // namespace cloudvault
