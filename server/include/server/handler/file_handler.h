// =============================================================
// server/include/server/handler/file_handler.h
// 第十一章：文件管理业务逻辑
// =============================================================

#pragma once

#include "common/protocol.h"
#include "server/file_storage.h"
#include "server/session_manager.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace cloudvault {

class TcpConnection;

/**
 * @brief 文件业务处理器。
 *
 * 负责目录操作、搜索、上传下载协议处理，
 * 并维护连接级上传上下文。
 */
class FileHandler {
public:
    /**
     * @brief 构造文件处理器。
     * @param sessions 会话管理器引用。
     * @param storage 文件存储抽象引用。
     */
    FileHandler(SessionManager& sessions, FileStorage& storage);

    /// @brief 处理目录列表请求。
    void handleList(std::shared_ptr<TcpConnection> conn,
                    const PDUHeader& hdr,
                    const std::vector<uint8_t>& body);

    /// @brief 处理创建目录请求。
    void handleMkdir(std::shared_ptr<TcpConnection> conn,
                     const PDUHeader& hdr,
                     const std::vector<uint8_t>& body);

    /// @brief 处理重命名请求。
    void handleRename(std::shared_ptr<TcpConnection> conn,
                      const PDUHeader& hdr,
                      const std::vector<uint8_t>& body);

    /// @brief 处理移动请求。
    void handleMove(std::shared_ptr<TcpConnection> conn,
                    const PDUHeader& hdr,
                    const std::vector<uint8_t>& body);

    /// @brief 处理删除请求。
    void handleDelete(std::shared_ptr<TcpConnection> conn,
                      const PDUHeader& hdr,
                      const std::vector<uint8_t>& body);

    /// @brief 处理搜索请求。
    void handleSearch(std::shared_ptr<TcpConnection> conn,
                      const PDUHeader& hdr,
                      const std::vector<uint8_t>& body);

    /// @brief 处理上传初始化请求。
    void handleUploadRequest(std::shared_ptr<TcpConnection> conn,
                             const PDUHeader& hdr,
                             const std::vector<uint8_t>& body);

    /// @brief 处理上传数据块请求。
    void handleUploadData(std::shared_ptr<TcpConnection> conn,
                          const PDUHeader& hdr,
                          const std::vector<uint8_t>& body);

    /// @brief 处理下载初始化请求。
    void handleDownloadRequest(std::shared_ptr<TcpConnection> conn,
                               const PDUHeader& hdr,
                               const std::vector<uint8_t>& body);

    /// @brief 处理下载数据块请求。
    void handleDownloadData(std::shared_ptr<TcpConnection> conn,
                            const PDUHeader& hdr,
                            const std::vector<uint8_t>& body);

    /**
     * @brief 连接关闭时清理上传上下文。
     * @param conn 关闭连接。
     */
    void handleConnectionClosed(std::shared_ptr<TcpConnection> conn);

private:
    /**
     * @brief 单连接上传状态。
     */
    struct UploadContext {
        /// @brief 上传所属用户名。
        std::string username;
        /// @brief 目标逻辑路径。
        std::string logical_path;
        /// @brief 目标绝对路径。
        std::filesystem::path absolute_path;
        /// @brief 上传文件输出流。
        std::ofstream stream;
        /// @brief 期望总大小。
        uint64_t expected_size = 0;
        /// @brief 已接收大小。
        uint64_t received_size = 0;
    };

    /// @brief 会话管理器引用。
    SessionManager& sessions_;
    /// @brief 文件存储抽象引用。
    FileStorage&    storage_;
    /// @brief 上传上下文锁。
    std::mutex upload_mutex_;
    /// @brief 连接指针到上传上下文映射。
    std::unordered_map<const TcpConnection*, UploadContext> uploads_;
};

} // namespace cloudvault
