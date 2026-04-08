// =============================================================
// server/src/handler/group_handler.cpp
// 群组管理与群聊处理器
// =============================================================

#include "server/handler/group_handler.h"

#include "common/protocol_codec.h"
#include "server/tcp_connection.h"

#include <arpa/inet.h>
#include <cstring>
#include <spdlog/spdlog.h>

namespace cloudvault {

namespace {

constexpr uint8_t kStatusOk = 0;
constexpr uint8_t kStatusInvalid = 1;
constexpr uint8_t kStatusNotFound = 2;
constexpr uint8_t kStatusAlreadyMember = 3;
constexpr uint8_t kStatusInternal = 5;
constexpr uint8_t kStatusUnauthorized = 7;
constexpr uint8_t kStatusForbidden = 8;
constexpr size_t kMaxMessageBytes = 4096;

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

bool readUInt32(const std::vector<uint8_t>& body, size_t& offset, uint32_t& out) {
    if (offset + 4 > body.size()) return false;
    uint32_t net = 0;
    std::memcpy(&net, body.data() + offset, sizeof(net));
    out = ntohl(net);
    offset += 4;
    return true;
}

std::vector<uint8_t> buildGroupStatus(MessageType type,
                                      uint8_t status,
                                      int group_id,
                                      const std::string& message,
                                      const std::string& name = {}) {
    auto builder = PDUBuilder(type)
        .writeUInt8(status)
        .writeUInt32(static_cast<uint32_t>(group_id));
    if (type == MessageType::CREATE_GROUP_RESPONSE) {
        builder.writeString(name);
    }
    builder.writeString(message);
    return builder.build();
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

std::string encodeOfflineGroupPayload(int group_id, const std::string& content) {
    return std::to_string(group_id) + '\n' + content;
}

} // namespace

GroupHandler::GroupHandler(Database& db, SessionManager& sessions)
    : users_(db), groups_(db), messages_(db), sessions_(sessions) {}

void GroupHandler::handleCreateGroup(std::shared_ptr<TcpConnection> conn,
                                     const PDUHeader&,
                                     const std::vector<uint8_t>& body) {
    const auto session = sessions_.findByConnection(conn);
    if (!session) {
        conn->send(buildGroupStatus(MessageType::CREATE_GROUP_RESPONSE,
                                    kStatusUnauthorized, 0, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string name;
    if (!readString(body, offset, name) || name.empty()) {
        conn->send(buildGroupStatus(MessageType::CREATE_GROUP_RESPONSE,
                                    kStatusInvalid, 0, "群名称不能为空"));
        return;
    }

    const int group_id = groups_.createGroup(name, session->user_id);
    if (group_id <= 0) {
        conn->send(buildGroupStatus(MessageType::CREATE_GROUP_RESPONSE,
                                    kStatusInternal, 0, "创建群组失败"));
        return;
    }

    conn->send(buildGroupStatus(MessageType::CREATE_GROUP_RESPONSE,
                                kStatusOk, group_id, "创建成功", name));
}

void GroupHandler::handleJoinGroup(std::shared_ptr<TcpConnection> conn,
                                   const PDUHeader&,
                                   const std::vector<uint8_t>& body) {
    const auto session = sessions_.findByConnection(conn);
    if (!session) {
        conn->send(buildGroupStatus(MessageType::JOIN_GROUP_RESPONSE,
                                    kStatusUnauthorized, 0, "未登录"));
        return;
    }

    size_t offset = 0;
    uint32_t group_id = 0;
    if (!readUInt32(body, offset, group_id) || group_id == 0) {
        conn->send(buildGroupStatus(MessageType::JOIN_GROUP_RESPONSE,
                                    kStatusInvalid, 0, "群组 ID 无效"));
        return;
    }

    const auto group = groups_.findGroup(static_cast<int>(group_id));
    if (!group) {
        conn->send(buildGroupStatus(MessageType::JOIN_GROUP_RESPONSE,
                                    kStatusNotFound, static_cast<int>(group_id), "群组不存在"));
        return;
    }

    if (groups_.isMember(static_cast<int>(group_id), session->user_id)) {
        conn->send(buildGroupStatus(MessageType::JOIN_GROUP_RESPONSE,
                                    kStatusAlreadyMember, static_cast<int>(group_id), "你已经在群里"));
        return;
    }

    if (!groups_.addMember(static_cast<int>(group_id), session->user_id)) {
        conn->send(buildGroupStatus(MessageType::JOIN_GROUP_RESPONSE,
                                    kStatusInternal, static_cast<int>(group_id), "加入群组失败"));
        return;
    }

    conn->send(buildGroupStatus(MessageType::JOIN_GROUP_RESPONSE,
                                kStatusOk, static_cast<int>(group_id), "加入成功"));
}

void GroupHandler::handleLeaveGroup(std::shared_ptr<TcpConnection> conn,
                                    const PDUHeader&,
                                    const std::vector<uint8_t>& body) {
    const auto session = sessions_.findByConnection(conn);
    if (!session) {
        conn->send(buildGroupStatus(MessageType::LEAVE_GROUP_RESPONSE,
                                    kStatusUnauthorized, 0, "未登录"));
        return;
    }

    size_t offset = 0;
    uint32_t group_id = 0;
    if (!readUInt32(body, offset, group_id) || group_id == 0) {
        conn->send(buildGroupStatus(MessageType::LEAVE_GROUP_RESPONSE,
                                    kStatusInvalid, 0, "群组 ID 无效"));
        return;
    }

    const auto group = groups_.findGroup(static_cast<int>(group_id));
    if (!group) {
        conn->send(buildGroupStatus(MessageType::LEAVE_GROUP_RESPONSE,
                                    kStatusNotFound, static_cast<int>(group_id), "群组不存在"));
        return;
    }

    if (!groups_.isMember(static_cast<int>(group_id), session->user_id)) {
        conn->send(buildGroupStatus(MessageType::LEAVE_GROUP_RESPONSE,
                                    kStatusForbidden, static_cast<int>(group_id), "你不在该群组中"));
        return;
    }

    bool ok = false;
    std::string message;
    if (group->owner_id == session->user_id) {
        ok = groups_.deleteGroup(static_cast<int>(group_id));
        message = ok ? "群组已解散" : "解散群组失败";
    } else {
        ok = groups_.removeMember(static_cast<int>(group_id), session->user_id);
        message = ok ? "已退出群组" : "退出群组失败";
    }

    conn->send(buildGroupStatus(MessageType::LEAVE_GROUP_RESPONSE,
                                ok ? kStatusOk : kStatusInternal,
                                static_cast<int>(group_id),
                                message));
}

void GroupHandler::handleGetGroupList(std::shared_ptr<TcpConnection> conn,
                                      const PDUHeader&,
                                      const std::vector<uint8_t>&) {
    const auto session = sessions_.findByConnection(conn);
    if (!session) {
        conn->send(PDUBuilder(MessageType::GET_GROUP_LIST_RESPONSE)
                       .writeUInt8(kStatusUnauthorized)
                       .writeUInt16(0)
                       .writeString("未登录")
                       .build());
        return;
    }

    const auto groups = groups_.listUserGroups(session->user_id);
    auto response = PDUBuilder(MessageType::GET_GROUP_LIST_RESPONSE)
        .writeUInt8(kStatusOk)
        .writeUInt16(static_cast<uint16_t>(groups.size()));
    for (const auto& group : groups) {
        const auto online_members = groups_.listOnlineMembers(group.id);
        response.writeUInt32(static_cast<uint32_t>(group.id))
            .writeString(group.name)
            .writeUInt32(static_cast<uint32_t>(group.owner_id))
            .writeUInt16(static_cast<uint16_t>(online_members.size()));
    }
    conn->send(response.build());
}

void GroupHandler::handleGroupChat(std::shared_ptr<TcpConnection> conn,
                                   const PDUHeader&,
                                   const std::vector<uint8_t>& body) {
    const auto session = sessions_.findByConnection(conn);
    if (!session) {
        return;
    }

    size_t offset = 0;
    uint32_t group_id = 0;
    std::string content;
    if (!readUInt32(body, offset, group_id) ||
        !readString(body, offset, content) ||
        group_id == 0 ||
        content.empty() ||
        content.size() > kMaxMessageBytes) {
        return;
    }

    const auto group = groups_.findGroup(static_cast<int>(group_id));
    if (!group || !groups_.isMember(static_cast<int>(group_id), session->user_id)) {
        return;
    }

    const std::string created_at =
        messages_.storeGroupMessage(session->user_id, static_cast<int>(group_id), content);
    if (created_at.empty()) {
        spdlog::error("GROUP_CHAT persist failed: user={} group_id={}",
                      session->username, group_id);
        return;
    }

    const auto members = groups_.listMembers(static_cast<int>(group_id));
    for (const auto& member : members) {
        if (member.user_id == session->user_id) {
            continue;
        }

        auto push = buildGroupChatPush(session->username,
                                       static_cast<int>(group_id),
                                       content,
                                       created_at);
        if (member.is_online) {
            sessions_.sendTo(member.username, std::move(push));
            continue;
        }

        messages_.queueOfflineMessage(session->user_id,
                                      member.user_id,
                                      static_cast<uint32_t>(MessageType::GROUP_CHAT),
                                      encodeOfflineGroupPayload(static_cast<int>(group_id), content),
                                      created_at);
    }
}

} // namespace cloudvault
