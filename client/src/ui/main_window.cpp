// =============================================================
// client/src/ui/main_window.cpp
// 最小主窗口
// =============================================================

#include "main_window.h"

#include "ui/friend_page.h"

#include <QCloseEvent>

MainWindow::MainWindow(const QString& username,
                       cloudvault::FriendService& friend_service,
                       QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("CloudVault - %1").arg(username));
    resize(900, 620);
    setCentralWidget(new FriendPage(username, friend_service, this));
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    emit windowClosed();
    QMainWindow::closeEvent(event);
}
