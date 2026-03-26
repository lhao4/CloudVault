// =============================================================
// client/src/ui/main_window.h
// 最小主窗口
// =============================================================

#pragma once

#include "network/chat_service.h"
#include "network/friend_service.h"
#include "network/file_service.h"
#include "network/share_service.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QProgressBar;
class QPushButton;
class QFrame;
class QSplitter;
class QTextEdit;
class QVBoxLayout;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(const QString& username,
               cloudvault::ChatService& chat_service,
               cloudvault::FriendService& friend_service,
               cloudvault::FileService& file_service,
               cloudvault::ShareService& share_service,
               QWidget* parent = nullptr);
    ~MainWindow() override;

signals:
    void windowClosed();
    void logoutRequested();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    struct ConversationSummary {
        QString preview;
        QString timestamp;
        int unread_count = 0;
        bool has_message = false;
        bool last_message_outgoing = false;
    };

    void setupUi();
    void connectSignals();
    void appendChatMessage(const cloudvault::ChatMessage& message);
    void appendDateDividerIfNeeded(const QString& timestamp);
    void rebuildMessageList(const QList<cloudvault::ChatMessage>& messages);
    void showChatEmptyState();
    void showChatConversation(const QString& username, bool online);
    void requestConversationSnapshot(const QString& peer);
    void updateConversationSummary(const QString& peer,
                                   const QList<cloudvault::ChatMessage>& messages);
    void applyConversationMessage(const cloudvault::ChatMessage& message, bool increment_unread);
    void clearConversationUnread(const QString& peer);
    int calculateUnreadCount(const QString& peer,
                             const QList<cloudvault::ChatMessage>& messages) const;
    QDateTime loadConversationReadAt(const QString& peer) const;
    void saveConversationReadAt(const QString& peer, const QDateTime& timestamp) const;
    void refreshFriendList(const QList<QPair<QString, bool>>& friends);
    void filterFriendList(const QString& keyword);
    void updateContactSelectionState();
    void applySelectedFriend();
    void sendCurrentMessage();
    void refreshFileList(const QString& path, const cloudvault::FileEntries& entries);
    void updateFileSelectionState();
    void applySelectedFile();
    void setFileStatus(const QString& message, bool error = false);
    void resetFileActionSummary();
    void navigateToFilePath(const QString& path);
    void openCurrentParentDirectory();
    void createDirectory();
    void renameSelectedFile();
    void moveSelectedFile();
    void deleteSelectedFile();
    void runFileSearch();
    void clearFileSearchIfNeeded(const QString& text);
    void switchMainTab(int index);
    void openOnlineUserDialog();
    void openShareFileDialog();
    void uploadFile();
    void downloadSelectedFile();
    void enqueueUploads(const QStringList& local_paths);
    void startNextQueuedUpload();
    void showTransferRow(const QString& title, int percent, bool cancellable);
    void hideTransferRow();
    void loadProfileDraft();
    void saveProfileDraft();
    QString selectedFriend() const;
    bool selectedFriendOnline() const;
    QString selectedFilePath() const;
    bool selectedFileIsDir() const;

    QString current_username_;
    cloudvault::ChatService&   chat_service_;
    cloudvault::FriendService& friend_service_;
    cloudvault::FileService&   file_service_;
    cloudvault::ShareService&  share_service_;
    QList<QPair<QString, bool>> friends_;
    QHash<QString, ConversationSummary> conversation_summaries_;
    QSet<QString> pending_history_requests_;
    cloudvault::FileEntries current_file_entries_;
    QString active_chat_peer_;
    QString current_file_path_ = QStringLiteral("/");
    QString current_file_query_;
    bool file_search_mode_ = false;

    QWidget* content_root_ = nullptr;
    QSplitter* content_splitter_ = nullptr;
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
    QStackedWidget* chat_stack_ = nullptr;

    QLabel* chat_avatar_label_ = nullptr;
    QLabel* chat_title_label_ = nullptr;
    QLabel* chat_status_label_ = nullptr;
    QListWidget* message_list_ = nullptr;
    QLabel* file_path_label_ = nullptr;
    QLabel* file_status_label_ = nullptr;
    QLineEdit* file_search_edit_ = nullptr;
    QListWidget* file_list_ = nullptr;
    QLabel* file_empty_state_label_ = nullptr;
    QPushButton* file_back_btn_ = nullptr;
    QPushButton* file_upload_btn_ = nullptr;
    QPushButton* file_refresh_btn_ = nullptr;
    QPushButton* file_create_btn_ = nullptr;
    QFrame* file_transfer_row_ = nullptr;
    QLabel* file_transfer_label_ = nullptr;
    QLabel* file_transfer_percent_label_ = nullptr;
    QProgressBar* file_transfer_bar_ = nullptr;
    QPushButton* file_transfer_cancel_btn_ = nullptr;
    QLabel* profile_name_label_ = nullptr;
    QLabel* profile_id_label_ = nullptr;
    QLineEdit* profile_nickname_edit_ = nullptr;
    QLineEdit* profile_signature_edit_ = nullptr;
    QTextEdit* message_input_ = nullptr;
    QPushButton* send_btn_ = nullptr;
    QPushButton* logout_btn_ = nullptr;
    QPushButton* file_share_btn_ = nullptr;
    QPushButton* file_download_btn_ = nullptr;
    QPushButton* file_rename_btn_ = nullptr;
    QPushButton* file_move_btn_ = nullptr;
    QPushButton* file_delete_btn_ = nullptr;

    QLabel* detail_contact_avatar_label_ = nullptr;
    QLabel* detail_contact_name_label_ = nullptr;
    QLabel* detail_contact_status_label_ = nullptr;
    QVBoxLayout* detail_shared_files_layout_ = nullptr;
    QLabel* detail_shared_empty_label_ = nullptr;
    QLabel* detail_file_target_label_ = nullptr;
    QLabel* detail_file_meta_label_ = nullptr;
    QStringList pending_upload_paths_;
};
