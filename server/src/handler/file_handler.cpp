// =============================================================
// server/src/handler/file_handler.cpp
// 第十一章：文件管理业务逻辑
// =============================================================

#include "server/handler/file_handler.h"

#include "common/protocol_codec.h"
#include "server/tcp_connection.h"

#include <arpa/inet.h>
#include <cstring>

namespace cloudvault {

namespace {

constexpr uint8_t kStatusOk = 0;
constexpr uint8_t kStatusInvalid = 1;
constexpr uint8_t kStatusNotFound = 2;
constexpr uint8_t kStatusConflict = 3;
constexpr uint8_t kStatusUnauthorized = 4;
constexpr uint8_t kStatusInternal = 5;

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

std::vector<uint8_t> buildStatus(MessageType type,
                                 uint8_t status,
                                 const std::string& message) {
    return PDUBuilder(type)
        .writeUInt8(status)
        .writeString(message)
        .build();
}

std::vector<uint8_t> buildList(MessageType type,
                               uint8_t status,
                               const std::string& path,
                               const std::vector<FileStorage::Entry>& entries) {
    auto pdu = PDUBuilder(type)
        .writeUInt8(status)
        .writeString(path)
        .writeUInt16(static_cast<uint16_t>(entries.size()));
    for (const auto& entry : entries) {
        pdu.writeString(entry.name)
            .writeString(entry.path)
            .writeUInt8(entry.is_dir ? 1 : 0)
            .writeUInt64(entry.size)
            .writeString(entry.modified_at);
    }
    return pdu.build();
}

std::pair<uint8_t, std::string> mapStorageError(const std::runtime_error& error) {
    const std::string message = error.what();
    if (message.find("越权") != std::string::npos) {
        return {kStatusUnauthorized, message};
    }
    if (message.find("不存在") != std::string::npos) {
        return {kStatusNotFound, message};
    }
    if (message.find("已存在") != std::string::npos) {
        return {kStatusConflict, message};
    }
    return {kStatusInvalid, message};
}

} // namespace

FileHandler::FileHandler(SessionManager& sessions, FileStorage& storage)
    : sessions_(sessions), storage_(storage) {}

void FileHandler::handleList(std::shared_ptr<TcpConnection> conn,
                             const PDUHeader&,
                             const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::FLUSH_FILE, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string path;
    if (!readString(body, offset, path)) {
        conn->send(buildStatus(MessageType::FLUSH_FILE, kStatusInvalid, "请求格式错误"));
        return;
    }

    try {
        const auto entries = storage_.list(current->username, path);
        conn->send(buildList(MessageType::FLUSH_FILE, kStatusOk, path.empty() ? "/" : path, entries));
    } catch (const std::runtime_error& error) {
        const auto [status, message] = mapStorageError(error);
        conn->send(buildStatus(MessageType::FLUSH_FILE, status, message));
    }
}

void FileHandler::handleMkdir(std::shared_ptr<TcpConnection> conn,
                              const PDUHeader&,
                              const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::MKDIR, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string dir_name;
    std::string parent_path;
    if (!readString(body, offset, dir_name) || !readString(body, offset, parent_path)) {
        conn->send(buildStatus(MessageType::MKDIR, kStatusInvalid, "请求格式错误"));
        return;
    }

    try {
        storage_.createDirectory(current->username, parent_path, dir_name);
        conn->send(buildStatus(MessageType::MKDIR, kStatusOk, "新建文件夹成功"));
    } catch (const std::runtime_error& error) {
        const auto [status, message] = mapStorageError(error);
        conn->send(buildStatus(MessageType::MKDIR, status, message));
    }
}

void FileHandler::handleRename(std::shared_ptr<TcpConnection> conn,
                               const PDUHeader&,
                               const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::RENAME, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string target_path;
    std::string new_name;
    if (!readString(body, offset, target_path) || !readString(body, offset, new_name)) {
        conn->send(buildStatus(MessageType::RENAME, kStatusInvalid, "请求格式错误"));
        return;
    }

    try {
        storage_.renamePath(current->username, target_path, new_name);
        conn->send(buildStatus(MessageType::RENAME, kStatusOk, "重命名成功"));
    } catch (const std::runtime_error& error) {
        const auto [status, message] = mapStorageError(error);
        conn->send(buildStatus(MessageType::RENAME, status, message));
    }
}

void FileHandler::handleMove(std::shared_ptr<TcpConnection> conn,
                             const PDUHeader&,
                             const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::MOVE, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string source_path;
    std::string destination_path;
    if (!readString(body, offset, source_path) || !readString(body, offset, destination_path)) {
        conn->send(buildStatus(MessageType::MOVE, kStatusInvalid, "请求格式错误"));
        return;
    }

    try {
        storage_.movePath(current->username, source_path, destination_path);
        conn->send(buildStatus(MessageType::MOVE, kStatusOk, "移动成功"));
    } catch (const std::runtime_error& error) {
        const auto [status, message] = mapStorageError(error);
        conn->send(buildStatus(MessageType::MOVE, status, message));
    }
}

void FileHandler::handleDelete(std::shared_ptr<TcpConnection> conn,
                               const PDUHeader&,
                               const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::DELETE_FILE, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string target_path;
    if (!readString(body, offset, target_path)) {
        conn->send(buildStatus(MessageType::DELETE_FILE, kStatusInvalid, "请求格式错误"));
        return;
    }

    try {
        storage_.deletePath(current->username, target_path);
        conn->send(buildStatus(MessageType::DELETE_FILE, kStatusOk, "删除成功"));
    } catch (const std::runtime_error& error) {
        const auto [status, message] = mapStorageError(error);
        conn->send(buildStatus(MessageType::DELETE_FILE, status, message));
    }
}

void FileHandler::handleSearch(std::shared_ptr<TcpConnection> conn,
                               const PDUHeader&,
                               const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::SEARCH_FILE, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string keyword;
    if (!readString(body, offset, keyword)) {
        conn->send(buildStatus(MessageType::SEARCH_FILE, kStatusInvalid, "请求格式错误"));
        return;
    }

    try {
        const auto entries = storage_.search(current->username, keyword);
        conn->send(buildList(MessageType::SEARCH_FILE, kStatusOk, keyword, entries));
    } catch (const std::runtime_error& error) {
        const auto [status, message] = mapStorageError(error);
        conn->send(buildStatus(MessageType::SEARCH_FILE, status, message));
    }
}

} // namespace cloudvault
