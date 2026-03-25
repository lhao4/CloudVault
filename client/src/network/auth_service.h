// =============================================================
// client/src/network/auth_service.h
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

class AuthService : public QObject {
    Q_OBJECT

public:
    explicit AuthService(TcpClient& client,
                         ResponseRouter& router,
                         QObject* parent = nullptr);

    // 发起注册请求（在 Qt 主线程调用）
    void registerUser(const QString& username, const QString& password);

    // 发起登录请求（在 Qt 主线程调用）
    void login(const QString& username, const QString& password);

signals:
    // 注册结果
    void registerSuccess();
    void registerFailed(const QString& reason);

    // 登录结果
    void loginSuccess(int userId);
    void loginFailed(const QString& reason);

private:
    void onRegisterResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onLoginResponse   (const PDUHeader& hdr, const std::vector<uint8_t>& body);

    TcpClient&      client_;
    ResponseRouter& router_;
};

} // namespace cloudvault
