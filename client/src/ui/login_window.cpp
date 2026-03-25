// =============================================================
// client/src/ui/login_window.cpp
// 登录窗口完整实现
//
// 架构说明：
//   构造函数调用三个私有方法完成初始化：
//     setupUi()        → 创建控件、搭建布局
//     setupStyle()     → 应用 QSS 样式
//     connectSignals() → 绑定信号槽
//
// 第五章限制：槽函数只做本地校验 + qDebug 输出，
//            第六章接入 AuthService 后会修改这里。
// =============================================================

// .cpp 文件中才 #include 完整的 Qt 头文件
// 头文件中用前向声明，.cpp 中才真正包含，这是标准做法
#include "login_window.h"

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

// =============================================================
// 构造函数
// =============================================================
LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent)  // 调用基类构造函数，传入 parent 指针
{
    // 设置窗口标题（显示在标题栏）
    setWindowTitle("CloudHive");

    // 只保留关闭按钮，去掉最小化和最大化按钮
    // Qt::Window             ：这是一个独立顶层窗口
    // Qt::WindowTitleHint    ：显示标题栏
    // Qt::WindowCloseButtonHint：显示关闭按钮（×）
    // 不加 WindowMinimizeButtonHint / WindowMaximizeButtonHint → 这两个按钮消失
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    // 固定窗口大小（宽 × 高），setFixedSize 同时禁止水平和垂直方向拖拽
    setFixedSize(400, 560);

    // 按顺序初始化 UI 的三个阶段
    setupUi();        // 1. 搭骨架
    setupStyle();     // 2. 刷样式
    connectSignals(); // 3. 接信号
}

