// =============================================================
// client/src/ui/online_user_dialog.h
// 在线用户弹窗
// =============================================================

#pragma once

#include "service/friend_service.h"

#include <QDialog>
#include <QList>
#include <QString>

class QListWidget;
class QPushButton;
class QLineEdit;
class QLabel;

class OnlineUserDialog : public QDialog {
    Q_OBJECT

public:
    explicit OnlineUserDialog(const QList<cloudvault::FriendProfile>& users,
                              QWidget* parent = nullptr);
    void setUsers(const QList<cloudvault::FriendProfile>& users);

signals:
    void refreshRequested();
    void userChosen(const QString& username);

private:
    void setupUi();
    void connectSignals();
    void populateList(const QString& keyword = QString());

    QList<cloudvault::FriendProfile> users_;
    QLabel* title_label_ = nullptr;
    QLineEdit* search_edit_ = nullptr;
    QListWidget* user_list_ = nullptr;
    QPushButton* refresh_btn_ = nullptr;
    QPushButton* add_btn_ = nullptr;
};
