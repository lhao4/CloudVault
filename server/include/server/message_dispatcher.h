// =============================================================
// server/include/server/message_dispatcher.h
// PDU 消息路由分发器
//
// 职责：维护 MessageType → HandlerFn 路由表，
//       根据 PDU header 调用对应处理函数
// =============================================================

#pragma once

#include "common/protocol.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace cloudvault {

class TcpConnection;

class MessageDispatcher {
public:
    using HandlerFn = std::function<void(
        std::shared_ptr<TcpConnection>,
        const PDUHeader&,
        const std::vector<uint8_t>&)>;

    void registerHandler(MessageType type, HandlerFn handler);

    void dispatch(std::shared_ptr<TcpConnection> conn,
                  const PDUHeader&               header,
                  const std::vector<uint8_t>&    body);

private:
    std::unordered_map<uint32_t, HandlerFn> handlers_;
};

} // namespace cloudvault