// =============================================================
// setupUi()：创建所有控件并搭建布局层次
//
// 布局树（对应 ui-flow.md S-01）：
//   LoginWindow
//   └── main_layout（QVBoxLayout，外边距 40px）
//       └── card（QWidget，白色背景）
//           └── card_layout（QVBoxLayout，内边距 32px，间距 24px）
//               ├── header_widget（Logo + 标题）
//               ├── tab_widget_（登录/注册 Tab）
//               └── status_frame（服务器状态栏）
// =============================================================
void LoginWindow::setupUi() {
    // ── 最外层布局：给窗口本身设置背景色用 ──────────────────
    // QVBoxLayout 将子控件垂直排列
    auto* main_layout = new QVBoxLayout(this);
    // setContentsMargins(左, 上, 右, 下)：布局与窗口边缘的距离
    main_layout->setContentsMargins(40, 40, 40, 40);
    // setAlignment：当子控件不占满空间时的对齐方式
    main_layout->setAlignment(Qt::AlignTop);

    // ── 白色卡片容器 ──────────────────────────────────────────
    // card 本身是一个无内容的 QWidget，通过 QSS 设置白色背景和圆角
    // 所有可见控件都放在 card 内部
    auto* card = new QWidget(this);
    card->setObjectName("card");  // objectName 供 QSS 精确定位

    auto* card_layout = new QVBoxLayout(card);
    card_layout->setContentsMargins(32, 32, 32, 32);
    card_layout->setSpacing(24);  // 子控件之间的垂直间距

    main_layout->addWidget(card);

    // ── Header：图标 + 应用名 + 副标题 ───────────────────────
    auto* header_widget = new QWidget(card);
    auto* header_layout = new QHBoxLayout(header_widget);
    header_layout->setContentsMargins(0, 0, 0, 0);
    header_layout->setSpacing(12);

    // 应用图标（用 Emoji 临时代替，第九章替换为真实图标）
    auto* icon_label = new QLabel("💬", header_widget);
    icon_label->setObjectName("appIcon");

    // 右侧标题区域（垂直排列：应用名 + 副标题）
    auto* title_widget = new QWidget(header_widget);
    auto* title_layout = new QVBoxLayout(title_widget);
    title_layout->setContentsMargins(0, 0, 0, 0);
    title_layout->setSpacing(2);

    auto* app_name_label = new QLabel("CloudHive", title_widget);
    app_name_label->setObjectName("appName");

    auto* subtitle_label = new QLabel("即时通讯 · 文件传输", title_widget);
    subtitle_label->setObjectName("appSubtitle");

    title_layout->addWidget(app_name_label);
    title_layout->addWidget(subtitle_label);

    header_layout->addWidget(icon_label);
    header_layout->addWidget(title_widget);
    header_layout->addStretch();  // 弹性空间，把内容推向左侧

    card_layout->addWidget(header_widget);

    // ── Tab 控件：「登录」和「注册」两个标签页 ───────────────
    // QTabWidget 管理多个页面，用户点击标签切换
    tab_widget_ = new QTabWidget(card);
    tab_widget_->setObjectName("mainTab");

    // ┌────────────────────────────────────────┐
    // │ 登录 Tab                               │
    // └────────────────────────────────────────┘
    auto* login_tab = new QWidget();
    auto* login_layout = new QVBoxLayout(login_tab);
    login_layout->setContentsMargins(0, 16, 0, 0);
    login_layout->setSpacing(6);

    // 用户名标签 + 输入框
    auto* login_user_label = new QLabel("用户名", login_tab);
    login_user_label->setObjectName("fieldLabel");
    login_username_edit_ = new QLineEdit(login_tab);
    login_username_edit_->setPlaceholderText("请输入用户名");
    login_username_edit_->setObjectName("fieldEdit");

    // 密码标签 + 输入框
    auto* login_pwd_label = new QLabel("密码", login_tab);
    login_pwd_label->setObjectName("fieldLabel");
    login_password_edit_ = new QLineEdit(login_tab);
    login_password_edit_->setPlaceholderText("请输入密码");
    // QLineEdit::Password 模式：输入时显示圆点而非明文
    login_password_edit_->setEchoMode(QLineEdit::Password);
    login_password_edit_->setObjectName("fieldEdit");

    // 状态/错误提示标签（初始隐藏，出错时 show()）
    login_status_label_ = new QLabel("", login_tab);
    login_status_label_->setObjectName("statusLabel");
    login_status_label_->setWordWrap(true);  // 文字过长时自动换行
    login_status_label_->hide();             // 初始隐藏

    // 登录按钮
    login_btn_ = new QPushButton("登录", login_tab);
    login_btn_->setObjectName("primaryBtn");
    login_btn_->setCursor(Qt::PointingHandCursor);  // 鼠标悬停时变成手形

    // 加间距让按钮和上方内容有呼吸感
    login_layout->addWidget(login_user_label);
    login_layout->addWidget(login_username_edit_);
    login_layout->addSpacing(4);
    login_layout->addWidget(login_pwd_label);
    login_layout->addWidget(login_password_edit_);
    login_layout->addWidget(login_status_label_);
    login_layout->addSpacing(8);
    login_layout->addWidget(login_btn_);
    login_layout->addStretch();  // 把内容顶到上方，底部留白

    // ┌────────────────────────────────────────┐
    // │ 注册 Tab                               │
    // └────────────────────────────────────────┘
    auto* reg_tab = new QWidget();
    auto* reg_layout = new QVBoxLayout(reg_tab);
    reg_layout->setContentsMargins(0, 16, 0, 0);
    reg_layout->setSpacing(6);

    // 用户名 + 昵称：并排两列（QHBoxLayout）
    auto* name_row = new QWidget(reg_tab);
    auto* name_row_layout = new QHBoxLayout(name_row);
    name_row_layout->setContentsMargins(0, 0, 0, 0);
    name_row_layout->setSpacing(8);

    reg_username_edit_ = new QLineEdit(name_row);
    reg_username_edit_->setPlaceholderText("用户名");
    reg_username_edit_->setObjectName("fieldEdit");

    reg_display_name_edit_ = new QLineEdit(name_row);
    reg_display_name_edit_->setPlaceholderText("昵称");
    reg_display_name_edit_->setObjectName("fieldEdit");

    name_row_layout->addWidget(reg_username_edit_);
    name_row_layout->addWidget(reg_display_name_edit_);

    // 密码
    auto* reg_pwd_label = new QLabel("密码（至少 8 位）", reg_tab);
    reg_pwd_label->setObjectName("fieldLabel");
    reg_password_edit_ = new QLineEdit(reg_tab);
    reg_password_edit_->setPlaceholderText("请设置密码");
    reg_password_edit_->setEchoMode(QLineEdit::Password);
    reg_password_edit_->setObjectName("fieldEdit");

    // 确认密码
    auto* reg_confirm_label = new QLabel("确认密码", reg_tab);
    reg_confirm_label->setObjectName("fieldLabel");
    reg_confirm_edit_ = new QLineEdit(reg_tab);
    reg_confirm_edit_->setPlaceholderText("再次输入密码");
    reg_confirm_edit_->setEchoMode(QLineEdit::Password);
    reg_confirm_edit_->setObjectName("fieldEdit");

    // 注册状态提示
    reg_status_label_ = new QLabel("", reg_tab);
    reg_status_label_->setObjectName("statusLabel");
    reg_status_label_->setWordWrap(true);
    reg_status_label_->hide();

    // 注册按钮
    reg_btn_ = new QPushButton("创建账号", reg_tab);
    reg_btn_->setObjectName("primaryBtn");
    reg_btn_->setCursor(Qt::PointingHandCursor);

    reg_layout->addWidget(name_row);
    reg_layout->addSpacing(4);
    reg_layout->addWidget(reg_pwd_label);
    reg_layout->addWidget(reg_password_edit_);
    reg_layout->addSpacing(4);
    reg_layout->addWidget(reg_confirm_label);
    reg_layout->addWidget(reg_confirm_edit_);
    reg_layout->addWidget(reg_status_label_);
    reg_layout->addSpacing(8);
    reg_layout->addWidget(reg_btn_);
    reg_layout->addStretch();

    // 将两个 Tab 页面添加到 QTabWidget
    tab_widget_->addTab(login_tab, "登录");
    tab_widget_->addTab(reg_tab, "注册");

    card_layout->addWidget(tab_widget_);

    // ── 底部服务器状态栏 ──────────────────────────────────────
    // 一个带背景色的小面板，显示当前服务器连接状态
    auto* status_frame = new QWidget(card);
    status_frame->setObjectName("statusFrame");

    auto* status_layout = new QHBoxLayout(status_frame);
    status_layout->setContentsMargins(12, 8, 12, 8);
    status_layout->setSpacing(8);

    // 圆点：用 "●" 字符模拟，颜色由 QSS 控制
    server_dot_label_ = new QLabel("●", status_frame);
    server_dot_label_->setObjectName("serverDot");

    // 地址和状态文字（使用等宽字体让 IP 地址对齐更美观）
    server_addr_label_ = new QLabel("127.0.0.1:5000 · 已就绪", status_frame);
    server_addr_label_->setObjectName("serverAddr");

    status_layout->addWidget(server_dot_label_);
    status_layout->addWidget(server_addr_label_);
    status_layout->addStretch();

    card_layout->addWidget(status_frame);
}

