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
#include "service/group_service.h"
#include "service/share_service.h"

#include <QObject>
#include <QString>
#include <memory>

class LoginWindow;
class MainWindow;
class QWidget;

/**
 * @brief 客户端应用装配根对象。
 *
 * 负责创建并持有网络层、服务层和顶层窗口对象，
 * 并协调登录窗口与主窗口之间的切换、断线重连和退出流程。
 */
class App : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造应用装配对象并初始化服务依赖。
     * @param parent Qt 父对象。
     */
    explicit App(QObject* parent = nullptr);

    /**
     * @brief 析构应用装配对象。
     */
    ~App() override;

    /**
     * @brief 获取应用启动时应展示的首个窗口。
     * @return 首窗口指针（当前为登录窗口）。
     */
    QWidget* initialWindow() const;

private:
    /**
     * @brief 连接全局信号槽。
     */
    void connectSignals();

    /**
     * @brief 创建并展示主窗口。
     * @param username 当前登录用户名。
     */
    void showMainWindow(const QString& username);

    /**
     * @brief 处理主窗口关闭事件。
     */
    void handleMainWindowClosed();

    /**
     * @brief 处理用户主动退出登录。
     */
    void handleLogoutRequested();

    /**
     * @brief 处理 TCP 连接成功事件。
     */
    void handleSocketConnected();

    /**
     * @brief 处理 TCP 连接断开事件，并按策略触发重连。
     */
    void handleSocketDisconnected();

    /**
     * @brief 处理 TCP 错误事件。
     * @param message 错误描述。
     */
    void handleSocketError(const QString& message);

    /// @brief 低层 TCP 客户端。
    cloudvault::TcpClient tcp_client_;
    /// @brief PDU 路由器，将消息分发给各服务。
    cloudvault::ResponseRouter router_;
    /// @brief 认证服务。
    cloudvault::AuthService auth_service_;
    /// @brief 群组服务。
    cloudvault::GroupService group_service_;
    /// @brief 聊天服务。
    cloudvault::ChatService chat_service_;
    /// @brief 好友服务。
    cloudvault::FriendService friend_service_;
    /// @brief 文件服务。
    cloudvault::FileService file_service_;
    /// @brief 分享服务。
    cloudvault::ShareService share_service_;

    /// @brief 登录窗口实例。
    std::unique_ptr<LoginWindow> login_window_;
    /// @brief 主窗口实例（登录后创建）。
    std::unique_ptr<MainWindow> main_window_;
    /// @brief 是否禁止自动重连（退出流程中置为 true）。
    bool reconnect_suppressed_ = false;
    /// @brief 是否已有待执行重连任务，防止重复排队。
    bool reconnect_pending_ = false;
};
