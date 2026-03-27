// =============================================================
// client/src/service/friend_service.h
// 好友功能协议封装
// =============================================================

#pragma once

#include "network/response_router.h"
#include "network/tcp_client.h"

#include <QObject>
#include <QPair>
#include <QString>
#include <QList>

namespace cloudvault {

class FriendService : public QObject {
    Q_OBJECT

public:
    explicit FriendService(TcpClient& client,
                           ResponseRouter& router,
                           QObject* parent = nullptr);

    void findUser(const QString& target);
    void addFriend(const QString& target);
    void agreeFriend(const QString& requester);
    void flushFriends();
    void deleteFriend(const QString& target);

signals:
    void userFound(const QString& username, bool online);
    void userNotFound(const QString& username);
    void friendRequestSent(const QString& target);
    void incomingFriendRequest(const QString& from);
    void friendAdded(const QString& username);
    void friendAddFailed(const QString& reason);
    void friendsRefreshed(const QList<QPair<QString, bool>>& friends);
    void friendListRefreshFailed(const QString& reason);
    void friendDeleted(const QString& username);
    void friendDeleteFailed(const QString& reason);

private:
    void onFindUserResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onAddFriendResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onFriendRequestPush(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onAgreeFriendResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onFriendAddedPush(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onFlushFriendsResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onDeleteFriendResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onFriendDeletedPush(const PDUHeader& hdr, const std::vector<uint8_t>& body);

    TcpClient&      client_;
    ResponseRouter& router_;

    QString pending_find_target_;
    QString pending_add_target_;
    QString pending_agree_target_;
    QString pending_delete_target_;
};

} // namespace cloudvault
