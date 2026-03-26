// =============================================================
// client/src/network/share_service.cpp
// 第十三章：文件分享协议封装
// =============================================================

#include "network/share_service.h"

#include "common/protocol.h"
#include "common/protocol_codec.h"

#include <cstring>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

namespace cloudvault {

namespace {

constexpr uint8_t kStatusOk = 0;

bool readString(const std::vector<uint8_t>& body, size_t& offset, std::string& out) {
    if (offset + 2 > body.size()) {
        return false;
    }

    uint16_t len = 0;
    std::memcpy(&len, body.data() + offset, sizeof(len));
    len = ntohs(len);
    offset += 2;

    if (offset + len > body.size()) {
        return false;
    }

    out.assign(reinterpret_cast<const char*>(body.data() + offset), len);
    offset += len;
    return true;
}

bool readFixedString32(const std::vector<uint8_t>& body, size_t& offset, std::string& out) {
    constexpr size_t kFixedLen = 32;
    if (offset + kFixedLen > body.size()) {
        return false;
    }

    const char* data = reinterpret_cast<const char*>(body.data() + offset);
    size_t actual_len = 0;
    while (actual_len < kFixedLen && data[actual_len] != '\0') {
        ++actual_len;
    }
    out.assign(data, actual_len);
    offset += kFixedLen;
    return true;
}

QString readMessage(const std::vector<uint8_t>& body, size_t offset) {
    std::string message;
    if (readString(body, offset, message)) {
        return QString::fromStdString(message);
    }
    return QStringLiteral("服务器返回格式错误");
}

} // namespace

ShareService::ShareService(TcpClient& client,
                           ResponseRouter& router,
                           QObject* parent)
    : QObject(parent), client_(client), router_(router) {
    router_.registerHandler(
        MessageType::SHARE_REQUEST,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onShareResponse(hdr, body);
        });
    router_.registerHandler(
        MessageType::SHARE_NOTIFY,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onShareNotify(hdr, body);
        });
    router_.registerHandler(
        MessageType::SHARE_AGREE_RESPOND,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onShareAgreeResponse(hdr, body);
        });
}

void ShareService::shareFile(const QString& file_path, const QStringList& targets) {
    pending_share_path_ = file_path.trimmed();
    pending_share_targets_ = targets;

    auto pdu = PDUBuilder(MessageType::SHARE_REQUEST)
        .writeUInt32(static_cast<uint32_t>(targets.size()));
    for (const QString& target : targets) {
        pdu.writeFixedString(target.trimmed().toStdString(), 32);
    }
    pdu.writeString(pending_share_path_.toStdString());
    client_.send(pdu.build());
}

void ShareService::acceptShare(const QString& sender_name, const QString& source_path) {
    pending_accept_sender_ = sender_name.trimmed();
    pending_accept_path_ = source_path.trimmed();

    auto pdu = PDUBuilder(MessageType::SHARE_AGREE_REQUEST)
        .writeFixedString(pending_accept_sender_.toStdString(), 32)
        .writeString(pending_accept_path_.toStdString())
        .build();
    client_.send(std::move(pdu));
}

void ShareService::onShareResponse(const PDUHeader&,
                                   const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit shareRequestFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    const uint8_t status = body[0];
    const QString message = readMessage(body, 1);
    if (status == kStatusOk) {
        emit shareRequestSent(message);
    } else {
        emit shareRequestFailed(message);
    }
}

void ShareService::onShareNotify(const PDUHeader&,
                                 const std::vector<uint8_t>& body) {
    size_t offset = 0;
    std::string from_user;
    std::string file_path;
    if (!readFixedString32(body, offset, from_user) ||
        !readString(body, offset, file_path)) {
        return;
    }

    emit incomingShareRequest(QString::fromStdString(from_user),
                              QString::fromStdString(file_path));
}

void ShareService::onShareAgreeResponse(const PDUHeader&,
                                        const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit shareAcceptFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    const uint8_t status = body[0];
    const QString message = readMessage(body, 1);
    if (status == kStatusOk) {
        emit shareAccepted(message);
    } else {
        emit shareAcceptFailed(message);
    }
}

} // namespace cloudvault
