// =============================================================
// client/src/ui/login_window.cpp
// 登录窗口实现（.ui 文件版本）
//
// AUTOUIC 会根据 login_window.ui 生成 ui_login_window.h，
// 其中包含 Ui::LoginWindow 结构体和 setupUi() 方法。
// setupUi(this) 一行代替了之前几百行的手写布局代码。
// =============================================================

#include "login_window.h"
#include "main_window.h"
#include "ui_login_window.h"  // AUTOUIC 自动生成，路径在 cmake-build-debug/ 下

#include "common/protocol.h"
#include "common/protocol_codec.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QTimer>

// 日志分类：运行时可通过 QT_LOGGING_RULES="ui.login=true" 环境变量启用
Q_LOGGING_CATEGORY(lcLogin, "ui.login")

// ── 主题色彩常量（单一可信来源）───────────────────────────────────────────
// QSS 模板中使用 @token 占位符，setupStyle() 会将其替换为对应颜色值。
// 修改主题只需在此处调整，QSS 无需改动。
// ─────────────────────────────────────────────────────────────────────────
namespace {
const QPair<const char*, const char*> kTheme[] = {
    {"@bgFieldFocus",   "#FFFFFF"},  // 输入框聚焦背景
    {"@bgPage",         "#F4F6F8"},  // 页面背景
    {"@bgField",        "#F0F2F5"},  // 输入框背景
    {"@bgToggleHover",  "#EAEDF0"},  // 密码切换按钮悬停背景
    {"@bgToggle",       "#EFF6FF"},  // 密码切换按钮选中背景
    {"@border",         "#E2E6EA"},  // 默认边框
    {"@txtDark",        "#374151"},  // 标签文字
    {"@txtPrimary",     "#111827"},  // 主文字
    {"@txtSecondary",   "#6B7280"},  // 次要文字
    {"@txtMuted",       "#9CA3AF"},  // 占位符文字
    {"@accentDisabled", "#93C5FD"},  // 品牌蓝禁用
    {"@accentPressed",  "#1D4ED8"},  // 品牌蓝按下
    {"@accentHover",    "#2563EB"},  // 品牌蓝悬停
    {"@accent",         "#3B82F6"},  // 品牌蓝（按钮/高亮）
    {"@error",          "#EF4444"},  // 错误红
    {"@statusBg",       "#F0FDF4"},  // 状态栏背景
    {"@statusBorder",   "#BBF7D0"},  // 状态栏边框
    {"@statusDot",      "#16A34A"},  // 在线状态点
    {"@statusText",     "#15803D"},  // 状态栏文字
};

QString formatServerEndpoint(const QString& host, quint16 port) {
    return QString("%1:%2").arg(host).arg(port);
}
} // namespace

// =============================================================
// 构造函数
// =============================================================
LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent)
    , ui_(std::make_unique<Ui::LoginWindow>())
    , auth_service_(tcp_client_, router_, this)
    , chat_service_(tcp_client_, router_, this)
    , friend_service_(tcp_client_, router_, this)
    , file_service_(tcp_client_, router_, this)
    , share_service_(tcp_client_, router_, this)
{
    ui_->setupUi(this);

    setMinimumSize(400, 560);
    resize(400, 560);
    setWindowIcon(QApplication::windowIcon());

    setupStyle();
    connectSignals();
    registerHandlers();

    server_config_loaded_ = loadServerConfig();
    if (server_config_loaded_) {
        ui_->serverAddrLabel->setText(serverEndpoint() + " · 已就绪");
        setupNetwork();
    } else {
        applyConfigError("未找到有效配置");
    }
}

// =============================================================
// 析构函数：= default 即可，unique_ptr 自动释放 ui_
// 必须在 .cpp 中定义（而非头文件 inline），因为此处才能看到
// Ui::LoginWindow 的完整类型（通过 #include "ui_login_window.h"）
// =============================================================
LoginWindow::~LoginWindow() = default;

