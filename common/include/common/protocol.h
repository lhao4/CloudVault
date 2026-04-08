// =============================================================
// common/include/common/protocol.h
// PDU 协议定义：消息类型枚举 + 头部结构体
//
// PDU（Protocol Data Unit）格式：
//   ┌─────────────────────────────────────┐
//   │          PDU Header（16 字节）       │
//   │  total_length : uint32_t            │  整个 PDU 的字节数（含头）
//   │  body_length  : uint32_t            │  body 部分的字节数
//   │  message_type : uint32_t            │  MessageType 枚举值
//   │  reserved     : uint32_t            │  保留，填 0
//   ├─────────────────────────────────────┤
//   │          Body（变长）                │
//   │  各消息类型自定义格式                 │
//   └─────────────────────────────────────┘
//
// 字节序：所有多字节字段均使用网络字节序（大端）。
// =============================================================

#pragma once

#include <cstddef>
#include <cstdint>

namespace cloudvault {

// ── 消息类型 ──────────────────────────────────────────────
// 用 uint32_t 底层类型保证跨平台大小一致。
// 高字节表示功能分组，低字节表示具体消息。
enum class MessageType : uint32_t {
    // 连接保活（第七章）
    PING                = 0x0001,
    PONG                = 0x0002,

    // 认证（第八章）
    LOGIN_REQUEST       = 0x0101,
    LOGIN_RESPONSE      = 0x0102,
    REGISTER_REQUEST    = 0x0103,
    REGISTER_RESPONSE   = 0x0104,
    LOGOUT              = 0x0105,
    UPDATE_PROFILE_REQUEST  = 0x0106,
    UPDATE_PROFILE_RESPONSE = 0x0107,

    // 好友（第九章）
    FIND_USER_REQUEST   = 0x0201,
    FIND_USER_RESPONSE  = 0x0202,
    ADD_FRIEND_REQUEST  = 0x0203,
    ADD_FRIEND_RESPONSE = 0x0204,
    FRIEND_REQUEST_PUSH = 0x0205,
    AGREE_FRIEND_REQUEST = 0x0206,
    AGREE_FRIEND_RESPONSE = 0x0207,
    FRIEND_ADDED_PUSH   = 0x0208,
    FLUSH_FRIENDS_REQUEST = 0x0209,
    FLUSH_FRIENDS_RESPONSE = 0x020A,
    DELETE_FRIEND_REQUEST = 0x020B,
    DELETE_FRIEND_RESPONSE = 0x020C,
    FRIEND_DELETED_PUSH = 0x020D,
    ONLINE_USER_REQUEST     = 0x020E,
    ONLINE_USER_RESPONSE    = 0x020F,

    // 聊天（第十章）
    CHAT                = 0x0301,
    GROUP_CHAT          = 0x0302,
    GET_HISTORY         = 0x0303,

    // 文件管理（第十一章）
    MKDIR               = 0x0401,
    FLUSH_FILE          = 0x0402,
    MOVE                = 0x0403,
    DELETE_FILE         = 0x0404,
    RENAME              = 0x0405,
    SEARCH_FILE         = 0x0406,

    // 文件传输（第十二章）
    UPLOAD_REQUEST      = 0x0501,
    UPLOAD_DATA         = 0x0502,
    DOWNLOAD_REQUEST    = 0x0503,
    DOWNLOAD_DATA       = 0x0504,

    // 文件分享（第十三章）
    SHARE_REQUEST       = 0x0601,
    SHARE_NOTIFY        = 0x0602,
    SHARE_AGREE_REQUEST = 0x0603,
    SHARE_AGREE_RESPOND = 0x0604,

    // 群组管理
    CREATE_GROUP_REQUEST    = 0x0701,
    CREATE_GROUP_RESPONSE   = 0x0702,
    JOIN_GROUP_REQUEST      = 0x0703,
    JOIN_GROUP_RESPONSE     = 0x0704,
    LEAVE_GROUP_REQUEST     = 0x0705,
    LEAVE_GROUP_RESPONSE    = 0x0706,
    GET_GROUP_LIST_REQUEST  = 0x0707,
    GET_GROUP_LIST_RESPONSE = 0x0708,
};

// ── PDU 头部 ──────────────────────────────────────────────
// #pragma pack(push, 1)：关闭结构体字节对齐，保证编译器不会
// 在字段之间插入 padding，确保内存布局与网络帧完全一致。
#pragma pack(push, 1)
struct PDUHeader {
    uint32_t total_length;   // 整个 PDU 的总字节数（含 16 字节头）
    uint32_t body_length;    // body 部分的字节数
    uint32_t message_type;   // MessageType 值（网络字节序）
    uint32_t reserved;       // 保留字段，始终为 0
};
#pragma pack(pop)

// 编译期断言：确保头部恰好 16 字节
static_assert(sizeof(PDUHeader) == 16, "PDUHeader must be exactly 16 bytes");

// ── 协议常量 ──────────────────────────────────────────────
constexpr size_t PDU_HEADER_SIZE    = sizeof(PDUHeader);
constexpr size_t PDU_MAX_BODY_SIZE  = 64u * 1024u * 1024u;  // 64 MB 上限
constexpr size_t FILE_TRANSFER_CHUNK_SIZE = 4u * 1024u * 1024u;  // 4 MB

} // namespace cloudvault
