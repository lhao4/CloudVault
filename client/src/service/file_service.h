// =============================================================
// client/src/service/file_service.h
// 第十一章：文件管理协议封装
// =============================================================

#pragma once

#include "network/response_router.h"
#include "network/tcp_client.h"

#include <QObject>
#include <QFile>
#include <QList>
#include <QString>

namespace cloudvault {

/**
 * @brief 文件条目模型。
 */
struct FileEntry {
    /// @brief 文件/目录名称。
    QString name;
    /// @brief 服务器端完整路径。
    QString path;
    /// @brief 是否目录。
    bool    is_dir = false;
    /// @brief 文件大小（字节）。
    quint64 size = 0;
    /// @brief 修改时间文本。
    QString modified_at;
};

/// @brief 文件条目列表别名。
using FileEntries = QList<FileEntry>;

/**
 * @brief 文件业务服务。
 *
 * 封装目录浏览、搜索、增删改、上传下载等协议交互，
 * 并维护单路上传/下载上下文状态。
 */
class FileService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造文件服务。
     * @param client TCP 客户端引用。
     * @param router 响应路由器引用。
     * @param parent Qt 父对象。
     */
    explicit FileService(TcpClient& client,
                         ResponseRouter& router,
                         QObject* parent = nullptr);

    /**
     * @brief 列出目录内容。
     * @param path 目录路径。
     */
    void listFiles(const QString& path);
    /**
     * @brief 创建目录。
     * @param parent_path 父目录路径。
     * @param name 新目录名称。
     */
    void createDirectory(const QString& parent_path, const QString& name);
    /**
     * @brief 重命名路径。
     * @param target_path 目标路径。
     * @param new_name 新名称。
     */
    void renamePath(const QString& target_path, const QString& new_name);
    /**
     * @brief 移动路径。
     * @param source_path 源路径。
     * @param destination_path 目标目录路径。
     */
    void movePath(const QString& source_path, const QString& destination_path);
    /**
     * @brief 删除路径。
     * @param target_path 目标路径。
     */
    void deletePath(const QString& target_path);
    /**
     * @brief 搜索文件。
     * @param keyword 搜索关键词。
     */
    void search(const QString& keyword);
    /**
     * @brief 上传本地文件到远端目录。
     * @param local_path 本地文件路径。
     * @param remote_dir_path 远端目录路径。
     */
    void uploadFile(const QString& local_path, const QString& remote_dir_path);
    /**
     * @brief 下载远端文件到本地保存路径。
     * @param remote_file_path 远端文件路径。
     * @param local_save_path 本地保存路径。
     */
    void downloadFile(const QString& remote_file_path, const QString& local_save_path);
    /**
     * @brief 是否存在活动中的传输任务。
     * @return true 表示上传或下载进行中。
     */
    bool hasActiveTransfer() const;

signals:
    /// @brief 文件列表加载成功。
    void filesListed(const QString& path, const cloudvault::FileEntries& entries);
    /// @brief 文件列表加载失败。
    void fileListFailed(const QString& reason);
    /// @brief 搜索成功。
    void searchCompleted(const QString& keyword, const cloudvault::FileEntries& entries);
    /// @brief 搜索失败。
    void searchFailed(const QString& reason);
    /// @brief 文件操作成功。
    void fileOperationSucceeded(const QString& message);
    /// @brief 文件操作失败。
    void fileOperationFailed(const QString& message);
    /// @brief 上传进度。
    void uploadProgress(const QString& filename, quint64 sent_bytes, quint64 total_bytes);
    /// @brief 上传完成。
    void uploadFinished(const QString& remote_path, const QString& message);
    /// @brief 上传失败。
    void uploadFailed(const QString& message);
    /// @brief 下载进度。
    void downloadProgress(const QString& filename, quint64 received_bytes, quint64 total_bytes);
    /// @brief 下载完成。
    void downloadFinished(const QString& local_path, const QString& message);
    /// @brief 下载失败。
    void downloadFailed(const QString& message);

private:
    /// @brief 处理目录列表响应。
    void onListResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理创建目录响应。
    void onMkdirResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理重命名响应。
    void onRenameResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理移动响应。
    void onMoveResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理删除响应。
    void onDeleteResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理搜索响应。
    void onSearchResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理上传初始化响应。
    void onUploadInitResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理上传结果响应。
    void onUploadResult(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理下载初始化响应。
    void onDownloadInitResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理下载数据块。
    void onDownloadChunk(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 继续发送下一块上传数据。
    void uploadNextChunk();
    /// @brief 重置上传上下文。
    void resetUploadContext();
    /// @brief 重置下载上下文。
    void resetDownloadContext(bool remove_partial_file);

    /// @brief TCP 客户端引用。
    TcpClient&      client_;
    /// @brief 响应路由器引用。
    ResponseRouter& router_;

    /**
     * @brief 上传过程上下文。
     */
    struct UploadContext {
        /// @brief 本地文件对象。
        QFile file;
        /// @brief 本地文件路径。
        QString local_path;
        /// @brief 远端目录路径。
        QString remote_dir_path;
        /// @brief 文件名。
        QString filename;
        /// @brief 远端完整路径。
        QString remote_file_path;
        /// @brief 文件总字节数。
        quint64 total_bytes = 0;
        /// @brief 已发送字节数。
        quint64 sent_bytes = 0;
        /// @brief 是否等待服务端确认。
        bool waiting_for_server = false;
    } upload_;

    /**
     * @brief 下载过程上下文。
     */
    struct DownloadContext {
        /// @brief 本地写入文件对象。
        QFile file;
        /// @brief 远端文件路径。
        QString remote_file_path;
        /// @brief 用户选择的保存路径。
        QString local_save_path;
        /// @brief 实际本地输出路径。
        QString local_file_path;
        /// @brief 文件名。
        QString filename;
        /// @brief 文件总字节数。
        quint64 total_bytes = 0;
        /// @brief 已接收字节数。
        quint64 received_bytes = 0;
        /// @brief 是否等待初始化响应。
        bool waiting_for_init = false;
        /// @brief 下载任务是否激活。
        bool active = false;
    } download_;
};

} // namespace cloudvault

Q_DECLARE_METATYPE(cloudvault::FileEntry)
Q_DECLARE_METATYPE(cloudvault::FileEntries)