// =============================================================
// setupStyle()：QSS 样式表
// 颜色通过 @token 占位符引用，setupStyle() 末尾统一替换为 kTheme 中的值
// =============================================================
void LoginWindow::setupStyle() {
    QString qss = QStringLiteral(R"(
        LoginWindow {
            background-color: @bgPage;
        }

        QWidget#card {
            background-color: @bgFieldFocus;
            border-radius: 16px;
            border: 1px solid @border;
        }

        QLabel#appIcon {
            font-size: 36px;
        }

        QLabel#appNameLabel {
            font-size: 20px;
            font-weight: bold;
            color: @txtPrimary;
        }

        QLabel#appSubtitleLabel {
            font-size: 13px;
            color: @txtSecondary;
        }

        QTabWidget#tabWidget::pane {
            border: none;
            background: transparent;
        }

        QTabWidget#tabWidget QTabBar::tab {
            padding: 8px 28px;
            color: @txtSecondary;
            background: transparent;
            border: none;
            border-bottom: 2px solid transparent;
            font-size: 14px;
        }

        QTabWidget#tabWidget QTabBar::tab:selected {
            color: @accent;
            border-bottom: 2px solid @accent;
            font-weight: bold;
        }

        QTabWidget#tabWidget QTabBar::tab:hover:!selected {
            color: @txtDark;
        }

        QLabel#loginUserLabel, QLabel#loginPwdLabel,
        QLabel#regPwdLabel, QLabel#regConfirmLabel {
            font-size: 13px;
            font-weight: 500;
            color: @txtDark;
        }

        QLineEdit {
            height: 44px;
            border: 1.5px solid @border;
            border-radius: 8px;
            background: @bgField;
            padding: 0 12px;
            font-size: 14px;
            color: @txtPrimary;
            selection-background-color: @accent;
        }

        QLineEdit:focus {
            border-color: @accent;
            background: @bgFieldFocus;
        }

        QLineEdit::placeholder {
            color: @txtMuted;
        }

        QLabel#loginStatusLabel, QLabel#regStatusLabel {
            font-size: 12px;
            color: @error;
            padding: 2px 0;
        }

        QPushButton#loginBtn, QPushButton#regBtn {
            height: 44px;
            border-radius: 8px;
            background: @accent;
            color: white;
            font-size: 15px;
            font-weight: bold;
            border: none;
        }

        QPushButton#loginBtn:hover, QPushButton#regBtn:hover {
            background: @accentHover;
        }

        QPushButton#loginBtn:pressed, QPushButton#regBtn:pressed {
            background: @accentPressed;
        }

        QPushButton#loginBtn:disabled, QPushButton#regBtn:disabled {
            background: @accentDisabled;
        }

        QPushButton#loginPwdToggleBtn,
        QPushButton#regPwdToggleBtn,
        QPushButton#regConfirmToggleBtn {
            height: 44px;
            min-width: 44px;
            max-width: 44px;
            border: 1.5px solid @border;
            border-radius: 8px;
            background: @bgField;
            color: @txtSecondary;
            font-size: 12px;
            padding: 0;
        }

        QPushButton#loginPwdToggleBtn:hover,
        QPushButton#regPwdToggleBtn:hover,
        QPushButton#regConfirmToggleBtn:hover {
            color: @txtPrimary;
            background: @bgToggleHover;
        }

        QPushButton#loginPwdToggleBtn:checked,
        QPushButton#regPwdToggleBtn:checked,
        QPushButton#regConfirmToggleBtn:checked {
            color: @accent;
            border-color: @accent;
            background: @bgToggle;
        }

        QWidget#statusFrame {
            background: @statusBg;
            border: 1px solid @statusBorder;
            border-radius: 8px;
        }

        QLabel#serverDotLabel {
            color: @statusDot;
            font-size: 10px;
        }

        QLabel#serverAddrLabel {
            font-family: "Noto Sans Mono", "Consolas", monospace;
            font-size: 12px;
            color: @statusText;
        }
    )");

    // 将 @token 替换为 kTheme 中定义的颜色值
    for (const auto& [token, value] : kTheme) {
        qss.replace(QLatin1String(token), QLatin1String(value));
    }
    setStyleSheet(qss);
}

