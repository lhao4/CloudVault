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

#include <QDebug>

// =============================================================
// 构造函数
// =============================================================
LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent)
    , ui_(new Ui::LoginWindow)  // 创建 UI 结构体
{
    // setupUi(this)：读取 .ui 文件描述的布局，创建所有控件并挂载到 this
    // 这一行等价于之前 setupUi() 里几百行的 new QLabel / new QLineEdit ...
    ui_->setupUi(this);

    // 固定窗口大小，禁止拖拽缩放
    setFixedSize(400, 560);

    setupStyle();
    connectSignals();
}

// =============================================================
// 析构函数：释放 ui_ 指针
// （ui_ 管理的控件由 Qt parent-child 机制释放，ui_ 本身需要手动 delete）
// =============================================================
LoginWindow::~LoginWindow() {
    delete ui_;
}

// =============================================================
// setupStyle()：QSS 样式表（与之前完全相同）
// =============================================================
void LoginWindow::setupStyle() {
    setStyleSheet(R"(
        LoginWindow {
            background-color: #F4F6F8;
        }

        QWidget#card {
            background-color: #FFFFFF;
            border-radius: 16px;
            border: 1px solid #E2E6EA;
        }

        QLabel#appIcon {
            font-size: 36px;
        }

        QLabel#appNameLabel {
            font-size: 20px;
            font-weight: bold;
            color: #111827;
        }

        QLabel#appSubtitleLabel {
            font-size: 13px;
            color: #6B7280;
        }

        QTabWidget#tabWidget::pane {
            border: none;
            background: transparent;
        }

        QTabWidget#tabWidget QTabBar::tab {
            padding: 8px 28px;
            color: #6B7280;
            background: transparent;
            border: none;
            border-bottom: 2px solid transparent;
            font-size: 14px;
        }

        QTabWidget#tabWidget QTabBar::tab:selected {
            color: #3B82F6;
            border-bottom: 2px solid #3B82F6;
            font-weight: bold;
        }

        QTabWidget#tabWidget QTabBar::tab:hover:!selected {
            color: #374151;
        }

        QLabel#loginUserLabel, QLabel#loginPwdLabel,
        QLabel#regPwdLabel, QLabel#regConfirmLabel {
            font-size: 13px;
            font-weight: 500;
            color: #374151;
        }

        QLineEdit {
            height: 44px;
            border: 1.5px solid #E2E6EA;
            border-radius: 8px;
            background: #F0F2F5;
            padding: 0 12px;
            font-size: 14px;
            color: #111827;
        }

        QLineEdit:focus {
            border-color: #3B82F6;
            background: #FFFFFF;
        }

        QLabel#loginStatusLabel, QLabel#regStatusLabel {
            font-size: 12px;
            color: #EF4444;
            padding: 2px 0;
        }

        QPushButton#loginBtn, QPushButton#regBtn {
            height: 44px;
            border-radius: 8px;
            background: #3B82F6;
            color: white;
            font-size: 15px;
            font-weight: bold;
            border: none;
        }

        QPushButton#loginBtn:hover, QPushButton#regBtn:hover {
            background: #2563EB;
        }

        QPushButton#loginBtn:pressed, QPushButton#regBtn:pressed {
            background: #1D4ED8;
        }

        QPushButton#loginBtn:disabled, QPushButton#regBtn:disabled {
            background: #93C5FD;
        }

        QWidget#statusFrame {
            background: #F0FDF4;
            border: 1px solid #BBF7D0;
            border-radius: 8px;
        }

        QLabel#serverDotLabel {
            color: #16A34A;
            font-size: 10px;
        }

        QLabel#serverAddrLabel {
            font-family: "Noto Sans Mono", "Consolas", monospace;
            font-size: 12px;
            color: #15803D;
        }

        QLineEdit {
            selection-background-color: #3B82F6;
        }

        QLineEdit::placeholder {
            color: #9CA3AF;
        }
    )");
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
}

// =============================================================
// onLoginClicked()：登录按钮响应
// =============================================================
void LoginWindow::onLoginClicked() {
    const QString username = ui_->loginUsernameEdit->text().trimmed();
    const QString password = ui_->loginPasswordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        ui_->loginStatusLabel->setText("用户名和密码不能为空");
        ui_->loginStatusLabel->show();
        return;
    }

    ui_->loginStatusLabel->hide();
    ui_->loginBtn->setEnabled(false);
    ui_->loginBtn->setText("登录中…");

    qDebug() << "[LoginWindow] 准备登录：用户名 =" << username;
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

    if (username.isEmpty() || display_name.isEmpty() ||
        password.isEmpty() || confirm.isEmpty()) {
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

    qDebug() << "[LoginWindow] 准备注册："
             << "用户名 =" << username
             << "昵称 =" << display_name;
    // TODO（第六章）：AuthService::registerUser(username, display_name, password)
    // TODO（第六章）：下面的 resetRegBtn() 调用移至 onRegisterFailed() 回调中
    resetRegBtn();
}
