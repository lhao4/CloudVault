// =============================================================
// server/tests/test_session_dispatch.cpp
// SessionManager / MessageDispatcher 单元测试
// =============================================================

#include "server/message_dispatcher.h"
#include "server/session_manager.h"
#include "server/tcp_connection.h"

#include <gtest/gtest.h>

#include <memory>

using namespace cloudvault;

namespace {

std::shared_ptr<TcpConnection> fakeConn(int token) {
    auto owner = std::make_shared<int>(token);
    return std::shared_ptr<TcpConnection>(
        owner,
        reinterpret_cast<TcpConnection*>(static_cast<uintptr_t>(token + 1)));
}

std::shared_ptr<TcpConnection> capturedConn(std::vector<uint8_t>& sent) {
    auto conn = std::make_shared<TcpConnection>(nullptr, -1, "captured");
    conn->setSendInterceptor([&sent](const std::vector<uint8_t>& data) {
        sent = data;
    });
    return conn;
}

}  // namespace

TEST(SessionManager, AddQueryAndRemoveSession) {
    auto conn = fakeConn(7);

    SessionManager sessions;
    sessions.addSession(7, "alice", conn);

    EXPECT_TRUE(sessions.isOnline("alice"));
    ASSERT_TRUE(sessions.getConnection("alice"));
    ASSERT_TRUE(sessions.findByConnection(conn).has_value());
    EXPECT_EQ(sessions.findByConnection(conn)->user_id, 7);

    const auto online_users = sessions.onlineUsers();
    ASSERT_EQ(online_users.size(), 1u);
    EXPECT_EQ(online_users.front(), "alice");

    std::vector<uint8_t> sent;
    auto send_conn = capturedConn(sent);
    sessions.addSession(8, "bob", send_conn);
    sessions.sendTo("bob", std::vector<uint8_t>{1u, 2u, 3u});
    EXPECT_EQ(sent, (std::vector<uint8_t>{1u, 2u, 3u}));

    sessions.removeByConnection(conn);
    EXPECT_FALSE(sessions.isOnline("alice"));
    EXPECT_EQ(sessions.getConnection("alice"), nullptr);
}

TEST(MessageDispatcher, RegisteredHandlerReceivesPayload) {
    auto conn = fakeConn(42);

    MessageDispatcher dispatcher;
    bool called = false;
    dispatcher.registerHandler(
        MessageType::PING,
        [&called, &conn](std::shared_ptr<TcpConnection> actual_conn,
                         const PDUHeader& header,
                         const std::vector<uint8_t>& body) {
            called = true;
            EXPECT_EQ(actual_conn, conn);
            EXPECT_EQ(header.message_type, static_cast<uint32_t>(MessageType::PING));
            EXPECT_EQ(body.size(), 3u);
            EXPECT_EQ(body[0], 1u);
        });

    PDUHeader header{};
    header.message_type = static_cast<uint32_t>(MessageType::PING);
    header.body_length = 3;
    std::vector<uint8_t> body{1u, 2u, 3u};

    dispatcher.dispatch(conn, header, body);
    EXPECT_TRUE(called);
}