// =============================================================
// resetLoginBtn() / resetRegBtn()：恢复按钮到可点击状态
// 第六章接入 AuthService 后，这两个方法将在服务回调（onLoginFailed / onRegisterFailed）
// 中调用，而不是在此处直接调用。
// =============================================================
void LoginWindow::resetLoginBtn() {
    ui_->loginBtn->setEnabled(true);
    ui_->loginBtn->setText("登录");
}

void LoginWindow::resetRegBtn() {
    ui_->regBtn->setEnabled(true);
    ui_->regBtn->setText("创建账号");
}

// =============================================================
// connectSignals()：绑定按钮点击信号到槽函数
// 通过 ui_->控件名 访问 .ui 文件中定义的控件
// =============================================================
void LoginWindow::connectSignals() {
    connect(ui_->loginBtn, &QPushButton::clicked,
            this, &LoginWindow::onLoginClicked);

    connect(ui_->regBtn, &QPushButton::clicked,
            this, &LoginWindow::onRegisterClicked);

    connect(ui_->loginPwdToggleBtn, &QPushButton::toggled,
            this, &LoginWindow::toggleLoginPwdVisibility);

    connect(ui_->regPwdToggleBtn, &QPushButton::toggled,
            this, &LoginWindow::toggleRegPwdVisibility);

    connect(ui_->regConfirmToggleBtn, &QPushButton::toggled,
            this, &LoginWindow::toggleRegConfirmVisibility);

    // AuthService 信号 → UI 槽
    connect(&auth_service_, &cloudvault::AuthService::registerSuccess,
            this, [this] {
                ui_->regStatusLabel->setText("注册成功，请登录");
                ui_->regStatusLabel->setStyleSheet("color: #16A34A;");
                ui_->regStatusLabel->show();
                resetRegBtn();
            });

    connect(&auth_service_, &cloudvault::AuthService::registerFailed,
            this, [this](const QString& reason) {
                ui_->regStatusLabel->setText(reason);
                ui_->regStatusLabel->setStyleSheet("");
                ui_->regStatusLabel->show();
                resetRegBtn();
            });

    connect(&auth_service_, &cloudvault::AuthService::loginSuccess,
            this, [this](int userId) {
                Q_UNUSED(userId);
                ui_->loginStatusLabel->setText("登录成功");
                ui_->loginStatusLabel->setStyleSheet("color: #16A34A;");
                ui_->loginStatusLabel->show();
                resetLoginBtn();

                main_window_.reset();
                main_window_ = std::make_unique<MainWindow>(
                    current_username_, chat_service_, friend_service_, file_service_, share_service_);
                connect(main_window_.get(), &MainWindow::windowClosed,
                        this, [this] {
                            auth_service_.logout();
                            tcp_client_.disconnectFromServer();
                            QCoreApplication::quit();
                        });
                connect(main_window_.get(), &MainWindow::logoutRequested,
                        this, [this] {
                            if (main_window_) {
                                main_window_->hide();
                            }

                            auth_service_.logout();
                            tcp_client_.disconnectFromServer();
                            ui_->loginPasswordEdit->clear();
                            ui_->loginStatusLabel->setText("已退出登录");
                            ui_->loginStatusLabel->setStyleSheet("color: #16A34A;");
                            ui_->loginStatusLabel->show();
                            show();
                            raise();
                            activateWindow();

                            QTimer::singleShot(120, this, [this] {
                                if (server_config_loaded_) {
                                    tcp_client_.connectToServer(server_host_, server_port_);
                                }
                            });
                        });
                hide();
                main_window_->show();
                main_window_->raise();
                main_window_->activateWindow();
                friend_service_.flushFriends();
            });

    connect(&auth_service_, &cloudvault::AuthService::loginFailed,
            this, [this](const QString& reason) {
                ui_->loginStatusLabel->setText(reason);
                ui_->loginStatusLabel->setStyleSheet("");
                ui_->loginStatusLabel->show();
                resetLoginBtn();
            });
}

