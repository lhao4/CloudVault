// =============================================================
// client/src/network/response_router.cpp
// 客户端 PDU 消息路由分发器实现
// =============================================================

#include "response_router.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcRouter, "network.router")

namespace cloudvault {

void ResponseRouter::registerHandler(MessageType type, HandlerFn handler) {
    handlers_[static_cast<uint32_t>(type)] = std::move(handler);
}

void ResponseRouter::dispatch(const PDUHeader&            header,
                              const std::vector<uint8_t>& body)
{
    auto it = handlers_.find(header.message_type);
    if (it != handlers_.end()) {
        it->second(header, body);
    } else {
        qCWarning(lcRouter, "No handler for message_type=0x%04X",
                  header.message_type);
    }
}

} // namespace cloudvault
