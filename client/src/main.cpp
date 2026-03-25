// =============================================================
// client/src/main.cpp
// 客户端程序入口
// =============================================================

#include <QApplication>   // Qt 应用程序对象，管理事件循环
#include <QFont>          // 用于设置全局字体
#include <QScreen>        // 用于获取屏幕尺寸（居中显示）

#include "ui/login_window.h"

int main(int argc, char* argv[]) {
    // QApplication 必须是第一个创建的 Qt 对象。
    // 它负责：初始化 Qt、管理事件循环、处理命令行参数。
    QApplication app(argc, argv);

    // 设置应用程序元信息（影响 QSettings 存储路径等）
    app.setApplicationName("CloudHive");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("CloudHive");

    // 设置全局字体：优先使用 Noto Sans CJK（支持中文），
    // 逗号后是备选字体，Qt 会按顺序查找第一个可用的
    QFont font("Noto Sans CJK SC, Noto Sans SC, WenQuanYi Micro Hei, Sans");
    font.setPointSize(10);
    app.setFont(font);

    // 创建并显示登录窗口
    // 注意：LoginWindow 在栈上创建，QApplication 退出前它不会析构
    LoginWindow window;

    // 居中显示：先检查 primaryScreen() 是否有效（WSL 下偶尔为 nullptr）
    if (auto* screen = QGuiApplication::primaryScreen()) {
        const QRect screen_geo = screen->availableGeometry();
        window.move(screen_geo.center() - window.rect().center());
    }

    window.show();

    // app.exec()：启动 Qt 事件循环，阻塞直到窗口关闭
    // 返回值是退出码（正常退出返回 0）
    return app.exec();
}
