// =============================================================
// server/src/message_dispatcher.cpp
// PDU 消息路由分发器实现
// =============================================================

#include "server/message_dispatcher.h"
#include "server/tcp_connection.h"

#include <spdlog/spdlog.h>

namespace cloudvault {

void MessageDispatcher::registerHandler(MessageType type, HandlerFn handler) {
    handlers_[static_cast<uint32_t>(type)] = std::move(handler);
}

void MessageDispatcher::dispatch(std::shared_ptr<TcpConnection> conn,
                                 const PDUHeader&               header,
                                 const std::vector<uint8_t>&    body)
{
    auto it = handlers_.find(header.message_type);
    if (it != handlers_.end()) {
        it->second(conn, header, body);
    } else {
        spdlog::warn("No handler for message_type=0x{:04X}", header.message_type);
    }
}

} // namespace cloudvault
