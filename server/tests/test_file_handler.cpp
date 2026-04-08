// =============================================================
// server/tests/test_file_handler.cpp
// FileHandler 单元测试
// =============================================================

#include "server/file_storage.h"
#include "server/handler/file_handler.h"
#include "server/session_manager.h"
#include "server/tcp_connection.h"

#include "common/protocol.h"
#include "common/protocol_codec.h"
#include "test_helpers.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <cstring>

using namespace cloudvault;

namespace {

struct CapturedPdu {
    PDUHeader header{};
    std::vector<uint8_t> body;
};

std::vector<CapturedPdu> decodeCaptured(const std::vector<std::vector<uint8_t>>& raw_pdus) {
    std::vector<CapturedPdu> decoded;
    for (const auto& raw : raw_pdus) {
        PDUParser parser;
        parser.feed(raw.data(), raw.size());
        CapturedPdu item;
        EXPECT_TRUE(parser.tryParse(item.header, item.body));
        decoded.push_back(std::move(item));
    }
    return decoded;
}

uint8_t readStatus(const std::vector<uint8_t>& body) {
    EXPECT_FALSE(body.empty());
    return body.empty() ? 0 : body.front();
}

std::string readStringField(const std::vector<uint8_t>& body, size_t offset) {
    if (offset + 2 > body.size()) {
        return {};
    }
    uint16_t len = 0;
    std::memcpy(&len, body.data() + offset, sizeof(len));
    len = ntohs(len);
    offset += 2;
    if (offset + len > body.size()) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(body.data() + offset), len);
}

std::shared_ptr<TcpConnection> makeCapturedConn(std::vector<std::vector<uint8_t>>& sent) {
    auto conn = std::make_shared<TcpConnection>(nullptr, -1, "handler-test");
    conn->setSendInterceptor([&sent](const std::vector<uint8_t>& data) {
        sent.push_back(data);
    });
    return conn;
}

std::vector<uint8_t> extractBody(const std::vector<uint8_t>& pdu, PDUHeader& header) {
    PDUParser parser;
    parser.feed(pdu.data(), pdu.size());
    std::vector<uint8_t> body;
    EXPECT_TRUE(parser.tryParse(header, body));
    return body;
}

}  // namespace

TEST(FileHandler, UnauthorizedListReturnsStatusMessage) {
    test_helpers::TempDir temp_dir;
    FileStorage storage(temp_dir.path);
    SessionManager sessions;
    FileHandler handler(sessions, storage);

    std::vector<std::vector<uint8_t>> sent;
    auto conn = makeCapturedConn(sent);
    PDUHeader request_header{};
    const auto request_body =
        extractBody(PDUBuilder(MessageType::FLUSH_FILE).writeString("/").build(), request_header);
    handler.handleList(conn, request_header, request_body);

    ASSERT_EQ(sent.size(), 1u);
    const auto decoded = decodeCaptured(sent);
    ASSERT_EQ(decoded.size(), 1u);
    EXPECT_EQ(decoded[0].header.message_type, static_cast<uint32_t>(MessageType::FLUSH_FILE));
    EXPECT_EQ(readStatus(decoded[0].body), 4u);
}

