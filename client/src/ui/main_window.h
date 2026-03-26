// =============================================================
// client/src/ui/main_window.h
// 最小主窗口
// =============================================================

#pragma once

#include "network/friend_service.h"
#include "network/file_service.h"

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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(const QString& username,
               cloudvault::FriendService& friend_service,
               cloudvault::FileService& file_service,
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
    void applySelectedFriend();
    void refreshFileList(const QString& path, const cloudvault::FileEntries& entries);
    void updateFileSelectionState();
    void applySelectedFile();
    void setFileStatus(const QString& message, bool error = false);
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
    QString selectedFriend() const;
    bool selectedFriendOnline() const;
    QString selectedFilePath() const;
    bool selectedFileIsDir() const;
    bool fileListShowingSearchResults() const;

    QString current_username_;
    cloudvault::FriendService& friend_service_;
    cloudvault::FileService&   file_service_;
    QList<QPair<QString, bool>> friends_;
    cloudvault::FileEntries current_file_entries_;
    QString current_file_path_ = QStringLiteral("/");
    QString current_file_query_;
    bool file_search_mode_ = false;

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
    QLabel* file_status_label_ = nullptr;
    QLineEdit* file_search_edit_ = nullptr;
    QListWidget* file_list_ = nullptr;
    QLabel* file_empty_state_label_ = nullptr;
    QPushButton* file_back_btn_ = nullptr;
    QPushButton* file_refresh_btn_ = nullptr;
    QPushButton* file_create_btn_ = nullptr;
    QLabel* profile_name_label_ = nullptr;
    QLabel* profile_id_label_ = nullptr;
    QTextEdit* message_input_ = nullptr;
    QPushButton* logout_btn_ = nullptr;
    QPushButton* file_share_btn_ = nullptr;
    QPushButton* file_rename_btn_ = nullptr;
    QPushButton* file_move_btn_ = nullptr;
    QPushButton* file_delete_btn_ = nullptr;

    QLabel* detail_contact_name_label_ = nullptr;
    QLabel* detail_contact_status_label_ = nullptr;
    QLabel* detail_file_target_label_ = nullptr;
    QLabel* detail_file_meta_label_ = nullptr;
    QLabel* detail_profile_name_label_ = nullptr;
    QLabel* detail_profile_status_label_ = nullptr;
};
