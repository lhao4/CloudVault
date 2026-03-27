// =============================================================
// client/src/app.cpp
// 客户端应用装配层实现
// =============================================================

#include "app.h"

#include "ui/login_window.h"
#include "ui/main_window.h"

#include <QCoreApplication>
#include <QTimer>

App::App(QObject* parent)
    : QObject(parent)
    , auth_service_(tcp_client_, router_, this)
    , chat_service_(tcp_client_, router_, this)
    , friend_service_(tcp_client_, router_, this)
    , file_service_(tcp_client_, router_, this)
    , share_service_(tcp_client_, router_, this)
{
    login_window_ = std::make_unique<LoginWindow>(tcp_client_, router_, auth_service_);
    connectSignals();
}

App::~App() = default;

QWidget* App::initialWindow() const {
    return login_window_.get();
}

void App::connectSignals() {
    connect(login_window_.get(), &LoginWindow::loginSucceeded,
            this, &App::showMainWindow);
    connect(&tcp_client_, &cloudvault::TcpClient::connected,
            this, &App::handleSocketConnected);
    connect(&tcp_client_, &cloudvault::TcpClient::disconnected,
            this, &App::handleSocketDisconnected);
    connect(&tcp_client_, &cloudvault::TcpClient::errorOccurred,
            this, &App::handleSocketError);
}

void App::showMainWindow(const QString& username) {
    reconnect_suppressed_ = false;
    reconnect_pending_ = false;
    main_window_.reset();
    main_window_ = std::make_unique<MainWindow>(
        username, chat_service_, friend_service_, file_service_, share_service_);

    connect(main_window_.get(), &MainWindow::windowClosed,
            this, &App::handleMainWindowClosed);
    connect(main_window_.get(), &MainWindow::logoutRequested,
            this, &App::handleLogoutRequested);

    if (login_window_) {
        login_window_->hide();
    }

    main_window_->show();
    main_window_->raise();
    main_window_->activateWindow();
    main_window_->hideConnectionBanner();
    main_window_->appendEventLog(QStringLiteral("用户 %1 登录成功").arg(username), QStringLiteral("✓"));
    friend_service_.flushFriends();
}

void App::handleMainWindowClosed() {
    reconnect_suppressed_ = true;
    auth_service_.logout();
    tcp_client_.disconnectFromServer();
    QCoreApplication::quit();
}

void App::handleLogoutRequested() {
    reconnect_suppressed_ = true;
    reconnect_pending_ = false;
    if (main_window_) {
        main_window_->hide();
        main_window_.reset();
    }

    auth_service_.logout();
    tcp_client_.disconnectFromServer();

    if (login_window_) {
        login_window_->handleLoggedOut();
    }
}

void App::handleSocketConnected() {
    reconnect_pending_ = false;
    if (main_window_) {
        main_window_->hideConnectionBanner();
        main_window_->appendEventLog(QStringLiteral("已连接至服务器"), QStringLiteral("✓"));
    }
}

void App::handleSocketDisconnected() {
    if (!main_window_ || reconnect_suppressed_) {
        return;
    }

    main_window_->showConnectionBanner(QStringLiteral("已断开连接，正在重连…"));
    main_window_->appendEventLog(QStringLiteral("连接已断开，3 秒后重连"), QStringLiteral("●"));

    if (reconnect_pending_ || !login_window_ || !login_window_->hasServerConfig()) {
        return;
    }

    reconnect_pending_ = true;
    QTimer::singleShot(3000, this, [this] {
        reconnect_pending_ = false;
        if (reconnect_suppressed_ || tcp_client_.isConnected() || !login_window_) {
            return;
        }
        login_window_->reconnectToConfiguredServer();
    });
}

void App::handleSocketError(const QString& message) {
    if (!main_window_ || reconnect_suppressed_) {
        return;
    }

    main_window_->showConnectionBanner(QStringLiteral("网络异常：%1").arg(message));
    main_window_->appendEventLog(QStringLiteral("网络错误：%1").arg(message), QStringLiteral("●"));
}