// =============================================================
// onLoginClicked()：登录按钮响应
// =============================================================
void LoginWindow::onLoginClicked() {
    const QString username = ui_->loginUsernameEdit->text().trimmed();
    const QString password = ui_->loginPasswordEdit->text();

    // 密码不做 trimmed()，保留前后空格（可能是有意设置的密码内容）
    // 但用 trimmed().isEmpty() 检测仅含空格的无效输入
    if (username.isEmpty() || password.trimmed().isEmpty()) {
        ui_->loginStatusLabel->setText("用户名和密码不能为空");
        ui_->loginStatusLabel->show();
        return;
    }

    ui_->loginStatusLabel->hide();
    ui_->loginBtn->setEnabled(false);
    ui_->loginBtn->setText("登录中…");
    current_username_ = username;

    qCDebug(lcLogin) << "准备登录：用户名 =" << username;
    auth_service_.login(username, password);
}

// =============================================================
// onRegisterClicked()：注册按钮响应
// =============================================================
void LoginWindow::onRegisterClicked() {
    const QString username     = ui_->regUsernameEdit->text().trimmed();
    const QString password     = ui_->regPasswordEdit->text();
    const QString confirm      = ui_->regConfirmEdit->text();

    auto showError = [this](const QString& msg) {
        ui_->regStatusLabel->setText(msg);
        ui_->regStatusLabel->show();
    };

    // 密码本身不 trim，但用 trimmed().isEmpty() 检测仅含空格的无效输入
    if (username.isEmpty() ||
        password.trimmed().isEmpty() || confirm.trimmed().isEmpty()) {
        showError("用户名和密码均为必填项");
        return;
    }

    if (username.length() < 3 || username.length() > 20) {
        showError("用户名长度须在 3–20 个字符之间");
        return;
    }

    if (password.length() < 8) {
        showError("密码至少需要 8 位字符");
        return;
    }

    if (password != confirm) {
        showError("两次输入的密码不一致");
        return;
    }

    ui_->regStatusLabel->hide();
    ui_->regBtn->setEnabled(false);
    ui_->regBtn->setText("注册中…");

    qCDebug(lcLogin) << "准备注册：用户名 =" << username;
    auth_service_.registerUser(username, password);
}

// =============================================================
// toggleLoginPwdVisibility()：切换登录密码明/密文显示
// =============================================================
void LoginWindow::toggleLoginPwdVisibility() {
    const bool show = ui_->loginPwdToggleBtn->isChecked();
    ui_->loginPasswordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    ui_->loginPwdToggleBtn->setText(show ? "隐藏" : "显示");
}

// =============================================================
// toggleRegPwdVisibility()：切换注册密码明/密文显示
// =============================================================
void LoginWindow::toggleRegPwdVisibility() {
    const bool show = ui_->regPwdToggleBtn->isChecked();
    ui_->regPasswordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    ui_->regPwdToggleBtn->setText(show ? "隐藏" : "显示");
}

// =============================================================
// toggleRegConfirmVisibility()：切换确认密码明/密文显示
// =============================================================
void LoginWindow::toggleRegConfirmVisibility() {
    const bool show = ui_->regConfirmToggleBtn->isChecked();
    ui_->regConfirmEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    ui_->regConfirmToggleBtn->setText(show ? "隐藏" : "显示");
}

bool LoginWindow::loadServerConfig() {
    const QDir current_dir(QDir::currentPath());
    const QDir app_dir(QCoreApplication::applicationDirPath());
    const QStringList candidates = {
        current_dir.filePath("client.json"),
        current_dir.filePath("config/client.json"),
        current_dir.filePath("client/config/client.json"),
        current_dir.filePath("../config/client.json"),
        current_dir.filePath("../client/config/client.json"),
        current_dir.filePath("../../client/config/client.json"),
        app_dir.filePath("client.json"),
        app_dir.filePath("config/client.json"),
        app_dir.filePath("../config/client.json"),
        app_dir.filePath("../client/config/client.json"),
        app_dir.filePath("../../client/config/client.json"),
    };

    for (const QString& path : candidates) {
        const QString normalized_path = QDir::cleanPath(path);
        QFile file(normalized_path);
        if (!file.exists()) {
            continue;
        }
        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(lcLogin) << "Failed to open client config:"
                               << normalized_path << file.errorString();
            continue;
        }

        QJsonParseError parse_error;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse_error);
        if (parse_error.error != QJsonParseError::NoError || !doc.isObject()) {
            qCWarning(lcLogin) << "Failed to parse client config:" << normalized_path
                               << parse_error.errorString();
            continue;
        }

        const QJsonObject server = doc.object().value("server").toObject();
        const QString host = server.value("host").toString().trimmed();
        const int port = server.value("port").toInt(0);

        if (host.isEmpty() || port <= 0 || port > 65535) {
            qCWarning(lcLogin) << "Invalid server config in" << normalized_path
                               << "host=" << host << "port=" << port;
            continue;
        }

        server_host_ = host;
        server_port_ = static_cast<quint16>(port);
        qCInfo(lcLogin) << "Loaded client config from"
                        << QFileInfo(normalized_path).absoluteFilePath()
                        << "server=" << serverEndpoint();
        return true;
    }

    qCWarning(lcLogin) << "Client config is required but no valid config file was found";
    return false;
}

