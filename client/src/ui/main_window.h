// =============================================================
// client/src/ui/main_window.h
// 最小主窗口
// =============================================================

#pragma once

#include "network/friend_service.h"
#include "network/chat_service.h"

#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QPair>
#include <QString>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QFrame;
class QTextEdit;
class QVBoxLayout;
class QStackedWidget;
class QScrollArea;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(const QString& username,
               cloudvault::FriendService& friend_service,
               cloudvault::ChatService& chat_service,
               QWidget* parent = nullptr);
    ~MainWindow() override;

signals:
    void windowClosed();
    void logoutRequested();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void connectSignals();
    void refreshFriendList(const QList<QPair<QString, bool>>& friends);
    void filterFriendList(const QString& keyword);
    void updateContactSelectionState();
    void updateDeleteFriendAction();
    void renderConversation(const QString& peer);
    void appendConversationMessage(const cloudvault::ChatMessage& message);
    void sendCurrentMessage();
    void refreshCurrentConversation();
    QString formatMessageTime(const QString& timestamp) const;
    void applySelectedFriend();
    void switchMainTab(int index);
    void confirmDeleteSelectedFriend();
    void openOnlineUserDialog();
    void openShareFileDialog();
    QString selectedFriend() const;
    bool selectedFriendOnline() const;

    struct ConversationState {
        QList<cloudvault::ChatMessage> messages;
        int unread_count = 0;
        bool history_loaded = false;
    };

    QString current_username_;
    cloudvault::FriendService& friend_service_;
    cloudvault::ChatService& chat_service_;
    QList<QPair<QString, bool>> friends_;
    QHash<QString, ConversationState> conversations_;

    QWidget* content_root_ = nullptr;
    QFrame* sidebar_panel_ = nullptr;
    QWidget* detail_panel_ = nullptr;
    QVBoxLayout* content_layout_ = nullptr;

    QLabel* sidebar_title_label_ = nullptr;
    QLineEdit* contact_search_edit_ = nullptr;
    QListWidget* contact_list_ = nullptr;
    QPushButton* sidebar_action_btn_ = nullptr;

    QPushButton* message_tab_btn_ = nullptr;
    QPushButton* file_tab_btn_ = nullptr;
    QPushButton* profile_tab_btn_ = nullptr;

    QStackedWidget* center_stack_ = nullptr;

    QLabel* chat_title_label_ = nullptr;
    QLabel* chat_status_label_ = nullptr;
    QLabel* file_path_label_ = nullptr;
    QLabel* profile_name_label_ = nullptr;
    QLabel* profile_id_label_ = nullptr;
    QTextEdit* message_input_ = nullptr;
    QWidget* message_list_widget_ = nullptr;
    QVBoxLayout* message_list_layout_ = nullptr;
    QScrollArea* message_scroll_area_ = nullptr;
    QPushButton* send_message_btn_ = nullptr;
    QPushButton* logout_btn_ = nullptr;
    QPushButton* file_share_btn_ = nullptr;
    QPushButton* delete_friend_btn_ = nullptr;

    QLabel* detail_contact_name_label_ = nullptr;
    QLabel* detail_contact_status_label_ = nullptr;
    QLabel* detail_file_target_label_ = nullptr;
    QLabel* detail_profile_name_label_ = nullptr;
    QLabel* detail_profile_status_label_ = nullptr;
};