TEST(FileHandler, ListAndSearchReturnEntriesForLoggedInUser) {
    test_helpers::TempDir temp_dir;
    FileStorage storage(temp_dir.path);
    storage.createDirectory("alice", "/", "docs");
    const auto target = storage.prepareUploadTarget("alice", "/docs", "report.txt");
    std::ofstream output(target.absolute_path, std::ios::binary);
    ASSERT_TRUE(output.is_open());
    output << "hello";
    output.close();

    SessionManager sessions;
    FileHandler handler(sessions, storage);
    std::vector<std::vector<uint8_t>> sent;
    auto conn = makeCapturedConn(sent);
    sessions.addSession(1, "alice", conn);

    PDUHeader list_header{};
    const auto list_body =
        extractBody(PDUBuilder(MessageType::FLUSH_FILE).writeString("/docs").build(), list_header);
    handler.handleList(conn, list_header, list_body);

    PDUHeader search_header{};
    const auto search_body =
        extractBody(PDUBuilder(MessageType::SEARCH_FILE).writeString("report").build(), search_header);
    handler.handleSearch(conn, search_header, search_body);

    ASSERT_EQ(sent.size(), 2u);
    const auto decoded = decodeCaptured(sent);
    EXPECT_EQ(decoded[0].header.message_type, static_cast<uint32_t>(MessageType::FLUSH_FILE));
    EXPECT_EQ(readStatus(decoded[0].body), 0u);
    EXPECT_EQ(readStringField(decoded[0].body, 1), "/docs");
    EXPECT_EQ(decoded[1].header.message_type, static_cast<uint32_t>(MessageType::SEARCH_FILE));
    EXPECT_EQ(readStatus(decoded[1].body), 0u);
    EXPECT_EQ(readStringField(decoded[1].body, 1), "report");
}

TEST(FileHandler, EmptyUploadCompletesImmediately) {
    test_helpers::TempDir temp_dir;
    FileStorage storage(temp_dir.path);
    storage.createDirectory("alice", "/", "uploads");

    SessionManager sessions;
    FileHandler handler(sessions, storage);
    std::vector<std::vector<uint8_t>> sent;
    auto conn = makeCapturedConn(sent);
    sessions.addSession(1, "alice", conn);

    PDUHeader header{};
    const auto parsed_body = extractBody(
        PDUBuilder(MessageType::UPLOAD_REQUEST)
            .writeFixedString("empty.txt", 32)
            .writeUInt64(0)
            .writeString("/uploads")
            .build(),
        header);

    handler.handleUploadRequest(conn, header, parsed_body);

    ASSERT_EQ(sent.size(), 2u);
    const auto decoded = decodeCaptured(sent);
    EXPECT_EQ(decoded[0].header.message_type, static_cast<uint32_t>(MessageType::UPLOAD_REQUEST));
    EXPECT_EQ(readStatus(decoded[0].body), 0u);
    EXPECT_EQ(decoded[1].header.message_type, static_cast<uint32_t>(MessageType::UPLOAD_DATA));
    EXPECT_EQ(readStatus(decoded[1].body), 0u);

    EXPECT_TRUE(std::filesystem::exists(temp_dir.path / "alice" / "uploads" / "empty.txt"));
}

TEST(FileHandler, DownloadRequestStreamsInitAndChunk) {
    test_helpers::TempDir temp_dir;
    FileStorage storage(temp_dir.path);
    storage.createDirectory("alice", "/", "docs");
    const auto file = storage.prepareUploadTarget("alice", "/docs", "hello.txt");
    std::ofstream output(file.absolute_path, std::ios::binary);
    ASSERT_TRUE(output.is_open());
    output << "hello world";
    output.close();

    SessionManager sessions;
    FileHandler handler(sessions, storage);
    std::vector<std::vector<uint8_t>> sent;
    auto conn = makeCapturedConn(sent);
    sessions.addSession(1, "alice", conn);

    PDUHeader header{};
    const auto body = extractBody(
        PDUBuilder(MessageType::DOWNLOAD_REQUEST)
            .writeString("/docs/hello.txt")
            .build(),
        header);

    handler.handleDownloadRequest(conn, header, body);

    ASSERT_GE(sent.size(), 2u);
    const auto decoded = decodeCaptured(sent);
    EXPECT_EQ(decoded.front().header.message_type,
              static_cast<uint32_t>(MessageType::DOWNLOAD_REQUEST));
    EXPECT_EQ(readStatus(decoded.front().body), 0u);
    EXPECT_EQ(decoded[1].header.message_type,
              static_cast<uint32_t>(MessageType::DOWNLOAD_DATA));
}
