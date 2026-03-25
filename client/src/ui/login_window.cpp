// =============================================================
// client/src/ui/login_window.cpp
// 登录窗口实现（.ui 文件版本）
//
// AUTOUIC 会根据 login_window.ui 生成 ui_login_window.h，
// 其中包含 Ui::LoginWindow 结构体和 setupUi() 方法。
// setupUi(this) 一行代替了之前几百行的手写布局代码。
// =============================================================

#include "login_window.h"
#include "ui_login_window.h"  // AUTOUIC 自动生成，路径在 cmake-build-debug/ 下

#include "common/protocol.h"
#include "common/protocol_codec.h"

#include <QLoggingCategory>
#include <QPixmap>

// 日志分类：运行时可通过 QT_LOGGING_RULES="ui.login=true" 环境变量启用
Q_LOGGING_CATEGORY(lcLogin, "ui.login")

// ── 主题色彩常量（单一可信来源）───────────────────────────────────────────
// QSS 模板中使用 @token 占位符，setupStyle() 会将其替换为对应颜色值。
// 修改主题只需在此处调整，QSS 无需改动。
// ─────────────────────────────────────────────────────────────────────────
namespace {
const QPair<const char*, const char*> kTheme[] = {
    {"@bgPage",         "#F4F6F8"},  // 页面背景
    {"@bgField",        "#F0F2F5"},  // 输入框背景
    {"@bgFieldFocus",   "#FFFFFF"},  // 输入框聚焦背景
    {"@bgToggle",       "#EFF6FF"},  // 密码切换按钮选中背景
    {"@bgToggleHover",  "#EAEDF0"},  // 密码切换按钮悬停背景
    {"@border",         "#E2E6EA"},  // 默认边框
    {"@txtPrimary",     "#111827"},  // 主文字
    {"@txtSecondary",   "#6B7280"},  // 次要文字
    {"@txtMuted",       "#9CA3AF"},  // 占位符文字
    {"@txtDark",        "#374151"},  // 标签文字
    {"@accent",         "#3B82F6"},  // 品牌蓝（按钮/高亮）
    {"@accentHover",    "#2563EB"},  // 品牌蓝悬停
    {"@accentPressed",  "#1D4ED8"},  // 品牌蓝按下
    {"@accentDisabled", "#93C5FD"},  // 品牌蓝禁用
    {"@error",          "#EF4444"},  // 错误红
    {"@statusBg",       "#F0FDF4"},  // 状态栏背景
    {"@statusBorder",   "#BBF7D0"},  // 状态栏边框
    {"@statusDot",      "#16A34A"},  // 在线状态点
    {"@statusText",     "#15803D"},  // 状态栏文字
};
} // namespace

// =============================================================
// 构造函数
// =============================================================
LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent)
    , ui_(std::make_unique<Ui::LoginWindow>())
{
    ui_->setupUi(this);

    // Qt 6 内置 High DPI 支持，逻辑像素自动按 devicePixelRatio 缩放。
    // 使用 setMinimumSize 而非 setFixedSize，允许布局在不同 DPI/字体配置下自适应。
    setMinimumSize(400, 560);
    resize(400, 560);

    // 设置 header 图标（40×40 逻辑像素，scaledContents 已在 .ui 中开启）
    ui_->iconLabel->setPixmap(QPixmap(":/icons/app_icon.png"));

    setupStyle();
    connectSignals();
    registerHandlers();
    setupNetwork();
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

    qCDebug(lcLogin) << "准备登录：用户名 =" << username;
    // TODO（第六章）：AuthService::login(username, password)
    // TODO（第六章）：下面的 resetLoginBtn() 调用移至 onLoginFailed() 回调中
    resetLoginBtn();
}

// =============================================================
// onRegisterClicked()：注册按钮响应
// =============================================================
void LoginWindow::onRegisterClicked() {
    const QString username     = ui_->regUsernameEdit->text().trimmed();
    const QString display_name = ui_->regDisplayNameEdit->text().trimmed();
    const QString password     = ui_->regPasswordEdit->text();
    const QString confirm      = ui_->regConfirmEdit->text();

    auto showError = [this](const QString& msg) {
        ui_->regStatusLabel->setText(msg);
        ui_->regStatusLabel->show();
    };

    // 密码本身不 trim，但用 trimmed().isEmpty() 检测仅含空格的无效输入
    if (username.isEmpty() || display_name.isEmpty() ||
        password.trimmed().isEmpty() || confirm.trimmed().isEmpty()) {
        showError("所有字段均为必填项");
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

    qCDebug(lcLogin) << "准备注册：用户名 =" << username << "昵称 =" << display_name;
    // TODO（第六章）：AuthService::registerUser(username, display_name, password)
    // TODO（第六章）：下面的 resetRegBtn() 调用移至 onRegisterFailed() 回调中
    resetRegBtn();
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

    // TODO（第八章）：从配置文件读取服务器地址
    tcp_client_.connectToServer("127.0.0.1", 5000);
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
            ui_->serverAddrLabel->setText("127.0.0.1:5000");
            ui_->serverDotLabel->setStyleSheet("color: #16A34A;");  // 绿色
        });

    // TODO（第八章）：LOGIN_RESPONSE、REGISTER_RESPONSE
}

// =============================================================
// 网络事件槽
// =============================================================
void LoginWindow::onServerConnected() {
    qCDebug(lcLogin) << "Connected to server, sending PING";
    ui_->serverAddrLabel->setText("连接中…");

    // 发送 PING 探测服务器延迟
    auto ping = cloudvault::PDUBuilder(cloudvault::MessageType::PING).build();
    tcp_client_.send(std::move(ping));
}

void LoginWindow::onServerDisconnected() {
    qCDebug(lcLogin) << "Disconnected from server";
    ui_->serverAddrLabel->setText("未连接");
    ui_->serverDotLabel->setStyleSheet("color: #EF4444;");  // 红色
}

void LoginWindow::onServerError(const QString& message) {
    qCWarning(lcLogin) << "Server error:" << message;
    ui_->serverAddrLabel->setText("连接失败");
    ui_->serverDotLabel->setStyleSheet("color: #EF4444;");  // 红色
}
