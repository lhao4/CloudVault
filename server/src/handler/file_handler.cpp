// =============================================================
// server/src/handler/file_handler.cpp
// 第十一、十二章：文件管理与文件传输业务逻辑
// =============================================================

#include "server/handler/file_handler.h"

#include "common/protocol_codec.h"
#include "server/tcp_connection.h"

#include <arpa/inet.h>
#include <spdlog/spdlog.h>

#include <array>
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

bool readUInt32(const std::vector<uint8_t>& body, size_t& offset, uint32_t& out) {
    if (offset + 4 > body.size()) {
        return false;
    }

    std::memcpy(&out, body.data() + offset, sizeof(out));
    out = ntohl(out);
    offset += 4;
    return true;
}

uint64_t ntoh64(uint64_t value) {
    const uint32_t probe = 1;
    if (*reinterpret_cast<const uint8_t*>(&probe) == 0) {
        return value;
    }
    return ((value & 0xFF00000000000000ULL) >> 56)
         | ((value & 0x00FF000000000000ULL) >> 40)
         | ((value & 0x0000FF0000000000ULL) >> 24)
         | ((value & 0x000000FF00000000ULL) >>  8)
         | ((value & 0x00000000FF000000ULL) <<  8)
         | ((value & 0x0000000000FF0000ULL) << 24)
         | ((value & 0x000000000000FF00ULL) << 40)
         | ((value & 0x00000000000000FFULL) << 56);
}

bool readUInt64(const std::vector<uint8_t>& body, size_t& offset, uint64_t& out) {
    if (offset + 8 > body.size()) {
        return false;
    }

    std::memcpy(&out, body.data() + offset, sizeof(out));
    out = ntoh64(out);
    offset += 8;
    return true;
}

