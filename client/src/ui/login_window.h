// =============================================================
// client/src/ui/login_window.h
// 登录窗口类声明
//
// 设计原则：
//   1. 头文件只做"声明"，不做"定义"
//   2. Qt 控件用前向声明（forward declaration）代替 #include，
//      减少编译依赖、加快构建速度
//   3. 成员变量全部用指针（Qt parent-child 内存管理要求堆分配）
//   4. 命名规则：成员变量末尾加下划线（参见 conventions.md）
// =============================================================

#pragma once  // 防止头文件被重复包含（比 #ifndef 守卫更简洁）

#include <QWidget>  // LoginWindow 继承自 QWidget，必须完整包含

// ── 前向声明 ──────────────────────────────────────────────────
// 告诉编译器"这些类存在"，但不需要知道它们的完整定义。
// 这样头文件就不需要 #include <QTabWidget> 等大头文件，
// 节省编译时间。完整 #include 放在 .cpp 文件里。
class QTabWidget;
class QLineEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;

// ── LoginWindow 类声明 ─────────────────────────────────────────
// 继承 QWidget：QWidget 是所有 Qt 窗口/控件的基类。
// explicit：防止隐式类型转换（良好实践）。
class LoginWindow : public QWidget {
    Q_OBJECT  // 必须写！MOC（元对象编译器）靠它生成信号槽代码。
              // 少了这一行，connect() 连接的槽函数将无法调用。

public:
    // parent 默认 nullptr → 这是顶层窗口（没有父控件）
    // 顶层窗口会出现在任务栏，有自己的标题栏
    explicit LoginWindow(QWidget* parent = nullptr);

    // override：明确告诉编译器这是重写虚函数，有助于发现错误
    // = default：使用编译器自动生成的析构函数
    // Qt 的 parent-child 机制会自动析构子控件，所以这里不需要手动 delete
    ~LoginWindow() override = default;

private slots:
    // slots 关键字是 Qt 扩展语法，表示这些方法可以作为"槽"被信号触发
    // 命名规则：on + 触发对象 + 事件（camelCase，Qt 社区惯例）

    void onLoginClicked();     // 点击「登录」按钮时调用
    void onRegisterClicked();  // 点击「创建账号」按钮时调用

private:
    // ── UI 初始化三步曲（在构造函数中按顺序调用）────────────
    void setupUi();        // 第一步：创建控件、搭建布局结构
    void setupStyle();     // 第二步：应用 QSS 样式表（颜色、字体、圆角等）
    void connectSignals(); // 第三步：绑定信号（按钮点击）→ 槽（响应函数）

    // ── Tab 容器 ──────────────────────────────────────────────
    // QTabWidget：带标签页的容器，管理「登录」和「注册」两个 Tab
    QTabWidget* tab_widget_;

    // ── 登录 Tab 的控件 ───────────────────────────────────────
    QLineEdit*   login_username_edit_;  // 用户名输入框
    QLineEdit*   login_password_edit_;  // 密码输入框（设为 Password 模式后字符变成圆点）
    QPushButton* login_btn_;            // 「登录」按钮
    QLabel*      login_status_label_;   // 错误提示文字（初始隐藏，出错时 show()）

    // ── 注册 Tab 的控件 ───────────────────────────────────────
    QLineEdit*   reg_username_edit_;      // 注册用户名（不可重复，用于登录）
    QLineEdit*   reg_display_name_edit_;  // 昵称（可重复，显示用）
    QLineEdit*   reg_password_edit_;      // 密码（Password 模式）
    QLineEdit*   reg_confirm_edit_;       // 确认密码（两次必须一致）
    QPushButton* reg_btn_;                // 「创建账号」按钮
    QLabel*      reg_status_label_;       // 注册结果提示（初始隐藏）

    // ── 底部服务器状态栏 ──────────────────────────────────────
    QLabel* server_dot_label_;   // 彩色圆点 "●"，颜色表示连接状态（绿=已就绪）
    QLabel* server_addr_label_;  // 服务器地址和状态文字，如 "127.0.0.1:5000 · 已就绪"
};