// =============================================================
// setupStyle()：应用 QSS 样式表
//
// QSS（Qt Style Sheets）语法与 CSS 类似，但有以下区别：
//   - 不支持 CSS 变量（var(--color)），颜色需硬编码
//   - 选择器使用 ClassName 或 ClassName#objectName
//   - 属性名与 CSS 略有不同（如 border-radius 相同，但 background 不写 -color）
//
// 色彩系统（来自 ui-flow.md）：
//   accent:  #3B82F6（主色蓝）
//   bg:      #F4F6F8（窗口背景浅灰）
//   surface: #FFFFFF（卡片白色）
//   surface2:#F0F2F5（输入框背景、状态栏背景）
//   border:  #E2E6EA（边框浅灰）
//   text:    #111827（主文字深色）
//   text2:   #6B7280（次级文字灰色）
//   danger:  #EF4444（错误红色）
//   success: #22C55E（成功绿色）
// =============================================================
void LoginWindow::setupStyle() {
    // setStyleSheet() 应用到 LoginWindow 及其所有子控件
    // Qt 的 QSS 会向下继承，子控件会受父控件样式影响
    setStyleSheet(R"(
        /* ── 窗口背景 ──────────────────────────────── */
        LoginWindow {
            background-color: #F4F6F8;
        }

        /* ── 白色卡片 ──────────────────────────────── */
        /* #card 是 objectName 选择器，只匹配 objectName="card" 的 QWidget */
        QWidget#card {
            background-color: #FFFFFF;
            border-radius: 16px;
            /* QSS 不支持 box-shadow，用边框模拟轮廓 */
            border: 1px solid #E2E6EA;
        }

        /* ── 应用图标（Emoji） ─────────────────────── */
        QLabel#appIcon {
            font-size: 36px;
        }

        /* ── 应用名称 ──────────────────────────────── */
        QLabel#appName {
            font-size: 20px;
            font-weight: bold;
            color: #111827;
        }

        /* ── 副标题 ────────────────────────────────── */
        QLabel#appSubtitle {
            font-size: 13px;
            color: #6B7280;
        }

        /* ── Tab 控件整体 ──────────────────────────── */
        /* pane：Tab 页面的容器区域，去掉默认边框 */
        QTabWidget#mainTab::pane {
            border: none;
            background: transparent;
        }

        /* ── Tab 标签栏 ────────────────────────────── */
        QTabWidget#mainTab QTabBar::tab {
            padding: 8px 28px;
            color: #6B7280;
            background: transparent;
            border: none;
            border-bottom: 2px solid transparent;
            font-size: 14px;
        }

        /* 选中的 Tab 标签 */
        QTabWidget#mainTab QTabBar::tab:selected {
            color: #3B82F6;
            border-bottom: 2px solid #3B82F6;
            font-weight: bold;
        }

        /* 鼠标悬停的 Tab 标签 */
        QTabWidget#mainTab QTabBar::tab:hover:!selected {
            color: #374151;
        }

        /* ── 字段标签（"用户名"、"密码"等） ─────────── */
        QLabel#fieldLabel {
            font-size: 12px;
            color: #6B7280;
            margin-bottom: 2px;
        }

        /* ── 输入框 ────────────────────────────────── */
        QLineEdit#fieldEdit {
            height: 44px;
            border: 1.5px solid #E2E6EA;
            border-radius: 8px;
            background: #F0F2F5;
            padding: 0 12px;
            font-size: 14px;
            color: #111827;
        }

        /* 输入框聚焦时：蓝色边框 + 白色背景 */
        QLineEdit#fieldEdit:focus {
            border-color: #3B82F6;
            background: #FFFFFF;
        }

        /* ── 状态/错误提示标签 ─────────────────────── */
        QLabel#statusLabel {
            font-size: 12px;
            color: #EF4444;
            padding: 4px 0;
        }

        /* ── 主色按钮（登录/创建账号） ──────────────── */
        QPushButton#primaryBtn {
            height: 44px;
            border-radius: 8px;
            background: #3B82F6;
            color: white;
            font-size: 15px;
            font-weight: bold;
            border: none;
        }

        /* 鼠标悬停：深一点的蓝 */
        QPushButton#primaryBtn:hover {
            background: #2563EB;
        }

        /* 鼠标按下：更深的蓝 */
        QPushButton#primaryBtn:pressed {
            background: #1D4ED8;
        }

        /* 禁用状态（点击后等待响应时）：浅蓝，不可交互 */
        QPushButton#primaryBtn:disabled {
            background: #93C5FD;
        }

        /* ── 服务器状态栏 ──────────────────────────── */
        QWidget#statusFrame {
            background: #F0F2F5;
            border-radius: 8px;
        }

        /* 状态圆点：绿色表示已就绪 */
        QLabel#serverDot {
            color: #22C55E;
            font-size: 10px;
        }

        /* 服务器地址文字：等宽字体 */
        QLabel#serverAddr {
            font-family: "Consolas", "Courier New", monospace;
            font-size: 12px;
            color: #6B7280;
        }
    )");
}

