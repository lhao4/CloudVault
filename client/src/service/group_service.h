// =============================================================
// client/src/service/group_service.h
// 群组服务
// =============================================================

#pragma once

#include "network/response_router.h"
#include "network/tcp_client.h"
#include "ui/group_list_dialog.h"

#include <QObject>

namespace cloudvault {

class GroupService : public QObject {
    Q_OBJECT

public:
    explicit GroupService(TcpClient& client,
                          ResponseRouter& router,
                          QObject* parent = nullptr);

    void createGroup(const QString& name);
    void joinGroup(int groupId);
    void leaveGroup(int groupId);
    void getGroupList();
    void sendGroupMessage(int groupId, const QString& content);

signals:
    void groupCreated(int groupId, const QString& name);
    void groupCreateFailed(const QString& reason);
    void joinedGroup(int groupId);
    void joinFailed(const QString& reason);
    void leftGroup(int groupId);
    void leaveFailed(const QString& reason);
    void groupListReceived(const QList<GroupListEntry>& groups);
    void groupMessageReceived(const QString& from,
                              int groupId,
                              const QString& content,
                              const QString& timestamp);

private:
    void onCreateGroupResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onJoinGroupResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onLeaveGroupResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onGetGroupListResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onGroupChatPush(const PDUHeader& hdr, const std::vector<uint8_t>& body);

    TcpClient& client_;
    ResponseRouter& router_;
};

} // namespace cloudvault
