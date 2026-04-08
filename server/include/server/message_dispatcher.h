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

/**
 * @brief 服务端消息分发器。
 *
 * 维护 MessageType 到业务处理函数的映射，
 * 在工作线程中按 PDU 类型将请求派发给对应 Handler。
 */
class MessageDispatcher {
public:
    /**
     * @brief 消息处理函数签名。
     */
    using HandlerFn = std::function<void(
        std::shared_ptr<TcpConnection>,
        const PDUHeader&,
        const std::vector<uint8_t>&)>;

    /**
     * @brief 注册消息处理函数。
     * @param type 消息类型。
     * @param handler 对应处理函数。
     */
    void registerHandler(MessageType type, HandlerFn handler);

    /**
     * @brief 分发一个请求消息。
     * @param conn 请求来源连接。
     * @param header 协议头。
     * @param body 协议体。
     */
    void dispatch(std::shared_ptr<TcpConnection> conn,
                  const PDUHeader&               header,
                  const std::vector<uint8_t>&    body);

private:
    /// @brief 路由表（消息类型 -> 处理器）。
    std::unordered_map<uint32_t, HandlerFn> handlers_;
};

} // namespace cloudvault
