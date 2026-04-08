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

/**
 * @brief 客户端 PDU 响应分发器。
 *
 * 维护 MessageType 到处理函数的映射关系，供各 Service 注册。
 * 当 TcpClient 收到完整 PDU 后，由该路由器按类型分发到对应处理器。
 */
class ResponseRouter {
public:
    /**
     * @brief 消息处理函数签名。
     *
     * 处理函数在 Qt 主线程执行，可以安全更新 UI。
     */
    using HandlerFn = std::function<void(
        const PDUHeader&,
        const std::vector<uint8_t>&)>;

    /**
     * @brief 注册某个消息类型的处理函数。
     * @param type 消息类型。
     * @param handler 处理函数。
     */
    void registerHandler(MessageType type, HandlerFn handler);

    /**
     * @brief 分发收到的 PDU。
     * @param header 协议头。
     * @param body 协议体字节。
     */
    void dispatch(const PDUHeader&            header,
                  const std::vector<uint8_t>& body);

private:
    /// @brief 消息处理表，key 为 MessageType 底层值。
    std::unordered_map<uint32_t, HandlerFn> handlers_;
};

} // namespace cloudvault
