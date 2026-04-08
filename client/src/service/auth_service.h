// =============================================================
// client/src/service/auth_service.h
// 用户认证服务（注册 / 登录 / 登出）
//
// 职责：
//   - 构建 REGISTER_REQUEST / LOGIN_REQUEST PDU 并发送
//   - 解析 REGISTER_RESPONSE / LOGIN_RESPONSE body
//   - 通过信号将结果通知 UI 层
// =============================================================

#pragma once

#include "network/tcp_client.h"
#include "network/response_router.h"
#include "common/crypto_utils.h"

#include <QObject>
#include <QString>

namespace cloudvault {

/**
 * @brief 认证业务服务。
 *
 * 封装注册、登录、登出协议发送与响应解析，
 * 对 UI 层暴露统一信号接口。
 */
class AuthService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造认证服务。
     * @param client TCP 客户端引用。
     * @param router 响应路由器引用。
     * @param parent Qt 父对象。
     */
    explicit AuthService(TcpClient& client,
                         ResponseRouter& router,
                         QObject* parent = nullptr);

    /**
     * @brief 发起注册请求。
     * @param username 用户名。
     * @param password 明文密码（内部会做摘要处理）。
     */
    void registerUser(const QString& username, const QString& password);

    /**
     * @brief 发起登录请求。
     * @param username 用户名。
     * @param password 明文密码（内部会做摘要处理）。
     */
    void login(const QString& username, const QString& password);

    /**
     * @brief 主动发送登出请求。
     */
    void logout();
    void updateProfile(const QString& nickname, const QString& signature);

signals:
    /// @brief 注册成功。
    void registerSuccess();
    /// @brief 注册失败。
    void registerFailed(const QString& reason);

    /// @brief 登录成功。
    void loginSuccess(int userId);
    /// @brief 登录失败。
    void loginFailed(const QString& reason);
    /// @brief 资料更新成功。
    void profileUpdateSuccess();
    /// @brief 资料更新失败。
    void profileUpdateFailed(const QString& reason);

private:
    /**
     * @brief 处理注册响应。
     * @param hdr 响应头。
     * @param body 响应体。
     */
    void onRegisterResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /**
     * @brief 处理登录响应。
     * @param hdr 响应头。
     * @param body 响应体。
     */
    void onLoginResponse   (const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onUpdateProfileResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);

    /// @brief TCP 客户端引用。
    TcpClient&      client_;
    /// @brief 响应路由器引用。
    ResponseRouter& router_;
};

} // namespace cloudvault
