// =============================================================
// client/src/ui/friend_page.h
// 好友页
// =============================================================

#pragma once

#include "network/friend_service.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

class FriendPage : public QWidget {
    Q_OBJECT

public:
    FriendPage(const QString& current_username,
               cloudvault::FriendService& friend_service,
               QWidget* parent = nullptr);

private slots:
    void onSearchClicked();
    void onAddClicked();
    void onRefreshClicked();
    void onDeleteClicked();
    void onFriendSelectionChanged();

private:
    void setupUi();
    void connectSignals();
    void setStatus(const QString& text, bool error = false);
    QString selectedFriend() const;

    QString                     current_username_;
    QString                     searched_username_;
    cloudvault::FriendService&  friend_service_;

    QLineEdit*    search_edit_ = nullptr;
    QPushButton*  search_btn_ = nullptr;
    QPushButton*  add_btn_ = nullptr;
    QPushButton*  refresh_btn_ = nullptr;
    QPushButton*  delete_btn_ = nullptr;
    QLabel*       search_result_label_ = nullptr;
    QLabel*       status_label_ = nullptr;
    QListWidget*  friends_list_ = nullptr;
};
