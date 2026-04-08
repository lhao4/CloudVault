// =============================================================
// server/src/handler/auth_handler.cpp
// 用户认证业务逻辑实现
//
// Body 格式（与客户端 AuthService 约定）：
//
// REGISTER_REQUEST:
//   username      : string  (uint16 长度前缀 + UTF-8)
//   password_hash : fixed64 (客户端 hashForTransport 的结果)
//
// REGISTER_RESPONSE:
//   status  : uint8   (0=成功 1=用户名已存在 2=服务器错误)
//   message : string
//
// LOGIN_REQUEST:
//   username      : string
//   password_hash : fixed64
//
// LOGIN_RESPONSE:
//   status  : uint8   (0=成功 1=密码错误 2=用户不存在 3=已在线 4=服务器错误)
//   user_id : uint32  (仅 status==0 时有效)
//   message : string
// =============================================================

#include "server/handler/auth_handler.h"
#include "server/tcp_connection.h"
#include "common/protocol_codec.h"
#include "common/crypto_utils.h"

#include <arpa/inet.h>
#include <spdlog/spdlog.h>

namespace cloudvault {

// ── 内部工具：从 body 中读取带长度前缀的字符串 ─────────────────
namespace {

// 从 body[offset] 读取 uint16_t（大端），推进 offset
// 失败返回 false
bool readUInt16(const std::vector<uint8_t>& body, size_t& offset, uint16_t& out) {
    if (offset + 2 > body.size()) return false;
    uint16_t net;
    memcpy(&net, body.data() + offset, 2);
    out = ntohs(net);
    offset += 2;
    return true;
}

// 读取 len 字节字符串
bool readString(const std::vector<uint8_t>& body, size_t& offset,
                uint16_t len, std::string& out) {
    if (offset + len > body.size()) return false;
    out.assign(reinterpret_cast<const char*>(body.data() + offset), len);
    offset += len;
    return true;
}

// 读取固定 64 字节（password_hash）
bool readFixed64(const std::vector<uint8_t>& body, size_t& offset, std::string& out) {
    if (offset + 64 > body.size()) return false;
    out.assign(reinterpret_cast<const char*>(body.data() + offset), 64);
    offset += 64;
    return true;
}

// 构造并发送响应 PDU
void sendRegisterResponse(std::shared_ptr<TcpConnection> conn,
                          uint8_t status, const std::string& msg) {
    auto pdu = PDUBuilder(MessageType::REGISTER_RESPONSE)
        .writeUInt8(status)
        .writeString(msg)
        .build();
    conn->send(std::move(pdu));
}

void sendLoginResponse(std::shared_ptr<TcpConnection> conn,
                       uint8_t status, uint32_t user_id, const std::string& msg) {
    auto pdu = PDUBuilder(MessageType::LOGIN_RESPONSE)
        .writeUInt8(status)
        .writeUInt32(user_id)
        .writeString(msg)
        .build();
    conn->send(std::move(pdu));
}

std::vector<uint8_t> buildChatPush(const std::string& from,
                                   const std::string& to,
                                   const std::string& content,
                                   const std::string& created_at) {
    return PDUBuilder(MessageType::CHAT)
        .writeString(from)
        .writeString(to)
        .writeString(content)
        .writeString(created_at)
        .build();
}

std::vector<uint8_t> buildGroupChatPush(const std::string& from,
                                        int group_id,
                                        const std::string& content,
                                        const std::string& created_at) {
    return PDUBuilder(MessageType::GROUP_CHAT)
        .writeUInt32(static_cast<uint32_t>(group_id))
        .writeString(from)
        .writeString(content)
        .writeString(created_at)
        .build();
}

bool decodeOfflineGroupPayload(const std::string& payload,
                               int& group_id,
                               std::string& content) {
    const auto pos = payload.find('\n');
    if (pos == std::string::npos) {
        return false;
    }
    try {
        group_id = std::stoi(payload.substr(0, pos));
    } catch (...) {
        return false;
    }
    content = payload.substr(pos + 1);
    return group_id > 0;
}

} // namespace

// =============================================================
// 构造函数
// =============================================================
AuthHandler::AuthHandler(Database& db, SessionManager& sessions)
    : users_(db), messages_(db), sessions_(sessions) {}

// =============================================================
// handleRegister()
// =============================================================
void AuthHandler::handleRegister(std::shared_ptr<TcpConnection> conn,
                                  const PDUHeader& /*hdr*/,
                                  const std::vector<uint8_t>& body)
{
    // ── 解析 body ──────────────────────────────────────────
    size_t   offset = 0;
    uint16_t ulen   = 0;
    std::string username, password_hash;

    if (!readUInt16(body, offset, ulen) ||
        !readString(body, offset, ulen, username) ||
        !readFixed64(body, offset, password_hash)) {
        spdlog::warn("handleRegister: malformed body from {}", conn->peerAddr());
        sendRegisterResponse(conn, 2, "请求格式错误");
        return;
    }

    spdlog::info("REGISTER request: username={} from {}", username, conn->peerAddr());

    // ── 服务端对传输哈希再次加 salt 存储 ────────────────────
    const std::string stored_hash = crypto::hashPassword(password_hash);

    // ── 写入数据库 ──────────────────────────────────────────
    int user_id = users_.insertUser(username, stored_hash);

    if (user_id == -1) {
        spdlog::info("REGISTER failed: username '{}' already exists", username);
        sendRegisterResponse(conn, 1, "用户名已被占用");
        return;
    }

    if (user_id == 0) {
        spdlog::error("REGISTER failed: DB error for username='{}'", username);
        sendRegisterResponse(conn, 2, "服务器内部错误");
        return;
    }

    spdlog::info("REGISTER success: username={} uid={}", username, user_id);
    sendRegisterResponse(conn, 0, "注册成功");
}

// =============================================================
// handleLogin()
// =============================================================
void AuthHandler::handleLogin(std::shared_ptr<TcpConnection> conn,
                               const PDUHeader& /*hdr*/,
                               const std::vector<uint8_t>& body)
{
    // ── 解析 body ──────────────────────────────────────────
    size_t   offset = 0;
    uint16_t ulen   = 0;
    std::string username, password_hash;

    if (!readUInt16(body, offset, ulen) ||
        !readString(body, offset, ulen, username) ||
        !readFixed64(body, offset, password_hash)) {
        spdlog::warn("handleLogin: malformed body from {}", conn->peerAddr());
        sendLoginResponse(conn, 4, 0, "请求格式错误");
        return;
    }

    spdlog::info("LOGIN request: username={} from {}", username, conn->peerAddr());

    // ── 查询用户 ────────────────────────────────────────────
    auto user_opt = users_.findByName(username);
    if (!user_opt) {
        spdlog::info("LOGIN failed: user '{}' not found", username);
        sendLoginResponse(conn, 2, 0, "用户名不存在");
        return;
    }

    // ── 验证密码 ────────────────────────────────────────────
    if (!crypto::verifyPassword(password_hash, user_opt->password_hash)) {
        spdlog::info("LOGIN failed: wrong password for '{}'", username);
        sendLoginResponse(conn, 1, 0, "密码错误");
        return;
    }

    // ── 防重复登录 ──────────────────────────────────────────
    if (sessions_.isOnline(username)) {
        spdlog::info("LOGIN failed: '{}' already online", username);
        sendLoginResponse(conn, 3, 0, "该账号已在其他设备登录");
        return;
    }

    // ── 注册会话 ────────────────────────────────────────────
    sessions_.addSession(user_opt->user_id, username, conn);

    // 连接断开时自动移除会话并更新在线状态
    const int uid = user_opt->user_id;
    conn->setCloseCallback([this, uid, username](std::shared_ptr<TcpConnection>) {
        sessions_.removeSession(uid);
        users_.setOnline(uid, false);
        spdlog::info("Session ended: {} (uid={})", username, uid);
    });

    users_.setOnline(uid, true);

    spdlog::info("LOGIN success: username={} uid={}", username, uid);
    sendLoginResponse(conn, 0, static_cast<uint32_t>(uid), "登录成功");
    deliverOfflineMessages(conn, uid, username);
}

// =============================================================
// handleLogout()
// =============================================================
void AuthHandler::handleLogout(std::shared_ptr<TcpConnection> conn,
                                const PDUHeader& /*hdr*/,
                                const std::vector<uint8_t>& /*body*/)
{
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        spdlog::info("LOGOUT ignored for unauthenticated peer {}", conn->peerAddr());
        return;
    }

