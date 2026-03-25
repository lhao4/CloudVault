// =============================================================
// client/src/ui/online_user_dialog.h
// 在线用户弹窗
// =============================================================

#pragma once

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>

class QListWidget;
class QPushButton;
class QLineEdit;

class OnlineUserDialog : public QDialog {
    Q_OBJECT

public:
    explicit OnlineUserDialog(const QList<QPair<QString, bool>>& users,
                              QWidget* parent = nullptr);

signals:
    void refreshRequested();
    void userChosen(const QString& username);

private:
    void setupUi();
    void connectSignals();
    void populateList(const QString& keyword = QString());

    QList<QPair<QString, bool>> users_;
    QLineEdit* search_edit_ = nullptr;
    QListWidget* user_list_ = nullptr;
    QPushButton* refresh_btn_ = nullptr;
    QPushButton* add_btn_ = nullptr;
};
