// =============================================================
// client/src/ui/main_window.h
// 最小主窗口
// =============================================================

#pragma once

#include <QMainWindow>

namespace cloudvault { class FriendService; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(const QString& username,
               cloudvault::FriendService& friend_service,
               QWidget* parent = nullptr);
    ~MainWindow() override;

signals:
    void windowClosed();

protected:
    void closeEvent(QCloseEvent* event) override;
};
