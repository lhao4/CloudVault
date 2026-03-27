// =============================================================
// client/src/app.h
// 客户端应用装配层
// =============================================================

#pragma once

#include "network/response_router.h"
#include "network/tcp_client.h"
#include "service/auth_service.h"
#include "service/chat_service.h"
#include "service/file_service.h"
#include "service/friend_service.h"
#include "service/share_service.h"

#include <QObject>
#include <QString>
#include <memory>

class LoginWindow;
class MainWindow;
class QWidget;

class App : public QObject {
    Q_OBJECT

public:
    explicit App(QObject* parent = nullptr);
    ~App() override;

    QWidget* initialWindow() const;

private:
    void connectSignals();
    void showMainWindow(const QString& username);
    void handleMainWindowClosed();
    void handleLogoutRequested();
    void handleSocketConnected();
    void handleSocketDisconnected();
    void handleSocketError(const QString& message);

    cloudvault::TcpClient tcp_client_;
    cloudvault::ResponseRouter router_;
    cloudvault::AuthService auth_service_;
    cloudvault::ChatService chat_service_;
    cloudvault::FriendService friend_service_;
    cloudvault::FileService file_service_;
    cloudvault::ShareService share_service_;

    std::unique_ptr<LoginWindow> login_window_;
    std::unique_ptr<MainWindow> main_window_;
    bool reconnect_suppressed_ = false;
    bool reconnect_pending_ = false;
};
