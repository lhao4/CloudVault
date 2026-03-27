// =============================================================
// client/src/ui/login_window.h
// 登录窗口类声明（.ui 文件版本）
//
// 与纯代码版本的区别：
//   - 不再手动声明每个控件成员变量
//   - 改用 unique_ptr<Ui::LoginWindow>，由 AUTOUIC 生成的 ui_login_window.h 提供
//   - 控件通过 ui_->widgetName 访问，名字来自 .ui 文件中的 objectName
// =============================================================

#pragma once

#include "network/tcp_client.h"
#include "network/response_router.h"
#include "service/auth_service.h"

#include <QString>
#include <QWidget>
#include <memory>

// 前向声明 Ui::LoginWindow（完整定义在 AUTOUIC 生成的 ui_login_window.h 中）
// 析构函数在 .cpp 中定义为 = default，确保 unique_ptr 在完整类型可见处析构
QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE

/**
 * @brief 登录/注册入口窗口。
 *
 * 负责服务器配置加载、连接建立、登录注册交互和基础状态提示。
 */
class LoginWindow : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造登录窗口。
     * @param tcp_client TCP 客户端引用。
     * @param router 响应路由器引用。
     * @param auth_service 认证服务引用。
     * @param parent Qt 父对象。
     */
    explicit LoginWindow(cloudvault::TcpClient& tcp_client,
                         cloudvault::ResponseRouter& router,
                         cloudvault::AuthService& auth_service,
                         QWidget* parent = nullptr);
    /**
     * @brief 析构登录窗口。
     *
     * 在 .cpp 中定义为 = default，确保 unique_ptr 析构时完整类型可见。
     */
    ~LoginWindow() override;

    /**
     * @brief 处理登出后的 UI 复位。
     */
    void handleLoggedOut();
    /**
     * @brief 使用当前配置重新连接服务器。
     * @param delay_ms 延迟毫秒数，0 表示立即连接。
     */
    void reconnectToConfiguredServer(int delay_ms = 0);
    /**
     * @brief 当前是否已加载有效服务器配置。
     * @return true 表示配置可用于连接。
     */
    bool hasServerConfig() const;

private slots:
    /// @brief 点击登录按钮处理。
    void onLoginClicked();
    /// @brief 点击注册按钮处理。
    void onRegisterClicked();
    /// @brief 登录密码显示/隐藏切换。
    void toggleLoginPwdVisibility();
    /// @brief 注册密码显示/隐藏切换。
    void toggleRegPwdVisibility();
    /// @brief 注册确认密码显示/隐藏切换。
    void toggleRegConfirmVisibility();

    /// @brief 网络已连接回调。
    void onServerConnected();
    /// @brief 网络断开回调。
    void onServerDisconnected();
    /// @brief 网络错误回调。
    void onServerError(const QString& message);

signals:
    /// @brief 登录成功后发出，携带用户名。
    void loginSucceeded(const QString& username);

private:
    /**
     * @brief 应用窗口级样式。
     */
    void setupStyle();
    /**
     * @brief 连接窗口内部信号槽。
     */
    void connectSignals();
    /**
     * @brief 恢复登录按钮可用状态。
     */
    void resetLoginBtn();
    /**
     * @brief 恢复注册按钮可用状态。
     */
    void resetRegBtn();
    /**
     * @brief 加载服务器配置。
     * @return true 表示加载成功。
     */
    bool loadServerConfig();
    /**
     * @brief 初始化网络连接流程。
     */
    void setupNetwork();
    /**
     * @brief 注册登录页使用的协议处理器。
     */
    void registerHandlers();
    /**
     * @brief 应用配置错误状态到 UI。
     * @param message 错误文案。
     */
    void applyConfigError(const QString& message);
    /**
     * @brief 获取当前服务端地址字符串。
     * @return host:port 形式字符串。
     */
    QString serverEndpoint() const;

    /// @brief UI 对象，管理 .ui 生成控件。
    std::unique_ptr<Ui::LoginWindow> ui_;

    /// @brief 服务器主机地址。
    QString server_host_;
    /// @brief 服务器端口。
    quint16 server_port_ = 0;
    /// @brief 是否已成功加载服务器配置。
    bool server_config_loaded_ = false;
    /// @brief 当前登录用户名缓存。
    QString current_username_;

    /// @brief TCP 客户端引用。
    cloudvault::TcpClient& tcp_client_;
    /// @brief 响应路由器引用。
    cloudvault::ResponseRouter& router_;
    /// @brief 认证服务引用。
    cloudvault::AuthService& auth_service_;
};