bool readFixedString32(const std::vector<uint8_t>& body, size_t& offset, std::string& out) {
    constexpr size_t kFixedLen = 32;
    if (offset + kFixedLen > body.size()) {
        return false;
    }

    const char* data = reinterpret_cast<const char*>(body.data() + offset);
    size_t actual_len = 0;
    while (actual_len < kFixedLen && data[actual_len] != '\0') {
        ++actual_len;
    }
    out.assign(data, actual_len);
    offset += kFixedLen;
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

std::vector<uint8_t> buildDownloadInit(uint8_t status,
                                       uint64_t size,
                                       const std::string& filename) {
    return PDUBuilder(MessageType::DOWNLOAD_REQUEST)
        .writeUInt8(status)
        .writeUInt64(size)
        .writeFixedString(filename, 32)
        .build();
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

void FileHandler::handleUploadRequest(std::shared_ptr<TcpConnection> conn,
                                      const PDUHeader&,
                                      const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::UPLOAD_REQUEST, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string filename;
    uint64_t file_size = 0;
    std::string dir_path;
    if (!readFixedString32(body, offset, filename) ||
        !readUInt64(body, offset, file_size) ||
        !readString(body, offset, dir_path)) {
        conn->send(buildStatus(MessageType::UPLOAD_REQUEST, kStatusInvalid, "请求格式错误"));
        return;
    }

    if (filename.empty()) {
        conn->send(buildStatus(MessageType::UPLOAD_REQUEST, kStatusInvalid, "文件名不能为空"));
        return;
    }

    {
        std::lock_guard lock(upload_mutex_);
        if (uploads_.count(conn.get()) > 0) {
            conn->send(buildStatus(MessageType::UPLOAD_REQUEST, kStatusConflict, "已有上传任务正在进行"));
            return;
        }
    }

    try {
        const auto target = storage_.prepareUploadTarget(current->username, dir_path, filename);

        if (file_size == 0) {
            std::ofstream empty_stream(target.absolute_path, std::ios::binary | std::ios::trunc);
            if (!empty_stream.is_open()) {
                conn->send(buildStatus(MessageType::UPLOAD_REQUEST, kStatusInternal, "无法创建目标文件"));
                return;
            }
            empty_stream.close();
            conn->send(buildStatus(MessageType::UPLOAD_REQUEST, kStatusOk, "服务端已就绪"));
            conn->send(buildStatus(MessageType::UPLOAD_DATA, kStatusOk, "上传成功"));
            spdlog::info("UPLOAD success: {} -> {}", current->username, target.path);
            return;
        }

        UploadContext ctx;
        ctx.username = current->username;
        ctx.logical_path = target.path;
        ctx.absolute_path = target.absolute_path;
        ctx.expected_size = file_size;
        ctx.received_size = 0;
        ctx.stream.open(target.absolute_path, std::ios::binary | std::ios::trunc);
        if (!ctx.stream.is_open()) {
            conn->send(buildStatus(MessageType::UPLOAD_REQUEST, kStatusInternal, "无法创建目标文件"));
            return;
        }

        {
            std::lock_guard lock(upload_mutex_);
            uploads_[conn.get()] = std::move(ctx);
        }

        conn->send(buildStatus(MessageType::UPLOAD_REQUEST, kStatusOk, "服务端已就绪"));
        spdlog::info("UPLOAD init: user={} file={} size={}",
                     current->username,
                     target.path,
                     file_size);
    } catch (const std::runtime_error& error) {
        const auto [status, message] = mapStorageError(error);
        conn->send(buildStatus(MessageType::UPLOAD_REQUEST, status, message));
    }
}

void FileHandler::handleUploadData(std::shared_ptr<TcpConnection> conn,
                                   const PDUHeader&,
                                   const std::vector<uint8_t>& body) {
    size_t offset = 0;
    uint32_t chunk_size = 0;
    if (!readUInt32(body, offset, chunk_size) || offset + chunk_size != body.size()) {
        conn->send(buildStatus(MessageType::UPLOAD_DATA, kStatusInvalid, "上传分片格式错误"));
        return;
    }

    std::string logical_path;
    bool completed = false;
    {
        std::lock_guard lock(upload_mutex_);
        auto it = uploads_.find(conn.get());
        if (it == uploads_.end()) {
            conn->send(buildStatus(MessageType::UPLOAD_DATA, kStatusInvalid, "没有待接收的上传任务"));
            return;
        }

        auto& ctx = it->second;
        if (ctx.received_size + chunk_size > ctx.expected_size) {
            const auto absolute_path = ctx.absolute_path;
            ctx.stream.close();
            uploads_.erase(it);
            std::error_code ec;
            std::filesystem::remove(absolute_path, ec);
            conn->send(buildStatus(MessageType::UPLOAD_DATA, kStatusInvalid, "上传大小超出预期"));
            return;
        }

        ctx.stream.write(reinterpret_cast<const char*>(body.data() + offset),
                         static_cast<std::streamsize>(chunk_size));
        if (!ctx.stream.good()) {
            const auto absolute_path = ctx.absolute_path;
            ctx.stream.close();
            uploads_.erase(it);
            std::error_code ec;
            std::filesystem::remove(absolute_path, ec);
            conn->send(buildStatus(MessageType::UPLOAD_DATA, kStatusInternal, "写入文件失败"));
            return;
        }

        ctx.received_size += chunk_size;
        logical_path = ctx.logical_path;
        completed = (ctx.received_size == ctx.expected_size);
        if (completed) {
            ctx.stream.close();
            uploads_.erase(it);
        }
    }

    if (completed) {
        conn->send(buildStatus(MessageType::UPLOAD_DATA, kStatusOk, "上传成功"));
        spdlog::info("UPLOAD success: {}", logical_path);
    }
}

void FileHandler::handleDownloadRequest(std::shared_ptr<TcpConnection> conn,
                                        const PDUHeader&,
                                        const std::vector<uint8_t>& body) {
    const auto current = sessions_.findByConnection(conn);
    if (!current) {
        conn->send(buildStatus(MessageType::DOWNLOAD_REQUEST, kStatusUnauthorized, "未登录"));
        return;
    }

    size_t offset = 0;
    std::string file_path;
    if (!readString(body, offset, file_path)) {
        conn->send(buildStatus(MessageType::DOWNLOAD_REQUEST, kStatusInvalid, "请求格式错误"));
        return;
    }

    try {
        const auto entry = storage_.inspectPath(current->username, file_path);
        if (entry.is_dir) {
            conn->send(buildStatus(MessageType::DOWNLOAD_REQUEST, kStatusInvalid, "目录不支持下载"));
            return;
        }

        std::ifstream input(entry.absolute_path, std::ios::binary);
        if (!input.is_open()) {
            conn->send(buildStatus(MessageType::DOWNLOAD_REQUEST, kStatusInternal, "无法读取目标文件"));
            return;
        }

        conn->send(buildDownloadInit(kStatusOk, entry.size, entry.name));

        std::vector<char> buffer(FILE_TRANSFER_CHUNK_SIZE);
        while (input.good()) {
            input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const auto count = input.gcount();
            if (count <= 0) {
                break;
            }
            conn->send(PDUBuilder(MessageType::DOWNLOAD_DATA)
                           .writeUInt32(static_cast<uint32_t>(count))
                           .writeBytes(buffer.data(), static_cast<size_t>(count))
                           .build());
        }

        spdlog::info("DOWNLOAD streamed: user={} file={} size={}",
                     current->username,
                     entry.path,
                     entry.size);
    } catch (const std::runtime_error& error) {
        const auto [status, message] = mapStorageError(error);
        conn->send(buildStatus(MessageType::DOWNLOAD_REQUEST, status, message));
    }
}

void FileHandler::handleDownloadData(std::shared_ptr<TcpConnection> conn,
                                     const PDUHeader&,
                                     const std::vector<uint8_t>& body) {
    if (body.empty()) {
        spdlog::debug("DOWNLOAD ack: empty body from {}", conn->peerAddr());
        return;
    }

    const uint8_t status = body[0];
    std::string message = "客户端未提供状态说明";
    size_t offset = 1;
    readString(body, offset, message);
    spdlog::info("DOWNLOAD ack from {} status={} msg={}",
                 conn->peerAddr(),
                 static_cast<int>(status),
                 message);
}

void FileHandler::handleConnectionClosed(std::shared_ptr<TcpConnection> conn) {
    std::lock_guard lock(upload_mutex_);
    auto it = uploads_.find(conn.get());
    if (it == uploads_.end()) {
        return;
    }

    auto& ctx = it->second;
    ctx.stream.close();
    const bool incomplete = ctx.received_size < ctx.expected_size;
    const auto absolute_path = ctx.absolute_path;
    const auto logical_path = ctx.logical_path;
    uploads_.erase(it);

    if (incomplete) {
        std::error_code ec;
        std::filesystem::remove(absolute_path, ec);
        spdlog::info("UPLOAD aborted and cleaned: {}", logical_path);
    }
}

} // namespace cloudvault
