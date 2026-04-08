// =============================================================
// client/src/ui/contact_panel.h
// 联系人与群组主区面板
// =============================================================

#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QStackedWidget;

class ContactPanel : public QWidget {
    Q_OBJECT

public:
    enum class State {
        Empty,
        Friend,
        Group,
        FriendRequestIncoming,
        FriendRequestOutgoing,
    };

    explicit ContactPanel(const QString& current_username, QWidget* parent = nullptr);

    void showEmptyState();
    void showFriendProfile(const QString& username,
                           const QString& display_name,
                           const QString& signature,
                           bool online);
    void showFriendRequest(const QString& username, bool incoming);
    void showGroupProfile(int group_id,
                          const QString& group_name,
                          int owner_id,
                          int online_count);
    State state() const;

signals:
    void sendMessageRequested(const QString& username);
    void shareFileRequested(const QString& username);
    void deleteFriendRequested(const QString& username);
    void acceptFriendRequestRequested(const QString& username);
    void openGroupChatRequested(int group_id);
    void viewGroupMembersRequested(int group_id);
    void leaveGroupRequested(int group_id);

private:
    void updateAvatar(const QString& seed, int size);

    QString current_username_;
    QString current_friend_username_;
    int current_group_id_ = 0;
    State state_ = State::Empty;

    QStackedWidget* stack_ = nullptr;

    QLabel* avatar_label_ = nullptr;
    QLabel* title_label_ = nullptr;
    QLabel* subtitle_label_ = nullptr;
    QLabel* summary_label_ = nullptr;
    QLabel* friend_username_value_label_ = nullptr;
    QLabel* friend_nickname_value_label_ = nullptr;
    QLabel* friend_signature_value_label_ = nullptr;

    QPushButton* send_message_btn_ = nullptr;
    QPushButton* share_file_btn_ = nullptr;
    QPushButton* delete_friend_btn_ = nullptr;
    QPushButton* accept_friend_request_btn_ = nullptr;

    QLabel* group_avatar_label_ = nullptr;
    QLabel* group_title_label_ = nullptr;
    QLabel* group_subtitle_label_ = nullptr;
    QLabel* group_summary_label_ = nullptr;
    QLabel* group_id_value_label_ = nullptr;
    QLabel* group_owner_value_label_ = nullptr;
    QLabel* group_online_value_label_ = nullptr;

    QPushButton* open_group_chat_btn_ = nullptr;
    QPushButton* view_group_members_btn_ = nullptr;
    QPushButton* leave_group_btn_ = nullptr;
};