    sessions_.removeSession(current->user_id);
    users_.setOnline(current->user_id, false);
    spdlog::info("LOGOUT success: {} (uid={})", current->username, current->user_id);
}

// =============================================================
// handleUpdateProfile()
// Body: nickname(string) + signature(string)
// Response: status(uint8) + message(string)
// =============================================================
void AuthHandler::handleUpdateProfile(std::shared_ptr<TcpConnection> conn,
                                       const PDUHeader& /*hdr*/,
                                       const std::vector<uint8_t>& body)
{
    const auto session = sessions_.findByConnection(conn);
    if (!session) {
        auto pdu = PDUBuilder(MessageType::UPDATE_PROFILE_RESPONSE)
            .writeUInt8(1)
            .writeString("未登录")
            .build();
        conn->send(std::move(pdu));
        return;
    }

    size_t   offset = 0;
    uint16_t nick_len = 0, sig_len = 0;
    std::string nickname, signature;

    if (!readUInt16(body, offset, nick_len) ||
        !readString(body, offset, nick_len, nickname) ||
        !readUInt16(body, offset, sig_len) ||
        !readString(body, offset, sig_len, signature)) {
        auto pdu = PDUBuilder(MessageType::UPDATE_PROFILE_RESPONSE)
            .writeUInt8(2)
            .writeString("请求格式错误")
            .build();
        conn->send(std::move(pdu));
        return;
    }

    bool ok = users_.updateProfile(session->user_id, nickname, signature);

    uint8_t status = ok ? 0 : 3;
    std::string msg = ok ? "更新成功" : "更新失败";

    auto pdu = PDUBuilder(MessageType::UPDATE_PROFILE_RESPONSE)
        .writeUInt8(status)
        .writeString(msg)
        .build();
    conn->send(std::move(pdu));

    if (ok) {
        spdlog::info("Profile updated: {} nick='{}' sig='{}'",
                     session->username, nickname, signature);
    }
}

void AuthHandler::deliverOfflineMessages(std::shared_ptr<TcpConnection> conn,
                                         int user_id,
                                         const std::string& username) {
    const auto offline_messages = messages_.fetchUndeliveredMessages(user_id);
    if (offline_messages.empty()) {
        return;
    }

    for (const auto& item : offline_messages) {
        if (item.msg_type == static_cast<uint32_t>(MessageType::CHAT)) {
            conn->send(buildChatPush(item.sender_username,
                                     username,
                                     item.content,
                                     item.created_at));
        } else if (item.msg_type == static_cast<uint32_t>(MessageType::GROUP_CHAT)) {
            int group_id = 0;
            std::string content;
            if (!decodeOfflineGroupPayload(item.content, group_id, content)) {
                continue;
            }
            conn->send(buildGroupChatPush(item.sender_username,
                                          group_id,
                                          content,
                                          item.created_at));
        }
    }

    if (!messages_.markMessagesDelivered(user_id)) {
        spdlog::warn("Failed to mark offline chat messages delivered for {}", username);
        return;
    }

    spdlog::info("Delivered {} offline chat messages to {}",
                 offline_messages.size(), username);
}

} // namespace cloudvault
