// =============================================================
// client/src/ui/login_window.h
// 登录窗口类声明（.ui 文件版本）
//
// 与纯代码版本的区别：
//   - 不再手动声明每个控件成员变量
//   - 改用 Ui::LoginWindow* ui_，由 AUTOUIC 生成的 ui_login_window.h 提供
//   - 控件通过 ui_->widgetName 访问，名字来自 .ui 文件中的 objectName
// =============================================================

#pragma once

#include <QWidget>

// 前向声明 Ui::LoginWindow（完整定义在 AUTOUIC 生成的 ui_login_window.h 中）
// 这样头文件就不需要 #include "ui_login_window.h"，减少编译依赖
QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override;  // 需要显式定义以 delete ui_

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    void setupStyle();      // 应用 QSS 样式表
    void connectSignals();  // 绑定信号槽
    void resetLoginBtn();   // 恢复登录按钮为可点击状态
    void resetRegBtn();     // 恢复注册按钮为可点击状态

    Ui::LoginWindow* ui_;   // 指向自动生成的 UI 结构体，管理所有控件
};
