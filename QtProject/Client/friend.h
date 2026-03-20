#ifndef FRIEND_H
#define FRIEND_H

#include "chat.h"
#include "onlineuser.h"

#include <QWidget>

namespace Ui {
class Friend;
}

class Friend : public QWidget
{
    Q_OBJECT

public:
    explicit Friend(QWidget *parent = nullptr);
    ~Friend();
    OnlineUser* m_pOnlineUser;
    void flushFriend();
    void updateFriendrList(QStringList FriendList);
    Chat* m_pChat;
    QListWidget* getFriend_LW();

private slots:
    void on_findUser_PB_clicked();

    void on_onlineUser_PB_clicked();

    void on_flushFriend_PB_clicked();

    void on_delFriend_PB_clicked();

    void on_chat_PB_clicked();

private:
    Ui::Friend *ui;
};

#endif // FRIEND_H
