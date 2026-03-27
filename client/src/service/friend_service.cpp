// =============================================================
// client/src/service/friend_service.cpp
// 好友功能协议封装
// =============================================================

#include "friend_service.h"

#include "common/protocol_codec.h"

#include <QLoggingCategory>
#include <cstring>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

Q_LOGGING_CATEGORY(lcFriend, "network.friend")

namespace cloudvault {

namespace {

constexpr uint8_t kStatusOk = 0;

bool readString(const std::vector<uint8_t>& body, size_t& offset, std::string& out) {
    if (offset + 2 > body.size()) return false;

    uint16_t len = 0;
    std::memcpy(&len, body.data() + offset, sizeof(len));
    len = ntohs(len);
    offset += 2;

    if (offset + len > body.size()) return false;
    out.assign(reinterpret_cast<const char*>(body.data() + offset), len);
    offset += len;
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

FriendService::FriendService(TcpClient& client,
                             ResponseRouter& router,
                             QObject* parent)
    : QObject(parent), client_(client), router_(router) {
    router_.registerHandler(
        MessageType::FIND_USER_RESPONSE,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onFindUserResponse(hdr, body);
        });
    router_.registerHandler(
        MessageType::ADD_FRIEND_RESPONSE,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onAddFriendResponse(hdr, body);
        });
    router_.registerHandler(
        MessageType::FRIEND_REQUEST_PUSH,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onFriendRequestPush(hdr, body);
        });
    router_.registerHandler(
        MessageType::AGREE_FRIEND_RESPONSE,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onAgreeFriendResponse(hdr, body);
        });
    router_.registerHandler(
        MessageType::FRIEND_ADDED_PUSH,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onFriendAddedPush(hdr, body);
        });
    router_.registerHandler(
        MessageType::FLUSH_FRIENDS_RESPONSE,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onFlushFriendsResponse(hdr, body);
        });
    router_.registerHandler(
        MessageType::DELETE_FRIEND_RESPONSE,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onDeleteFriendResponse(hdr, body);
        });
    router_.registerHandler(
        MessageType::FRIEND_DELETED_PUSH,
        [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
            onFriendDeletedPush(hdr, body);
        });
}

void FriendService::findUser(const QString& target) {
    pending_find_target_ = target.trimmed();
    auto pdu = PDUBuilder(MessageType::FIND_USER_REQUEST)
        .writeString(pending_find_target_.toStdString())
        .build();
    client_.send(std::move(pdu));
}

void FriendService::addFriend(const QString& target) {
    pending_add_target_ = target.trimmed();
    auto pdu = PDUBuilder(MessageType::ADD_FRIEND_REQUEST)
        .writeString(pending_add_target_.toStdString())
        .build();
    client_.send(std::move(pdu));
}

void FriendService::agreeFriend(const QString& requester) {
    pending_agree_target_ = requester.trimmed();
    auto pdu = PDUBuilder(MessageType::AGREE_FRIEND_REQUEST)
        .writeString(pending_agree_target_.toStdString())
        .build();
    client_.send(std::move(pdu));
}

void FriendService::flushFriends() {
    client_.send(PDUBuilder(MessageType::FLUSH_FRIENDS_REQUEST).build());
}

void FriendService::deleteFriend(const QString& target) {
    pending_delete_target_ = target.trimmed();
    auto pdu = PDUBuilder(MessageType::DELETE_FRIEND_REQUEST)
        .writeString(pending_delete_target_.toStdString())
        .build();
    client_.send(std::move(pdu));
}

void FriendService::onFindUserResponse(const PDUHeader&,
                                       const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit userNotFound(pending_find_target_);
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    if (status != kStatusOk) {
        emit userNotFound(pending_find_target_);
        return;
    }

    std::string username;
    if (!readString(body, offset, username) || offset >= body.size()) {
        emit userNotFound(pending_find_target_);
        return;
    }

    emit userFound(QString::fromStdString(username), body[offset] != 0);
}

void FriendService::onAddFriendResponse(const PDUHeader&,
                                        const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit friendAddFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    const uint8_t status = body[0];
    const QString message = readMessage(body, 1);
    if (status == kStatusOk) {
        emit friendRequestSent(pending_add_target_);
    } else {
        emit friendAddFailed(message);
    }
}

void FriendService::onFriendRequestPush(const PDUHeader&,
                                        const std::vector<uint8_t>& body) {
    size_t offset = 0;
    std::string username;
    if (!readString(body, offset, username)) return;
    emit incomingFriendRequest(QString::fromStdString(username));
}

void FriendService::onAgreeFriendResponse(const PDUHeader&,
                                          const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit friendAddFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    const uint8_t status = body[0];
    const QString message = readMessage(body, 1);
    if (status == kStatusOk) {
        emit friendAdded(pending_agree_target_);
    } else {
        emit friendAddFailed(message);
    }
}

void FriendService::onFriendAddedPush(const PDUHeader&,
                                      const std::vector<uint8_t>& body) {
    size_t offset = 0;
    std::string username;
    if (!readString(body, offset, username)) return;
    emit friendAdded(QString::fromStdString(username));
}

void FriendService::onFlushFriendsResponse(const PDUHeader&,
                                           const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit friendListRefreshFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    if (status != kStatusOk) {
        emit friendListRefreshFailed(readMessage(body, offset));
        return;
    }

    if (offset + 2 > body.size()) {
        emit friendListRefreshFailed(QStringLiteral("好友列表响应格式错误"));
        return;
    }

    uint16_t count = 0;
    std::memcpy(&count, body.data() + offset, sizeof(count));
    count = ntohs(count);
    offset += 2;

    QList<QPair<QString, bool>> friends;
    for (uint16_t i = 0; i < count; ++i) {
        std::string username;
        if (!readString(body, offset, username) || offset >= body.size()) {
            emit friendListRefreshFailed(QStringLiteral("好友列表响应格式错误"));
            return;
        }
        friends.append(qMakePair(QString::fromStdString(username), body[offset++] != 0));
    }

    emit friendsRefreshed(friends);
}

void FriendService::onDeleteFriendResponse(const PDUHeader&,
                                           const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit friendDeleteFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    const uint8_t status = body[0];
    const QString message = readMessage(body, 1);
    if (status == kStatusOk) {
        emit friendDeleted(pending_delete_target_);
    } else {
        emit friendDeleteFailed(message);
    }
}

void FriendService::onFriendDeletedPush(const PDUHeader&,
                                        const std::vector<uint8_t>& body) {
    size_t offset = 0;
    std::string username;
    if (!readString(body, offset, username)) return;
    emit friendDeleted(QString::fromStdString(username));
}

} // namespace cloudvault
