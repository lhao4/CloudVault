// =============================================================
// client/src/ui/share_file_dialog.h
// 文件分享弹窗
// =============================================================

#pragma once

#include "service/friend_service.h"

#include <QDialog>
#include <QList>
#include <QString>

class QListWidget;
class QLabel;
class QLineEdit;
class QPushButton;

class ShareFileDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShareFileDialog(const QString& file_name,
                             const QList<cloudvault::FriendProfile>& friends,
                             QWidget* parent = nullptr);

signals:
    void shareConfirmed(const QString& file_name, const QStringList& targets);

private:
    void setupUi();
    void connectSignals();
    void populateList(const QString& keyword = QString());
    void updateConfirmButton();
    QStringList selectedTargets() const;

    QString file_name_;
    QList<cloudvault::FriendProfile> friends_;
    QLabel* file_hint_label_ = nullptr;
    QLabel* selection_hint_label_ = nullptr;
    QLineEdit* search_edit_ = nullptr;
    QListWidget* friend_list_ = nullptr;
    QLabel* empty_state_label_ = nullptr;
    QPushButton* select_all_btn_ = nullptr;
    QPushButton* clear_btn_ = nullptr;
    QPushButton* confirm_btn_ = nullptr;
};
