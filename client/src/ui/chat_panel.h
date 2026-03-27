// =============================================================
// client/src/ui/chat_panel.h
// 聊天中栏面板
// =============================================================

#pragma once

#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;
class QTextEdit;
class QStackedWidget;

class ChatPanel : public QWidget {
    Q_OBJECT

public:
    explicit ChatPanel(const QString& current_username, QWidget* parent = nullptr);

    QStackedWidget* stack() const;
    QLabel* avatarLabel() const;
    QLabel* titleLabel() const;
    QLabel* statusLabel() const;
    QLabel* groupTitleLabel() const;
    QLabel* groupStatusLabel() const;
    QListWidget* messageList() const;
    QTextEdit* messageInput() const;
    QPushButton* sendButton() const;
    QPushButton* groupListButton() const;

signals:
    void sendRequested();
    void groupListRequested();

private:
    QStackedWidget* stack_ = nullptr;
    QLabel* avatar_label_ = nullptr;
    QLabel* title_label_ = nullptr;
    QLabel* status_label_ = nullptr;
    QLabel* group_title_label_ = nullptr;
    QLabel* group_status_label_ = nullptr;
    QListWidget* message_list_ = nullptr;
    QTextEdit* message_input_ = nullptr;
    QPushButton* send_button_ = nullptr;
    QPushButton* group_list_button_ = nullptr;
};
