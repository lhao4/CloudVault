// =============================================================
// client/src/ui/main_window.h
// 最小主窗口
// =============================================================

#pragma once

#include "service/chat_service.h"
#include "service/friend_service.h"
#include "service/file_service.h"
#include "service/share_service.h"

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
class QPushButton;
class QFrame;
class QSplitter;
class QVBoxLayout;
class QStackedWidget;
class ChatPanel;
class FilePanel;
class DetailPanel;
class SidebarPanel;
class ProfilePanel;

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
    void appendEventLog(const QString& message, const QString& icon = QStringLiteral("●"));
    void showConnectionBanner(const QString& message);
    void hideConnectionBanner();

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
    void openGroupListDialog();
    void openShareFileDialog();
    void uploadFile();
    void downloadSelectedFile();
    void enqueueUploads(const QStringList& local_paths);
    void startNextQueuedUpload();
    void showTransferRow(const QString& title, int percent, bool cancellable);
    void hideTransferRow();
    void loadProfileDraft();
    void saveProfileDraft();
    void refreshIconBarActiveState();

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
    QString active_group_name_;

    QWidget* content_root_ = nullptr;
    QSplitter* content_splitter_ = nullptr;
    QFrame* sidebar_panel_ = nullptr;
    QWidget* detail_panel_ = nullptr;
    QFrame* connection_banner_ = nullptr;
    QLabel* connection_banner_label_ = nullptr;
    QWidget* icon_bar_ = nullptr;
    QPushButton* avatar_tab_btn_ = nullptr;
    QPushButton* message_tab_btn_ = nullptr;
    QPushButton* contact_tab_btn_ = nullptr;
    QPushButton* file_tab_btn_ = nullptr;
    QPushButton* settings_tab_btn_ = nullptr;
    int active_main_tab_ = 0;

    QStackedWidget* center_stack_ = nullptr;
    SidebarPanel* sidebar_widget_ = nullptr;
    ChatPanel* chat_panel_widget_ = nullptr;
    FilePanel* file_panel_widget_ = nullptr;
    DetailPanel* detail_panel_widget_ = nullptr;
    ProfilePanel* profile_panel_widget_ = nullptr;
    QStringList pending_upload_paths_;
};
