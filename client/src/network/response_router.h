// =============================================================
// client/src/network/response_router.h
// 客户端 PDU 消息路由分发器
//
// 职责：维护 MessageType → HandlerFn 路由表，
//       根据收到的 PDU header 调用对应处理函数。
//       与服务端 MessageDispatcher 逻辑相同，但在 Qt 主线程中执行。
// =============================================================

#pragma once

#include "common/protocol.h"

#include <functional>
#include <unordered_map>
#include <vector>

namespace cloudvault {

class ResponseRouter {
public:
    // handler 在 Qt 主线程中调用，可以直接操作 UI
    using HandlerFn = std::function<void(
        const PDUHeader&,
        const std::vector<uint8_t>&)>;

    void registerHandler(MessageType type, HandlerFn handler);

    void dispatch(const PDUHeader&            header,
                  const std::vector<uint8_t>& body);

private:
    std::unordered_map<uint32_t, HandlerFn> handlers_;
};

} // namespace cloudvault
