// =============================================================
// client/src/ui/file_panel.h
// 文件中栏面板
// =============================================================

#pragma once

#include "service/file_service.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
class QFrame;

class FilePanel : public QWidget {
    Q_OBJECT

public:
    explicit FilePanel(QWidget* parent = nullptr);

    void setStatusMessage(const QString& message, bool error = false);
    void resetSelectionSummary();
    void setSelectionSummary(const QString& name,
                             const QString& meta,
                             const QString& tooltip = QString());
    void setTransferState(const QString& title, int percent, bool cancellable);
    void clearTransferState();
    void populateEntries(const cloudvault::FileEntries& entries);
    void setPathState(const QString& text, const QString& tooltip, bool can_go_back);
    void setEmptyState(const QString& text, bool empty);
    void selectFirstEntry();
    void refreshSelectionHighlights();
    QString searchText() const;
    void clearSearch();
    void focusSearchSelectAll();
    int itemCount() const;
    bool hasSelection() const;
    QString currentPath() const;
    QString currentName() const;
    bool currentIsDir() const;
    void setActionButtonsEnabled(bool has_selection, bool is_file);
    void setTransferCancelEnabled(bool enabled);

    QListWidget* fileList() const;

signals:
    void backRequested();
    void uploadRequested();
    void refreshRequested();
    void createRequested();
    void searchRequested();
    void searchTextChanged(const QString& text);
    void selectionChanged();
    void itemActivated();
    void renameRequested();
    void moveRequested();
    void deleteRequested();
    void downloadRequested();
    void shareRequested();
    void transferCancelRequested();

private:
    QLabel* path_label_ = nullptr;
    QLabel* status_label_ = nullptr;
    QLineEdit* search_edit_ = nullptr;
    QListWidget* file_list_ = nullptr;
    QLabel* empty_state_label_ = nullptr;
    QPushButton* back_btn_ = nullptr;
    QPushButton* upload_btn_ = nullptr;
    QPushButton* refresh_btn_ = nullptr;
    QPushButton* create_btn_ = nullptr;
    QFrame* transfer_row_ = nullptr;
    QLabel* transfer_label_ = nullptr;
    QLabel* transfer_percent_label_ = nullptr;
    QProgressBar* transfer_bar_ = nullptr;
    QPushButton* transfer_cancel_btn_ = nullptr;
    QLabel* selection_label_ = nullptr;
    QLabel* meta_label_ = nullptr;
    QPushButton* download_btn_ = nullptr;
    QPushButton* share_btn_ = nullptr;
    QPushButton* rename_btn_ = nullptr;
    QPushButton* move_btn_ = nullptr;
    QPushButton* delete_btn_ = nullptr;
};
