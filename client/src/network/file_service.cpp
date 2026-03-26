// =============================================================
// client/src/network/file_service.cpp
// 第十一章：文件管理协议封装
// =============================================================

#include "network/file_service.h"

#include "common/protocol.h"
#include "common/protocol_codec.h"

#include <QMetaType>
#include <cstring>
#include <functional>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

namespace cloudvault {

namespace {

constexpr uint8_t kStatusOk = 0;

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

bool readString(const std::vector<uint8_t>& body, size_t& offset, QString& out) {
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

    out = QString::fromUtf8(reinterpret_cast<const char*>(body.data() + offset),
                            static_cast<int>(len));
    offset += len;
    return true;
}

bool readUInt16(const std::vector<uint8_t>& body, size_t& offset, uint16_t& out) {
    if (offset + 2 > body.size()) {
        return false;
    }
    std::memcpy(&out, body.data() + offset, sizeof(out));
    out = ntohs(out);
    offset += 2;
    return true;
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

QString readMessage(const std::vector<uint8_t>& body, size_t offset) {
    QString message;
    if (readString(body, offset, message)) {
        return message;
    }
    return QStringLiteral("服务器返回格式错误");
}

bool parseFileEntriesSafe(const std::vector<uint8_t>& body,
                          size_t& offset,
                          FileEntries& entries) {
    uint16_t count = 0;
    if (!readUInt16(body, offset, count)) {
        return false;
    }

    entries.clear();
    for (uint16_t i = 0; i < count; ++i) {
        FileEntry entry;
        uint64_t size = 0;
        if (!readString(body, offset, entry.name) ||
            !readString(body, offset, entry.path) ||
            offset >= body.size()) {
            return false;
        }
        entry.is_dir = body[offset++] != 0;
        if (!readUInt64(body, offset, size) ||
            !readString(body, offset, entry.modified_at)) {
            return false;
        }
        entry.size = size;
        entries.append(entry);
    }
    return true;
}

void emitOperationResult(const std::vector<uint8_t>& body,
                         const std::function<void(const QString&)>& on_success,
                         const std::function<void(const QString&)>& on_failure) {
    if (body.empty()) {
        on_failure(QStringLiteral("服务器返回空响应"));
        return;
    }

    const uint8_t status = body[0];
    const QString message = readMessage(body, 1);
    if (status == kStatusOk) {
        on_success(message);
    } else {
        on_failure(message);
    }
}

} // namespace

FileService::FileService(TcpClient& client,
                         ResponseRouter& router,
                         QObject* parent)
    : QObject(parent), client_(client), router_(router) {
    qRegisterMetaType<cloudvault::FileEntry>("cloudvault::FileEntry");
    qRegisterMetaType<cloudvault::FileEntries>("cloudvault::FileEntries");

    router_.registerHandler(MessageType::FLUSH_FILE,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onListResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::MKDIR,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onMkdirResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::RENAME,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onRenameResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::MOVE,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onMoveResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::DELETE_FILE,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onDeleteResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::SEARCH_FILE,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onSearchResponse(hdr, body);
                            });
}

void FileService::listFiles(const QString& path) {
    pending_list_path_ = path.trimmed();
    client_.send(PDUBuilder(MessageType::FLUSH_FILE)
                     .writeString(pending_list_path_.toStdString())
                     .build());
}

void FileService::createDirectory(const QString& parent_path, const QString& name) {
    client_.send(PDUBuilder(MessageType::MKDIR)
                     .writeString(name.trimmed().toStdString())
                     .writeString(parent_path.trimmed().toStdString())
                     .build());
}

void FileService::renamePath(const QString& target_path, const QString& new_name) {
    client_.send(PDUBuilder(MessageType::RENAME)
                     .writeString(target_path.trimmed().toStdString())
                     .writeString(new_name.trimmed().toStdString())
                     .build());
}

void FileService::movePath(const QString& source_path, const QString& destination_path) {
    client_.send(PDUBuilder(MessageType::MOVE)
                     .writeString(source_path.trimmed().toStdString())
                     .writeString(destination_path.trimmed().toStdString())
                     .build());
}

void FileService::deletePath(const QString& target_path) {
    client_.send(PDUBuilder(MessageType::DELETE_FILE)
                     .writeString(target_path.trimmed().toStdString())
                     .build());
}

void FileService::search(const QString& keyword) {
    pending_search_keyword_ = keyword.trimmed();
    client_.send(PDUBuilder(MessageType::SEARCH_FILE)
                     .writeString(pending_search_keyword_.toStdString())
                     .build());
}

void FileService::onListResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit fileListFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    if (status != kStatusOk) {
        emit fileListFailed(readMessage(body, offset));
        return;
    }

    QString path;
    if (!readString(body, offset, path)) {
        emit fileListFailed(QStringLiteral("文件列表响应格式错误"));
        return;
    }

    FileEntries entries;
    if (!parseFileEntriesSafe(body, offset, entries)) {
        emit fileListFailed(QStringLiteral("文件列表响应格式错误"));
        return;
    }

    emit filesListed(path, entries);
}

void FileService::onMkdirResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    emitOperationResult(body,
                        [this](const QString& message) { emit fileOperationSucceeded(message); },
                        [this](const QString& message) { emit fileOperationFailed(message); });
}

void FileService::onRenameResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    emitOperationResult(body,
                        [this](const QString& message) { emit fileOperationSucceeded(message); },
                        [this](const QString& message) { emit fileOperationFailed(message); });
}

void FileService::onMoveResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    emitOperationResult(body,
                        [this](const QString& message) { emit fileOperationSucceeded(message); },
                        [this](const QString& message) { emit fileOperationFailed(message); });
}

void FileService::onDeleteResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    emitOperationResult(body,
                        [this](const QString& message) { emit fileOperationSucceeded(message); },
                        [this](const QString& message) { emit fileOperationFailed(message); });
}

void FileService::onSearchResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (body.empty()) {
        emit searchFailed(QStringLiteral("服务器返回空响应"));
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    if (status != kStatusOk) {
        emit searchFailed(readMessage(body, offset));
        return;
    }

    QString echoed_keyword;
    if (!readString(body, offset, echoed_keyword)) {
        emit searchFailed(QStringLiteral("搜索响应格式错误"));
        return;
    }

    FileEntries entries;
    if (!parseFileEntriesSafe(body, offset, entries)) {
        emit searchFailed(QStringLiteral("搜索响应格式错误"));
        return;
    }

    emit searchCompleted(echoed_keyword, entries);
}

} // namespace cloudvault
