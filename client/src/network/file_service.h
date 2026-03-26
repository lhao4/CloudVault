// =============================================================
// client/src/network/file_service.h
// 第十一章：文件管理协议封装
// =============================================================

#pragma once

#include "network/response_router.h"
#include "network/tcp_client.h"

#include <QObject>
#include <QList>
#include <QString>

namespace cloudvault {

struct FileEntry {
    QString name;
    QString path;
    bool    is_dir = false;
    quint64 size = 0;
    QString modified_at;
};

using FileEntries = QList<FileEntry>;

class FileService : public QObject {
    Q_OBJECT

public:
    explicit FileService(TcpClient& client,
                         ResponseRouter& router,
                         QObject* parent = nullptr);

    void listFiles(const QString& path);
    void createDirectory(const QString& parent_path, const QString& name);
    void renamePath(const QString& target_path, const QString& new_name);
    void movePath(const QString& source_path, const QString& destination_path);
    void deletePath(const QString& target_path);
    void search(const QString& keyword);

signals:
    void filesListed(const QString& path, const cloudvault::FileEntries& entries);
    void fileListFailed(const QString& reason);
    void searchCompleted(const QString& keyword, const cloudvault::FileEntries& entries);
    void searchFailed(const QString& reason);
    void fileOperationSucceeded(const QString& message);
    void fileOperationFailed(const QString& message);

private:
    void onListResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onMkdirResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onRenameResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onMoveResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onDeleteResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onSearchResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);

    TcpClient&      client_;
    ResponseRouter& router_;

    QString pending_list_path_;
    QString pending_search_keyword_;
};

} // namespace cloudvault

Q_DECLARE_METATYPE(cloudvault::FileEntry)
Q_DECLARE_METATYPE(cloudvault::FileEntries)
