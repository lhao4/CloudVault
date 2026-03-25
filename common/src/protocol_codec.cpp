// =============================================================
// common/src/protocol_codec.cpp
// PDUBuilder 和 PDUParser 实现
// =============================================================

#include "common/protocol_codec.h"
#include "common/protocol.h"

#include <algorithm>
#include <cstring>

// 跨平台字节序转换函数
// htonl / ntohl：将 32 位整数在主机字节序和网络字节序（大端）之间转换
#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

namespace cloudvault {

// ── 内部工具：64 位网络字节序转换 ─────────────────────────
// C++ 标准库没有提供 htonll，需要手动实现。
// 原理：检测主机是否为小端（little-endian），若是则交换每个字节。
namespace {

uint64_t hton64(uint64_t val) {
    // 通过写入 1 并读取第一个字节来判断字节序
    // 小端系统：低字节在低地址，第一个字节为 1
    const uint32_t probe = 1;
    if (*reinterpret_cast<const uint8_t*>(&probe) == 0) {
        return val;  // 大端主机，无需转换
    }
    // 小端：逐字节翻转
    return ((val & 0xFF00000000000000ULL) >> 56)
         | ((val & 0x00FF000000000000ULL) >> 40)
         | ((val & 0x0000FF0000000000ULL) >> 24)
         | ((val & 0x000000FF00000000ULL) >>  8)
         | ((val & 0x00000000FF000000ULL) <<  8)
         | ((val & 0x0000000000FF0000ULL) << 24)
         | ((val & 0x000000000000FF00ULL) << 40)
         | ((val & 0x00000000000000FFULL) << 56);
}

} // namespace

// =============================================================
// PDUBuilder 实现
// =============================================================

PDUBuilder::PDUBuilder(MessageType type) : type_(type) {}

PDUBuilder& PDUBuilder::writeUInt8(uint8_t val) {
    body_.push_back(val);
    return *this;
}

PDUBuilder& PDUBuilder::writeUInt16(uint16_t val) {
    // htons：将 16 位整数转为网络字节序
    const uint16_t net = htons(val);
    const auto* p = reinterpret_cast<const uint8_t*>(&net);
    body_.insert(body_.end(), p, p + sizeof(uint16_t));
    return *this;
}

PDUBuilder& PDUBuilder::writeUInt32(uint32_t val) {
    const uint32_t net = htonl(val);
    const auto* p = reinterpret_cast<const uint8_t*>(&net);
    body_.insert(body_.end(), p, p + sizeof(uint32_t));
    return *this;
}

PDUBuilder& PDUBuilder::writeUInt64(uint64_t val) {
    const uint64_t net = hton64(val);
    const auto* p = reinterpret_cast<const uint8_t*>(&net);
    body_.insert(body_.end(), p, p + sizeof(uint64_t));
    return *this;
}

PDUBuilder& PDUBuilder::writeString(const std::string& s) {
    // 长度前缀（uint16）+ UTF-8 字节
    // 超过 65535 字节的部分截断
    const auto len = static_cast<uint16_t>(
        std::min(s.size(), static_cast<size_t>(0xFFFFu)));
    writeUInt16(len);
    body_.insert(body_.end(), s.begin(), s.begin() + len);
    return *this;
}

PDUBuilder& PDUBuilder::writeFixedString(const std::string& s, size_t maxLen) {
    // 写入前 min(s.size(), maxLen) 个字节，剩余部分补 0
    const size_t copyLen = std::min(s.size(), maxLen);
    body_.insert(body_.end(), s.begin(), s.begin() + copyLen);
    body_.insert(body_.end(), maxLen - copyLen, 0u);  // 零填充
    return *this;
}

PDUBuilder& PDUBuilder::writeBytes(const void* data, size_t len) {
    const auto* p = static_cast<const uint8_t*>(data);
    body_.insert(body_.end(), p, p + len);
    return *this;
}

std::vector<uint8_t> PDUBuilder::build() const {
    // 构造头部（所有字段均为网络字节序）
    PDUHeader hdr{};
    hdr.total_length = htonl(
        static_cast<uint32_t>(PDU_HEADER_SIZE + body_.size()));
    hdr.body_length  = htonl(static_cast<uint32_t>(body_.size()));
    hdr.message_type = htonl(static_cast<uint32_t>(type_));
    hdr.reserved     = 0;

    // 拼接 header + body
    std::vector<uint8_t> pdu(PDU_HEADER_SIZE + body_.size());
    std::memcpy(pdu.data(), &hdr, PDU_HEADER_SIZE);
    if (!body_.empty()) {
        std::memcpy(pdu.data() + PDU_HEADER_SIZE, body_.data(), body_.size());
    }
    return pdu;
}

// =============================================================
// PDUParser 实现
// =============================================================

void PDUParser::feed(const void* data, size_t len) {
    const auto* p = static_cast<const uint8_t*>(data);
    buffer_.insert(buffer_.end(), p, p + len);
}

bool PDUParser::tryParse(PDUHeader& header, std::vector<uint8_t>& body) {
    // 缓冲区不足头部大小：等待更多数据（半包情况）
    if (buffer_.size() < PDU_HEADER_SIZE) {
        return false;
    }

    // 读取头部（不消耗，只是 peek）
    PDUHeader raw{};
    std::memcpy(&raw, buffer_.data(), PDU_HEADER_SIZE);

    // 将网络字节序转为主机字节序
    const uint32_t totalLen = ntohl(raw.total_length);
    const uint32_t bodyLen  = ntohl(raw.body_length);
    const uint32_t msgType  = ntohl(raw.message_type);

    // 合法性校验：防止恶意/损坏的数据撑爆内存
    if (totalLen < PDU_HEADER_SIZE
     || bodyLen != totalLen - PDU_HEADER_SIZE
     || bodyLen > PDU_MAX_BODY_SIZE) {
        // 协议错误：清空缓冲区，让上层重置连接
        buffer_.clear();
        return false;
    }

    // 缓冲区中不足一个完整 PDU：等待更多数据（半包情况）
    if (buffer_.size() < totalLen) {
        return false;
    }

    // 数据完整，提取结果（转换为主机字节序供调用方使用）
    header.total_length  = totalLen;
    header.body_length   = bodyLen;
    header.message_type  = msgType;
    header.reserved      = ntohl(raw.reserved);

    body.assign(buffer_.begin() + PDU_HEADER_SIZE,
                buffer_.begin() + totalLen);

    // 消耗已解析的字节（支持粘包：缓冲区可能还有下一个 PDU）
    buffer_.erase(buffer_.begin(), buffer_.begin() + totalLen);
    return true;
}

void PDUParser::reset() {
    buffer_.clear();
}

} // namespace cloudvault
