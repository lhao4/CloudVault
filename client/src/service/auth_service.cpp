// =============================================================
// client/src/service/auth_service.cpp
// 认证服务实现
// =============================================================

#include "auth_service.h"

#include "common/protocol_codec.h"

#include <QLoggingCategory>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

Q_LOGGING_CATEGORY(lcAuth, "network.auth")

namespace cloudvault {

AuthService::AuthService(TcpClient& client,
                         ResponseRouter& router,
                         QObject* parent)
    : QObject(parent), client_(client), router_(router)
{
    // 注册 PDU 响应处理器
    router_.registerHandler(
        MessageType::REGISTER_RESPONSE,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onRegisterResponse(hdr, body);
        });

    router_.registerHandler(
        MessageType::LOGIN_RESPONSE,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onLoginResponse(hdr, body);
        });

    router_.registerHandler(
        MessageType::UPDATE_PROFILE_RESPONSE,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onUpdateProfileResponse(hdr, body);
        });
}

// =============================================================
// registerUser()：构建 REGISTER_REQUEST PDU 并发送
// =============================================================
void AuthService::registerUser(const QString& username, const QString& password) {
    const std::string uname = username.toStdString();
    // 客户端：对明文密码做一次 SHA-256 再传输（防明文）
    const std::string phash = crypto::hashForTransport(password.toStdString());

    auto pdu = PDUBuilder(MessageType::REGISTER_REQUEST)
        .writeString(uname)
        .writeFixedString(phash, 64)
        .build();

    qCDebug(lcAuth) << "Sending REGISTER_REQUEST for" << username;
    client_.send(std::move(pdu));
}

// =============================================================
// login()：构建 LOGIN_REQUEST PDU 并发送
// =============================================================
void AuthService::login(const QString& username, const QString& password) {
    const std::string uname = username.toStdString();
    const std::string phash = crypto::hashForTransport(password.toStdString());

    auto pdu = PDUBuilder(MessageType::LOGIN_REQUEST)
        .writeString(uname)
        .writeFixedString(phash, 64)
        .build();

    qCDebug(lcAuth) << "Sending LOGIN_REQUEST for" << username;
    client_.send(std::move(pdu));
}

void AuthService::logout() {
    qCDebug(lcAuth) << "Sending LOGOUT";
    client_.send(PDUBuilder(MessageType::LOGOUT).build());
}

void AuthService::updateProfile(const QString& nickname, const QString& signature) {
    auto pdu = PDUBuilder(MessageType::UPDATE_PROFILE_REQUEST)
        .writeString(nickname.trimmed().toStdString())
        .writeString(signature.trimmed().toStdString())
        .build();
    client_.send(std::move(pdu));
}

// =============================================================
// onRegisterResponse()：解析注册响应
// Body: status(uint8) + message(string)
// =============================================================
void AuthService::onRegisterResponse(const PDUHeader& /*hdr*/,
                                      const std::vector<uint8_t>& body)
{
    if (body.empty()) {
        emit registerFailed("服务器返回空响应");
        return;
    }

    size_t offset = 0;
    uint8_t status = body[offset++];

    // 读取 message 字符串（uint16 长度前缀）
    std::string message;
    if (offset + 2 <= body.size()) {
        uint16_t mlen;
        memcpy(&mlen, body.data() + offset, 2);
        mlen = ntohs(mlen);
        offset += 2;
        if (offset + mlen <= body.size()) {
            message.assign(reinterpret_cast<const char*>(body.data() + offset), mlen);
        }
    }

    qCDebug(lcAuth) << "REGISTER_RESPONSE: status=" << status
                    << "msg=" << QString::fromStdString(message);

    if (status == 0) {
        emit registerSuccess();
    } else {
        emit registerFailed(QString::fromStdString(message));
    }
}

// =============================================================
// onLoginResponse()：解析登录响应
// Body: status(uint8) + user_id(uint32) + message(string)
// =============================================================
void AuthService::onLoginResponse(const PDUHeader& /*hdr*/,
                                   const std::vector<uint8_t>& body)
{
    if (body.size() < 5) {  // 至少 1 + 4 字节
        emit loginFailed("服务器返回空响应");
        return;
    }

    size_t offset = 0;
    uint8_t status = body[offset++];

    uint32_t user_id_net;
    memcpy(&user_id_net, body.data() + offset, 4);
    int user_id = static_cast<int>(ntohl(user_id_net));
    offset += 4;

    std::string message;
    if (offset + 2 <= body.size()) {
        uint16_t mlen;
        memcpy(&mlen, body.data() + offset, 2);
        mlen = ntohs(mlen);
        offset += 2;
        if (offset + mlen <= body.size()) {
            message.assign(reinterpret_cast<const char*>(body.data() + offset), mlen);
        }
    }

    qCDebug(lcAuth) << "LOGIN_RESPONSE: status=" << status
                    << "uid=" << user_id
                    << "msg=" << QString::fromStdString(message);

    if (status == 0) {
        emit loginSuccess(user_id);
    } else {
        emit loginFailed(QString::fromStdString(message));
    }
}

void AuthService::onUpdateProfileResponse(const PDUHeader&,
                                          const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit profileUpdateFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];

    std::string message;
    if (offset + 2 <= body.size()) {
        uint16_t mlen = 0;
        std::memcpy(&mlen, body.data() + offset, 2);
        mlen = ntohs(mlen);
        offset += 2;
        if (offset + mlen <= body.size()) {
            message.assign(reinterpret_cast<const char*>(body.data() + offset), mlen);
        }
    }

    if (status == 0) {
        emit profileUpdateSuccess();
    } else {
        emit profileUpdateFailed(QString::fromStdString(message));
    }
}

} // namespace cloudvault
