// =============================================================
// client/src/ui/file_panel.h
// 文件中栏面板
// =============================================================

#pragma once

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

    QLabel* pathLabel() const;
    QLabel* statusLabel() const;
    QLineEdit* searchEdit() const;
    QListWidget* fileList() const;
    QLabel* emptyStateLabel() const;
    QPushButton* backButton() const;
    QPushButton* uploadButton() const;
    QPushButton* refreshButton() const;
    QPushButton* createButton() const;
    QFrame* transferRow() const;
    QLabel* transferLabel() const;
    QLabel* transferPercentLabel() const;
    QProgressBar* transferBar() const;
    QPushButton* transferCancelButton() const;
    QLabel* selectionLabel() const;
    QLabel* metaLabel() const;
    QPushButton* downloadButton() const;
    QPushButton* shareButton() const;
    QPushButton* renameButton() const;
    QPushButton* moveButton() const;
    QPushButton* deleteButton() const;

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
