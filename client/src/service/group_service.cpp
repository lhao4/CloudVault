// =============================================================
// client/src/service/group_service.cpp
// 群组服务实现
// =============================================================

#include "group_service.h"

#include "common/protocol_codec.h"

#include <cstring>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

namespace cloudvault {

namespace {

bool readUInt32(const std::vector<uint8_t>& body, size_t& offset, uint32_t& out) {
    if (offset + 4 > body.size()) return false;
    uint32_t net = 0;
    std::memcpy(&net, body.data() + offset, sizeof(net));
    out = ntohl(net);
    offset += 4;
    return true;
}

bool readUInt16(const std::vector<uint8_t>& body, size_t& offset, uint16_t& out) {
    if (offset + 2 > body.size()) return false;
    uint16_t net = 0;
    std::memcpy(&net, body.data() + offset, sizeof(net));
    out = ntohs(net);
    offset += 2;
    return true;
}

bool readString(const std::vector<uint8_t>& body, size_t& offset, std::string& out) {
    uint16_t len = 0;
    if (!readUInt16(body, offset, len) || offset + len > body.size()) {
        return false;
    }
    out.assign(reinterpret_cast<const char*>(body.data() + offset), len);
    offset += len;
    return true;
}

QString readMessage(const std::vector<uint8_t>& body, size_t offset) {
    std::string message;
    return readString(body, offset, message)
        ? QString::fromStdString(message)
        : QStringLiteral("服务器返回格式错误");
}

} // namespace

GroupService::GroupService(TcpClient& client,
                           ResponseRouter& router,
                           QObject* parent)
    : QObject(parent), client_(client), router_(router) {
    qRegisterMetaType<GroupListEntry>("GroupListEntry");
    qRegisterMetaType<QList<GroupListEntry>>("QList<GroupListEntry>");

    router_.registerHandler(MessageType::CREATE_GROUP_RESPONSE,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onCreateGroupResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::JOIN_GROUP_RESPONSE,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onJoinGroupResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::LEAVE_GROUP_RESPONSE,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onLeaveGroupResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::GET_GROUP_LIST_RESPONSE,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onGetGroupListResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::GROUP_CHAT,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onGroupChatPush(hdr, body);
                            });
}

void GroupService::createGroup(const QString& name) {
    client_.send(PDUBuilder(MessageType::CREATE_GROUP_REQUEST)
                     .writeString(name.trimmed().toStdString())
                     .build());
}

void GroupService::joinGroup(int groupId) {
    client_.send(PDUBuilder(MessageType::JOIN_GROUP_REQUEST)
                     .writeUInt32(static_cast<uint32_t>(groupId))
                     .build());
}

void GroupService::leaveGroup(int groupId) {
    client_.send(PDUBuilder(MessageType::LEAVE_GROUP_REQUEST)
                     .writeUInt32(static_cast<uint32_t>(groupId))
                     .build());
}

void GroupService::getGroupList() {
    client_.send(PDUBuilder(MessageType::GET_GROUP_LIST_REQUEST).build());
}

void GroupService::sendGroupMessage(int groupId, const QString& content) {
    client_.send(PDUBuilder(MessageType::GROUP_CHAT)
                     .writeUInt32(static_cast<uint32_t>(groupId))
                     .writeString(content.toStdString())
                     .build());
}

void GroupService::onCreateGroupResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (body.size() < 5) {
        emit groupCreateFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    uint32_t group_id = 0;
    std::string name;
    if (!readUInt32(body, offset, group_id) || !readString(body, offset, name)) {
        emit groupCreateFailed(QStringLiteral("创建群组响应格式错误"));
        return;
    }

    const QString message = readMessage(body, offset);
    if (status == 0) {
        emit groupCreated(static_cast<int>(group_id), QString::fromStdString(name));
    } else {
        emit groupCreateFailed(message);
    }
}

void GroupService::onJoinGroupResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (body.size() < 5) {
        emit joinFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    uint32_t group_id = 0;
    if (!readUInt32(body, offset, group_id)) {
        emit joinFailed(QStringLiteral("加入群组响应格式错误"));
        return;
    }

    const QString message = readMessage(body, offset);
    if (status == 0) {
        emit joinedGroup(static_cast<int>(group_id));
    } else {
        emit joinFailed(message);
    }
}

void GroupService::onLeaveGroupResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (body.size() < 5) {
        emit leaveFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    uint32_t group_id = 0;
    if (!readUInt32(body, offset, group_id)) {
        emit leaveFailed(QStringLiteral("退出群组响应格式错误"));
        return;
    }

    const QString message = readMessage(body, offset);
    if (status == 0) {
        emit leftGroup(static_cast<int>(group_id));
    } else {
        emit leaveFailed(message);
    }
}

void GroupService::onGetGroupListResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (body.size() < 3) {
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    uint16_t count = 0;
    if (!readUInt16(body, offset, count)) {
        return;
    }

    if (status != 0) {
        return;
    }

    QList<GroupListEntry> groups;
    for (uint16_t i = 0; i < count; ++i) {
        uint32_t group_id = 0;
        uint32_t owner_id = 0;
        uint16_t online_count = 0;
        std::string name;
        if (!readUInt32(body, offset, group_id) ||
            !readString(body, offset, name) ||
            !readUInt32(body, offset, owner_id) ||
            !readUInt16(body, offset, online_count)) {
            return;
        }

        GroupListEntry entry;
        entry.group_id = static_cast<int>(group_id);
        entry.name = QString::fromStdString(name);
        entry.online_count = static_cast<int>(online_count);
        entry.owner_id = static_cast<int>(owner_id);
        groups.append(entry);
    }

    emit groupListReceived(groups);
}

void GroupService::onGroupChatPush(const PDUHeader&, const std::vector<uint8_t>& body) {
    size_t offset = 0;
    uint32_t group_id = 0;
    std::string from;
    std::string content;
    std::string timestamp;
    if (!readUInt32(body, offset, group_id) ||
        !readString(body, offset, from) ||
        !readString(body, offset, content) ||
        !readString(body, offset, timestamp)) {
        return;
    }

    emit groupMessageReceived(QString::fromStdString(from),
                              static_cast<int>(group_id),
                              QString::fromStdString(content),
                              QString::fromStdString(timestamp));
}

} // namespace cloudvault