// =============================================================
// connectSignals()：绑定信号（Signal）和槽（Slot）
//
// Qt 信号槽语法：
//   connect(发送者, &发送者类::信号名, 接收者, &接收者类::槽名);
//
// 这里使用新式语法（Qt5+），编译期检查类型，比旧式 SIGNAL/SLOT 宏安全。
// =============================================================
void LoginWindow::connectSignals() {
    // 「登录」按钮的 clicked 信号 → onLoginClicked() 槽
    connect(login_btn_, &QPushButton::clicked,
            this, &LoginWindow::onLoginClicked);

    // 「创建账号」按钮的 clicked 信号 → onRegisterClicked() 槽
    connect(reg_btn_, &QPushButton::clicked,
            this, &LoginWindow::onRegisterClicked);
}

// =============================================================
// onLoginClicked()：点击「登录」按钮的响应
//
// 第五章：只做本地校验，不发网络请求。
// 第六章：在 "TODO" 处接入 AuthService::login()。
// =============================================================
void LoginWindow::onLoginClicked() {
    // 获取输入内容，trimmed() 去掉首尾空白（防止只输入空格的情况）
    const QString username = login_username_edit_->text().trimmed();
    const QString password = login_password_edit_->text();

    // ── 校验：字段不能为空 ────────────────────────────────
    if (username.isEmpty() || password.isEmpty()) {
        login_status_label_->setText("用户名和密码不能为空");
        login_status_label_->show();
        return;  // 直接返回，不继续执行
    }

    // ── 校验通过：模拟发送中状态 ──────────────────────────
    // 隐藏错误提示（可能之前有错误）
    login_status_label_->hide();

    // 禁用按钮防止重复点击，修改文字给用户反馈
    login_btn_->setEnabled(false);
    login_btn_->setText("登录中…");

    // 调试输出（在 CLion 的 Run 控制台可见）
    // 第六章会替换为 AuthService::login(username, password) 调用
    qDebug() << "[LoginWindow] 准备登录：用户名 =" << username;

    // TODO（第六章）：调用 AuthService::login(username, password)
    // 成功 → 打开主窗口
    // 失败 → 恢复按钮，显示错误信息
}

