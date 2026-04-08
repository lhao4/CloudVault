// =============================================================
// client/src/ui/group_list_dialog.h
// 群组列表弹窗
// =============================================================

#pragma once

#include <QDialog>
#include <QList>
#include <QString>

struct GroupListEntry {
    int group_id = 0;
    QString name;
    int owner_id = 0;
    int online_count = 0;
    int unread_count = 0;
};

class QListWidget;
class QPushButton;
class QLabel;

class GroupListDialog : public QDialog {
    Q_OBJECT

public:
    explicit GroupListDialog(const QList<GroupListEntry>& groups,
                             QWidget* parent = nullptr);
    void setGroups(const QList<GroupListEntry>& groups);

signals:
    void groupChosen(int group_id);
    void createRequested(const QString& group_name);
    void leaveRequested(int group_id);

private:
    void setupUi();
    void connectSignals();
    void populateList();

    QList<GroupListEntry> groups_;
    QListWidget* group_list_ = nullptr;
    QLabel* empty_state_label_ = nullptr;
    QPushButton* enter_btn_ = nullptr;
    QPushButton* leave_btn_ = nullptr;
    QPushButton* create_btn_ = nullptr;
};

Q_DECLARE_METATYPE(GroupListEntry)