// =============================================================
// setupNetwork()：初始化网络层，连接到服务器
// =============================================================
void LoginWindow::setupNetwork() {
    // 绑定网络事件到 UI 槽
    connect(&tcp_client_, &cloudvault::TcpClient::connected,
            this, &LoginWindow::onServerConnected);
    connect(&tcp_client_, &cloudvault::TcpClient::disconnected,
            this, &LoginWindow::onServerDisconnected);
    connect(&tcp_client_, &cloudvault::TcpClient::errorOccurred,
            this, &LoginWindow::onServerError);

    // 收到 PDU 时交给 ResponseRouter 分发
    connect(&tcp_client_, &cloudvault::TcpClient::pduReceived,
            this, [this](cloudvault::PDUHeader hdr, std::vector<uint8_t> body) {
                router_.dispatch(hdr, body);
            });

    tcp_client_.connectToServer(server_host_, server_port_);
}

// =============================================================
// registerHandlers()：注册 PDU 响应处理器
// =============================================================
void LoginWindow::registerHandlers() {
    // PONG：服务器对 PING 的回应，表示连接正常
    router_.registerHandler(
        cloudvault::MessageType::PONG,
        [this](const cloudvault::PDUHeader& /*hdr*/,
               const std::vector<uint8_t>& /*body*/) {
            qCDebug(lcLogin) << "PONG received — server alive";
            ui_->serverAddrLabel->setText(serverEndpoint() + " · 已连接");
            ui_->serverDotLabel->setStyleSheet("color: #16A34A;");  // 绿色
        });

    // TODO（第八章）：LOGIN_RESPONSE、REGISTER_RESPONSE
}

// =============================================================
// 网络事件槽
// =============================================================
void LoginWindow::onServerConnected() {
    qCDebug(lcLogin) << "Connected to server, sending PING";
    ui_->serverAddrLabel->setText(serverEndpoint() + " · 连接中…");

    // 发送 PING 探测服务器延迟
    auto ping = cloudvault::PDUBuilder(cloudvault::MessageType::PING).build();
    tcp_client_.send(std::move(ping));
}

void LoginWindow::onServerDisconnected() {
    qCDebug(lcLogin) << "Disconnected from server";
    ui_->serverAddrLabel->setText(serverEndpoint() + " · 未连接");
    ui_->serverDotLabel->setStyleSheet("color: #EF4444;");  // 红色
}

void LoginWindow::onServerError(const QString& message) {
    qCWarning(lcLogin) << "Server error:" << message;
    ui_->serverAddrLabel->setText(serverEndpoint() + " · 连接失败");
    ui_->serverDotLabel->setStyleSheet("color: #EF4444;");  // 红色
}

void LoginWindow::applyConfigError(const QString& message) {
    ui_->serverAddrLabel->setText("配置错误");
    ui_->serverDotLabel->setStyleSheet("color: #EF4444;");
    ui_->loginStatusLabel->setText("客户端配置无效，请检查 client/config/client.json");
    ui_->loginStatusLabel->show();
    ui_->regStatusLabel->setText("客户端配置无效，请检查 client/config/client.json");
    ui_->regStatusLabel->show();
    ui_->loginBtn->setEnabled(false);
    ui_->regBtn->setEnabled(false);
    qCWarning(lcLogin) << "Client config error:" << message;
}

QString LoginWindow::serverEndpoint() const {
    return formatServerEndpoint(server_host_, server_port_);
}
