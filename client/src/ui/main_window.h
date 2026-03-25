// =============================================================
// client/src/ui/main_window.h
// 最小主窗口
// =============================================================

#pragma once

#include "network/friend_service.h"

#include <QList>
#include <QMainWindow>
#include <QPair>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;

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

private:
    void setupUi();
    void connectSignals();
    void refreshFriendList(const QList<QPair<QString, bool>>& friends);
    void filterFriendList(const QString& keyword);
    void applySelectedFriend();
    void switchMainTab(int index);
    QString selectedFriend() const;
    bool selectedFriendOnline() const;

    QString current_username_;
    cloudvault::FriendService& friend_service_;
    QList<QPair<QString, bool>> friends_;

    QLineEdit* contact_search_edit_ = nullptr;
    QListWidget* contact_list_ = nullptr;

    QPushButton* message_tab_btn_ = nullptr;
    QPushButton* file_tab_btn_ = nullptr;
    QPushButton* profile_tab_btn_ = nullptr;

    QStackedWidget* center_stack_ = nullptr;
    QStackedWidget* detail_stack_ = nullptr;

    QLabel* chat_title_label_ = nullptr;
    QLabel* chat_status_label_ = nullptr;
    QLabel* file_path_label_ = nullptr;
    QLabel* profile_name_label_ = nullptr;
    QLabel* profile_id_label_ = nullptr;

    QLabel* detail_contact_name_label_ = nullptr;
    QLabel* detail_contact_status_label_ = nullptr;
    QLabel* detail_file_target_label_ = nullptr;
    QLabel* detail_profile_name_label_ = nullptr;
    QLabel* detail_profile_status_label_ = nullptr;
};