// =============================================================
// onRegisterClicked()：点击「创建账号」按钮的响应
//
// 第五章：只做本地校验，第六章接入 AuthService::registerUser()。
// =============================================================
void LoginWindow::onRegisterClicked() {
    // 获取所有注册字段
    const QString username     = reg_username_edit_->text().trimmed();
    const QString display_name = reg_display_name_edit_->text().trimmed();
    const QString password     = reg_password_edit_->text();
    const QString confirm      = reg_confirm_edit_->text();

    // ── 辅助 lambda：显示错误信息并返回 ──────────────────
    // 用 lambda 避免重复 show()/setText() 代码
    auto showError = [this](const QString& msg) {
        reg_status_label_->setText(msg);
        reg_status_label_->setStyleSheet("color: #EF4444;");
        reg_status_label_->show();
    };

    // ── 校验 1：所有字段必填 ──────────────────────────────
    if (username.isEmpty() || display_name.isEmpty() ||
        password.isEmpty() || confirm.isEmpty()) {
        showError("所有字段均为必填项");
        return;
    }

    // ── 校验 2：用户名长度 ────────────────────────────────
    if (username.length() < 3 || username.length() > 20) {
        showError("用户名长度须在 3–20 个字符之间");
        return;
    }

    // ── 校验 3：密码至少 8 位 ─────────────────────────────
    if (password.length() < 8) {
        showError("密码至少需要 8 位字符");
        return;
    }

    // ── 校验 4：两次密码必须一致 ──────────────────────────
    if (password != confirm) {
        showError("两次输入的密码不一致");
        return;
    }

    // ── 全部通过：模拟注册中状态 ──────────────────────────
    reg_status_label_->hide();
    reg_btn_->setEnabled(false);
    reg_btn_->setText("注册中…");

    qDebug() << "[LoginWindow] 准备注册："
             << "用户名 =" << username
             << "昵称 =" << display_name;

    // TODO（第六章）：调用 AuthService::registerUser(username, display_name, password)
    // 成功 → 切换到登录 Tab，提示注册成功
    // 失败 → 恢复按钮，显示服务端返回的错误信息
}
