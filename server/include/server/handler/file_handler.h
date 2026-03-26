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

class FileHandler {
public:
    FileHandler(SessionManager& sessions, FileStorage& storage);

    void handleList(std::shared_ptr<TcpConnection> conn,
                    const PDUHeader& hdr,
                    const std::vector<uint8_t>& body);

    void handleMkdir(std::shared_ptr<TcpConnection> conn,
                     const PDUHeader& hdr,
                     const std::vector<uint8_t>& body);

    void handleRename(std::shared_ptr<TcpConnection> conn,
                      const PDUHeader& hdr,
                      const std::vector<uint8_t>& body);

    void handleMove(std::shared_ptr<TcpConnection> conn,
                    const PDUHeader& hdr,
                    const std::vector<uint8_t>& body);

    void handleDelete(std::shared_ptr<TcpConnection> conn,
                      const PDUHeader& hdr,
                      const std::vector<uint8_t>& body);

    void handleSearch(std::shared_ptr<TcpConnection> conn,
                      const PDUHeader& hdr,
                      const std::vector<uint8_t>& body);

    void handleUploadRequest(std::shared_ptr<TcpConnection> conn,
                             const PDUHeader& hdr,
                             const std::vector<uint8_t>& body);

    void handleUploadData(std::shared_ptr<TcpConnection> conn,
                          const PDUHeader& hdr,
                          const std::vector<uint8_t>& body);

    void handleDownloadRequest(std::shared_ptr<TcpConnection> conn,
                               const PDUHeader& hdr,
                               const std::vector<uint8_t>& body);

    void handleDownloadData(std::shared_ptr<TcpConnection> conn,
                            const PDUHeader& hdr,
                            const std::vector<uint8_t>& body);

    void handleConnectionClosed(std::shared_ptr<TcpConnection> conn);

private:
    struct UploadContext {
        std::string username;
        std::string logical_path;
        std::filesystem::path absolute_path;
        std::ofstream stream;
        uint64_t expected_size = 0;
        uint64_t received_size = 0;
    };

    SessionManager& sessions_;
    FileStorage&    storage_;
    std::mutex upload_mutex_;
    std::unordered_map<const TcpConnection*, UploadContext> uploads_;
};

} // namespace cloudvault
