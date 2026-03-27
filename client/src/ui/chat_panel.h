// =============================================================
// client/src/ui/chat_panel.h
// 聊天中栏面板
// =============================================================

#pragma once

#include "service/chat_service.h"

#include <QList>
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

    void showEmptyState();
    void showConversationHeader(const QString& username, const QString& presence);
    void setConversationStatus(const QString& status);
    void showGroupPlaceholder(const QString& group_name);
    void appendMessage(const cloudvault::ChatMessage& message, const QString& current_username);
    void rebuildMessages(const QList<cloudvault::ChatMessage>& messages,
                         const QString& current_username);
    QString inputText() const;
    void clearInput();

    QStackedWidget* stack() const;
    QLabel* avatarLabel() const;
    QLabel* titleLabel() const;
    QLabel* statusLabel() const;
    QLabel* groupTitleLabel() const;
    QLabel* groupStatusLabel() const;
    QListWidget* messageList() const;

signals:
    void sendRequested();
    void groupListRequested();

private:
    void appendDateDividerIfNeeded(const QString& timestamp);

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
