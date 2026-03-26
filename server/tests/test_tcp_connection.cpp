// =============================================================
// server/tests/test_tcp_connection.cpp
// TcpConnection 单元测试
// =============================================================

#include "server/tcp_connection.h"

#include <gtest/gtest.h>

using namespace cloudvault;

TEST(TcpConnection, CloseInvokesRegisteredCallbacks) {
    auto conn = std::make_shared<TcpConnection>(nullptr, -1, "peer-e");

    int close_count = 0;
    conn->setCloseCallback([&close_count](std::shared_ptr<TcpConnection>) {
        ++close_count;
    });

    conn->close();
    EXPECT_EQ(close_count, 1);
}
