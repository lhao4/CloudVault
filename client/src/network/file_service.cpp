// =============================================================
// client/src/network/file_service.cpp
// 第十一、十二章：文件管理与文件传输协议封装
// =============================================================

#include "network/file_service.h"

#include "common/protocol.h"
#include "common/protocol_codec.h"

#include <QDir>
#include <QFileInfo>
#include <QMetaType>
#include <QTimer>

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

bool readUInt64(const std::vector<uint8_t>& body, size_t& offset, uint64_t& out) {
    if (offset + 8 > body.size()) {
        return false;
    }
    std::memcpy(&out, body.data() + offset, sizeof(out));
    out = ntoh64(out);
    offset += 8;
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

bool readFixedString32(const std::vector<uint8_t>& body, size_t& offset, QString& out) {
    constexpr size_t kFixedLen = 32;
    if (offset + kFixedLen > body.size()) {
        return false;
    }

    const char* data = reinterpret_cast<const char*>(body.data() + offset);
    size_t actual_len = 0;
    while (actual_len < kFixedLen && data[actual_len] != '\0') {
        ++actual_len;
    }
    out = QString::fromUtf8(data, static_cast<int>(actual_len));
    offset += kFixedLen;
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
    if (offset + 2 > body.size()) {
        return false;
    }
    std::memcpy(&count, body.data() + offset, sizeof(count));
    count = ntohs(count);
    offset += 2;

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

QString uniqueFilePath(const QString& directory, const QString& filename) {
    const QFileInfo info(filename);
    const QString complete_name = info.fileName().isEmpty() ? QStringLiteral("download.bin")
                                                            : info.fileName();
    const QString base = QFileInfo(complete_name).completeBaseName();
    const QString suffix = QFileInfo(complete_name).suffix();

    QDir dir(directory);
    QString candidate = dir.filePath(complete_name);
    if (!QFileInfo::exists(candidate)) {
        return candidate;
    }

    for (int i = 1; i < 1000; ++i) {
        const QString numbered = suffix.isEmpty()
            ? QStringLiteral("%1 (%2)").arg(base).arg(i)
            : QStringLiteral("%1 (%2).%3").arg(base).arg(i).arg(suffix);
        candidate = dir.filePath(numbered);
        if (!QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return dir.filePath(complete_name);
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
    router_.registerHandler(MessageType::UPLOAD_REQUEST,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onUploadInitResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::UPLOAD_DATA,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onUploadResult(hdr, body);
                            });
    router_.registerHandler(MessageType::DOWNLOAD_REQUEST,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onDownloadInitResponse(hdr, body);
                            });
    router_.registerHandler(MessageType::DOWNLOAD_DATA,
                            [this](const PDUHeader& hdr, const std::vector<uint8_t>& body) {
                                onDownloadChunk(hdr, body);
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

void FileService::uploadFile(const QString& local_path, const QString& remote_dir_path) {
    if (hasActiveTransfer()) {
        emit uploadFailed(QStringLiteral("当前已有文件传输任务正在进行"));
        return;
    }

    const QFileInfo info(local_path);
    if (!info.exists() || !info.isFile()) {
        emit uploadFailed(QStringLiteral("本地文件不存在"));
        return;
    }

    const QString filename = info.fileName();
    if (filename.toUtf8().size() > 32) {
        emit uploadFailed(QStringLiteral("文件名过长，当前协议要求不超过 32 字节"));
        return;
    }

    upload_.file.setFileName(local_path);
    if (!upload_.file.open(QIODevice::ReadOnly)) {
        emit uploadFailed(QStringLiteral("无法读取本地文件"));
        resetUploadContext();
        return;
    }

    upload_.local_path = local_path;
    upload_.remote_dir_path = remote_dir_path.trimmed().isEmpty()
        ? QStringLiteral("/")
        : remote_dir_path.trimmed();
    upload_.filename = filename;
    upload_.remote_file_path = upload_.remote_dir_path;
    if (!upload_.remote_file_path.endsWith('/')) {
        upload_.remote_file_path += '/';
    }
    upload_.remote_file_path += filename;
    upload_.total_bytes = static_cast<quint64>(info.size());
    upload_.sent_bytes = 0;
    upload_.waiting_for_server = true;

    client_.send(PDUBuilder(MessageType::UPLOAD_REQUEST)
                     .writeFixedString(filename.toStdString(), 32)
                     .writeUInt64(upload_.total_bytes)
                     .writeString(upload_.remote_dir_path.toStdString())
                     .build());
}

void FileService::downloadFile(const QString& remote_file_path, const QString& local_dir_path) {
    if (hasActiveTransfer()) {
        emit downloadFailed(QStringLiteral("当前已有文件传输任务正在进行"));
        return;
    }

    if (remote_file_path.trimmed().isEmpty()) {
        emit downloadFailed(QStringLiteral("下载路径不能为空"));
        return;
    }

    QDir dir(local_dir_path);
    if (!dir.exists()) {
        emit downloadFailed(QStringLiteral("本地保存目录不存在"));
        return;
    }

    download_.remote_file_path = remote_file_path.trimmed();
    download_.local_dir_path = dir.absolutePath();
    download_.waiting_for_init = true;
    download_.active = false;
    download_.received_bytes = 0;
    download_.total_bytes = 0;
    download_.filename.clear();
    download_.local_file_path.clear();

    client_.send(PDUBuilder(MessageType::DOWNLOAD_REQUEST)
                     .writeString(download_.remote_file_path.toStdString())
                     .build());
}

bool FileService::hasActiveTransfer() const {
    return upload_.waiting_for_server
        || upload_.file.isOpen()
        || download_.waiting_for_init
        || download_.active;
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

void FileService::onUploadInitResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (!upload_.waiting_for_server) {
        return;
    }
    if (body.empty()) {
        emit uploadFailed(QStringLiteral("服务器返回空响应"));
        resetUploadContext();
        return;
    }

    const uint8_t status = body[0];
    const QString message = readMessage(body, 1);
    if (status != kStatusOk) {
        emit uploadFailed(message);
        resetUploadContext();
        return;
    }

    upload_.waiting_for_server = false;
    emit uploadProgress(upload_.filename, 0, upload_.total_bytes);
    if (upload_.total_bytes == 0) {
        return;
    }

    QTimer::singleShot(0, this, &FileService::uploadNextChunk);
}

void FileService::onUploadResult(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (!upload_.file.isOpen() && !upload_.filename.isEmpty() && !upload_.waiting_for_server) {
        // allow final result after the last chunk has already been sent and file closed
    } else if (upload_.filename.isEmpty()) {
        return;
    }

    if (body.empty()) {
        emit uploadFailed(QStringLiteral("服务器返回空响应"));
        resetUploadContext();
        return;
    }

    const uint8_t status = body[0];
    const QString message = readMessage(body, 1);
    const QString remote_path = upload_.remote_file_path;
    if (status == kStatusOk) {
        emit uploadFinished(remote_path, message);
    } else {
        emit uploadFailed(message);
    }
    resetUploadContext();
}

void FileService::onDownloadInitResponse(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (!download_.waiting_for_init) {
        return;
    }
    if (body.empty()) {
        emit downloadFailed(QStringLiteral("服务器返回空响应"));
        resetDownloadContext(false);
        return;
    }

    size_t offset = 0;
    const uint8_t status = body[offset++];
    if (status != kStatusOk) {
        emit downloadFailed(readMessage(body, offset));
        resetDownloadContext(false);
        return;
    }

    uint64_t total_bytes = 0;
    QString filename;
    if (!readUInt64(body, offset, total_bytes) || !readFixedString32(body, offset, filename)) {
        emit downloadFailed(QStringLiteral("下载初始化响应格式错误"));
        resetDownloadContext(false);
        return;
    }

    if (filename.isEmpty()) {
        filename = QFileInfo(download_.remote_file_path).fileName();
    }

    download_.filename = filename;
    download_.total_bytes = total_bytes;
    download_.local_file_path = uniqueFilePath(download_.local_dir_path, filename);
    download_.file.setFileName(download_.local_file_path);
    if (!download_.file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        client_.send(PDUBuilder(MessageType::DOWNLOAD_DATA)
                         .writeUInt8(1)
                         .writeString(QStringLiteral("无法写入本地文件").toStdString())
                         .build());
        emit downloadFailed(QStringLiteral("无法写入本地文件"));
        resetDownloadContext(false);
        return;
    }

    download_.waiting_for_init = false;
    download_.active = true;
    download_.received_bytes = 0;
    emit downloadProgress(download_.filename, 0, download_.total_bytes);

    if (download_.total_bytes == 0) {
        download_.file.close();
        client_.send(PDUBuilder(MessageType::DOWNLOAD_DATA)
                         .writeUInt8(0)
                         .writeString(QStringLiteral("下载成功").toStdString())
                         .build());
        emit downloadFinished(download_.local_file_path, QStringLiteral("下载成功"));
        resetDownloadContext(false);
    }
}

void FileService::onDownloadChunk(const PDUHeader&, const std::vector<uint8_t>& body) {
    if (!download_.active) {
        return;
    }

    size_t offset = 0;
    uint32_t chunk_size = 0;
    if (!readUInt32(body, offset, chunk_size) || offset + chunk_size != body.size()) {
        client_.send(PDUBuilder(MessageType::DOWNLOAD_DATA)
                         .writeUInt8(1)
                         .writeString(QStringLiteral("下载数据格式错误").toStdString())
                         .build());
        emit downloadFailed(QStringLiteral("下载数据格式错误"));
        resetDownloadContext(true);
        return;
    }

    if (download_.file.write(reinterpret_cast<const char*>(body.data() + offset),
                             static_cast<qint64>(chunk_size)) != static_cast<qint64>(chunk_size)) {
        client_.send(PDUBuilder(MessageType::DOWNLOAD_DATA)
                         .writeUInt8(1)
                         .writeString(QStringLiteral("本地写入失败").toStdString())
                         .build());
        emit downloadFailed(QStringLiteral("本地写入失败"));
        resetDownloadContext(true);
        return;
    }

    download_.received_bytes += chunk_size;
    if (download_.received_bytes > download_.total_bytes) {
        client_.send(PDUBuilder(MessageType::DOWNLOAD_DATA)
                         .writeUInt8(1)
                         .writeString(QStringLiteral("下载大小超出预期").toStdString())
                         .build());
        emit downloadFailed(QStringLiteral("下载大小超出预期"));
        resetDownloadContext(true);
        return;
    }

    emit downloadProgress(download_.filename,
                          download_.received_bytes,
                          download_.total_bytes);

    if (download_.received_bytes == download_.total_bytes) {
        download_.file.close();
        client_.send(PDUBuilder(MessageType::DOWNLOAD_DATA)
                         .writeUInt8(0)
                         .writeString(QStringLiteral("下载成功").toStdString())
                         .build());
        emit downloadFinished(download_.local_file_path, QStringLiteral("下载成功"));
        resetDownloadContext(false);
    }
}

void FileService::uploadNextChunk() {
    if (upload_.waiting_for_server || !upload_.file.isOpen()) {
        return;
    }

    const QByteArray chunk = upload_.file.read(static_cast<qint64>(FILE_TRANSFER_CHUNK_SIZE));
    if (chunk.isEmpty()) {
        upload_.file.close();
        return;
    }

    upload_.sent_bytes += static_cast<quint64>(chunk.size());
    client_.send(PDUBuilder(MessageType::UPLOAD_DATA)
                     .writeUInt32(static_cast<uint32_t>(chunk.size()))
                     .writeBytes(chunk.constData(), static_cast<size_t>(chunk.size()))
                     .build());
    emit uploadProgress(upload_.filename, upload_.sent_bytes, upload_.total_bytes);

    if (upload_.sent_bytes < upload_.total_bytes) {
        QTimer::singleShot(0, this, &FileService::uploadNextChunk);
    } else {
        upload_.file.close();
    }
}

void FileService::resetUploadContext() {
    if (upload_.file.isOpen()) {
        upload_.file.close();
    }
    upload_.file.setFileName(QString());
    upload_.local_path.clear();
    upload_.remote_dir_path.clear();
    upload_.filename.clear();
    upload_.remote_file_path.clear();
    upload_.total_bytes = 0;
    upload_.sent_bytes = 0;
    upload_.waiting_for_server = false;
}

void FileService::resetDownloadContext(bool remove_partial_file) {
    const QString partial_path = download_.local_file_path;
    if (download_.file.isOpen()) {
        download_.file.close();
    }
    download_.file.setFileName(QString());
    download_.remote_file_path.clear();
    download_.local_dir_path.clear();
    download_.local_file_path.clear();
    download_.filename.clear();
    download_.total_bytes = 0;
    download_.received_bytes = 0;
    download_.waiting_for_init = false;
    download_.active = false;

    if (remove_partial_file && !partial_path.isEmpty()) {
        QFile::remove(partial_path);
    }
}

} // namespace cloudvault
