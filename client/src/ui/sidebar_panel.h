// =============================================================
// client/src/ui/sidebar_panel.h
// 左侧联系人面板
// =============================================================

#pragma once

#include <QFrame>
#include <QList>

class QLineEdit;
class QListWidget;
class QPushButton;

struct SidebarContactEntry {
    QString username;
    QString preview;
    QString time;
    int unread = 0;
    bool online = false;
};

class SidebarPanel : public QFrame {
    Q_OBJECT

public:
    explicit SidebarPanel(QWidget* parent = nullptr);

    void setHeader(const QString& title,
                   const QString& placeholder,
                   bool show_action);
    void populateContacts(const QList<SidebarContactEntry>& contacts,
                          const QString& selected_username,
                          bool auto_select_first);
    void refreshSelectionHighlights();
    QString searchText() const;
    void clearSearch();
    void focusSearchSelectAll();
    int contactCount() const;
    QString currentUsername() const;
    bool currentOnline() const;

signals:
    void searchTextChanged(const QString& text);
    void contactSelectionChanged();
    void actionRequested();

private:
    class QLabel* title_label_ = nullptr;
    QLineEdit* search_edit_ = nullptr;
    QListWidget* contact_list_ = nullptr;
    QPushButton* action_btn_ = nullptr;
};
