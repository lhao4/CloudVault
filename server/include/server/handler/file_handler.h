// =============================================================
// server/include/server/handler/file_handler.h
// 第十一章：文件管理业务逻辑
// =============================================================

#pragma once

#include "common/protocol.h"
#include "server/file_storage.h"
#include "server/session_manager.h"

#include <memory>
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

private:
    SessionManager& sessions_;
    FileStorage&    storage_;
};

} // namespace cloudvault
