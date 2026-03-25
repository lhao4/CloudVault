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

} // namespace

// =============================================================
// 构造函数
// =============================================================
AuthHandler::AuthHandler(Database& db, SessionManager& sessions)
    : users_(db), sessions_(sessions) {}

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
}

// =============================================================
// handleLogout()
// =============================================================
void AuthHandler::handleLogout(std::shared_ptr<TcpConnection> conn,
                                const PDUHeader& /*hdr*/,
                                const std::vector<uint8_t>& /*body*/)
{
    // 根据连接找到对应会话并移除
    sessions_.removeByConnection(conn);
    spdlog::info("LOGOUT from {}", conn->peerAddr());
}

} // namespace cloudvault
