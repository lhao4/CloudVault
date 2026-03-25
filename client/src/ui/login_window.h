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
#include "network/auth_service.h"

#include <QString>
#include <QWidget>
#include <memory>

// 前向声明 Ui::LoginWindow（完整定义在 AUTOUIC 生成的 ui_login_window.h 中）
// 析构函数在 .cpp 中定义为 = default，确保 unique_ptr 在完整类型可见处析构
QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override;  // 在 .cpp 中定义为 = default（unique_ptr 需要完整类型）

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void toggleLoginPwdVisibility();    // 登录密码显示/隐藏
    void toggleRegPwdVisibility();      // 注册密码显示/隐藏
    void toggleRegConfirmVisibility();  // 确认密码显示/隐藏

    // 网络事件槽
    void onServerConnected();
    void onServerDisconnected();
    void onServerError(const QString& message);

private:
    void setupStyle();        // 应用 QSS 样式表
    void connectSignals();    // 绑定信号槽
    void resetLoginBtn();     // 恢复登录按钮为可点击状态
    void resetRegBtn();       // 恢复注册按钮为可点击状态
    bool loadServerConfig();  // 从配置文件读取服务器地址
    void setupNetwork();      // 初始化网络层并连接服务器
    void registerHandlers();  // 注册 PDU 响应处理器
    void applyConfigError(const QString& message);
    QString serverEndpoint() const;

    std::unique_ptr<Ui::LoginWindow> ui_;  // RAII 管理，析构时自动释放

    QString server_host_;
    quint16 server_port_ = 0;
    bool server_config_loaded_ = false;

    // 网络层（第七章）
    cloudvault::TcpClient      tcp_client_;
    cloudvault::ResponseRouter  router_;

    // 认证服务（第八章）
    cloudvault::AuthService    auth_service_;
};
