// =============================================================
// client/src/main.cpp
// 客户端程序入口
// =============================================================

#include <QApplication>   // Qt 应用程序对象，管理事件循环
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>          // 用于设置全局字体
#include <QIcon>          // 用于设置应用图标
#include <QScreen>        // 用于获取屏幕尺寸（居中显示）
#include <QWidget>

#include "app.h"

namespace {

QIcon loadApplicationIcon() {
    const QIcon resource_icon(QStringLiteral(":/icons/app_icon.png"));
    if (!resource_icon.isNull()) {
        return resource_icon;
    }

    const QDir current_dir(QDir::currentPath());
    const QDir app_dir(QCoreApplication::applicationDirPath());
    const QStringList candidates = {
        current_dir.filePath(QStringLiteral("client/resources/icons/app_icon.png")),
        current_dir.filePath(QStringLiteral("resources/icons/app_icon.png")),
        current_dir.filePath(QStringLiteral("icons/app_icon.png")),
        app_dir.filePath(QStringLiteral("client/resources/icons/app_icon.png")),
        app_dir.filePath(QStringLiteral("resources/icons/app_icon.png")),
        app_dir.filePath(QStringLiteral("icons/app_icon.png")),
    };

    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            return QIcon(path);
        }
    }
    return {};
}

QString loadGlobalStyleSheet() {
    const QString resource_path = QStringLiteral(":/styles/style.qss");
    QFile qss_file(resource_path);
    if (qss_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(qss_file.readAll());
    }

    const QDir current_dir(QDir::currentPath());
    const QDir app_dir(QCoreApplication::applicationDirPath());
    const QStringList candidates = {
        current_dir.filePath(QStringLiteral("client/resources/styles/style.qss")),
        current_dir.filePath(QStringLiteral("resources/styles/style.qss")),
        app_dir.filePath(QStringLiteral("client/resources/styles/style.qss")),
        app_dir.filePath(QStringLiteral("resources/styles/style.qss")),
    };

    for (const QString& path : candidates) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(file.readAll());
        }
    }
    return {};
}

} // namespace

int main(int argc, char* argv[]) {
    // QApplication 必须是第一个创建的 Qt 对象。
    // 它负责：初始化 Qt、管理事件循环、处理命令行参数。
    QApplication app(argc, argv);

    // 设置应用程序元信息（影响 QSettings 存储路径等）
    app.setApplicationName("CloudVault");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("CloudVault");

    const QIcon app_icon = loadApplicationIcon();
    if (!app_icon.isNull()) {
        app.setWindowIcon(app_icon);
        QApplication::setWindowIcon(app_icon);
    }

    QFont font;
    font.setFamilies({QStringLiteral("DM Sans"),
                      QStringLiteral("PingFang SC"),
                      QStringLiteral("Microsoft YaHei UI"),
                      QStringLiteral("Segoe UI")});
    font.setPointSizeF(10.0);
    app.setFont(font);

    const QString global_qss = loadGlobalStyleSheet();
    if (!global_qss.isEmpty()) {
        app.setStyleSheet(global_qss);
    }

    App app_controller;
    QWidget* window = app_controller.initialWindow();
    if (!window) {
        return 1;
    }

    if (!app_icon.isNull()) {
        window->setWindowIcon(app_icon);
    }

    // 居中显示：先检查 primaryScreen() 是否有效（WSL 下偶尔为 nullptr）
    if (auto* screen = QGuiApplication::primaryScreen()) {
        const QRect screen_geo = screen->availableGeometry();
        window->move(screen_geo.center() - window->rect().center());
    }

    window->show();

    // app.exec()：启动 Qt 事件循环，阻塞直到窗口关闭
    // 返回值是退出码（正常退出返回 0）
    return app.exec();
}
