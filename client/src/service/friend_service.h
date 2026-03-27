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

/**
 * @brief 好友业务服务。
 *
 * 封装查找用户、添加好友、同意申请、刷新好友列表、删除好友等协议交互。
 */
class FriendService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造好友服务。
     * @param client TCP 客户端引用。
     * @param router 响应路由器引用。
     * @param parent Qt 父对象。
     */
    explicit FriendService(TcpClient& client,
                           ResponseRouter& router,
                           QObject* parent = nullptr);

    /**
     * @brief 查找用户。
     * @param target 目标用户名。
     */
    void findUser(const QString& target);
    /**
     * @brief 发起添加好友请求。
     * @param target 目标用户名。
     */
    void addFriend(const QString& target);
    /**
     * @brief 同意好友请求。
     * @param requester 请求方用户名。
     */
    void agreeFriend(const QString& requester);
    /**
     * @brief 刷新好友列表。
     */
    void flushFriends();
    /**
     * @brief 删除好友。
     * @param target 目标用户名。
     */
    void deleteFriend(const QString& target);

signals:
    /// @brief 查找到用户。
    void userFound(const QString& username, bool online);
    /// @brief 用户不存在。
    void userNotFound(const QString& username);
    /// @brief 好友申请发送成功。
    void friendRequestSent(const QString& target);
    /// @brief 收到好友申请推送。
    void incomingFriendRequest(const QString& from);
    /// @brief 好友建立成功。
    void friendAdded(const QString& username);
    /// @brief 添加好友失败。
    void friendAddFailed(const QString& reason);
    /// @brief 好友列表刷新成功。
    void friendsRefreshed(const QList<QPair<QString, bool>>& friends);
    /// @brief 好友列表刷新失败。
    void friendListRefreshFailed(const QString& reason);
    /// @brief 删除好友成功。
    void friendDeleted(const QString& username);
    /// @brief 删除好友失败。
    void friendDeleteFailed(const QString& reason);

private:
    /// @brief 处理查找用户响应。
    void onFindUserResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理添加好友响应。
    void onAddFriendResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理好友申请推送。
    void onFriendRequestPush(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理同意好友响应。
    void onAgreeFriendResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理好友已添加推送。
    void onFriendAddedPush(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理好友列表刷新响应。
    void onFlushFriendsResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理删除好友响应。
    void onDeleteFriendResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理好友被删除推送。
    void onFriendDeletedPush(const PDUHeader& hdr, const std::vector<uint8_t>& body);

    /// @brief TCP 客户端引用。
    TcpClient&      client_;
    /// @brief 响应路由器引用。
    ResponseRouter& router_;

    /// @brief 当前挂起的查找用户目标。
    QString pending_find_target_;
    /// @brief 当前挂起的添加好友目标。
    QString pending_add_target_;
    /// @brief 当前挂起的同意好友目标。
    QString pending_agree_target_;
    /// @brief 当前挂起的删除好友目标。
    QString pending_delete_target_;
};

} // namespace cloudvault
