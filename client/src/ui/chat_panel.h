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

/**
 * @brief 聊天中栏面板。
 *
 * 承载聊天空态页和统一会话页（私聊与群聊共用同一结构），
 * 负责消息列表渲染入口与输入区交互信号。
 */
class ChatPanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造聊天面板。
     * @param current_username 当前用户，用于初始化头像。
     * @param parent Qt 父对象。
     */
    explicit ChatPanel(const QString& current_username, QWidget* parent = nullptr);

    /**
     * @brief 切换到空状态页。
     */
    void showEmptyState();

    /**
     * @brief 展示会话头部信息并切换到会话页（私聊和群聊统一入口）。
     * @param title 会话标题（联系人用户名或群名称）。
     * @param subtitle 副标题（在线状态或群人数摘要）。
     * @param is_group 是否为群聊会话。
     */
    void showConversation(const QString& title, const QString& subtitle, bool is_group = false);

    /**
     * @brief 设置会话副标题文本（例如"正在加载历史消息"）。
     * @param status 状态文本。
     */
    void setConversationStatus(const QString& status);

    /**
     * @brief 追加一条消息到消息列表。
     * @param message 消息对象。
     * @param current_username 当前用户名，用于判断左右气泡。
     */
    void appendMessage(const cloudvault::ChatMessage& message, const QString& current_username);

    /**
     * @brief 用完整历史重建消息列表。
     * @param messages 历史消息列表。
     * @param current_username 当前用户名。
     */
    void rebuildMessages(const QList<cloudvault::ChatMessage>& messages,
                         const QString& current_username);

    /**
     * @brief 获取输入框文本。
     * @return 文本内容。
     */
    QString inputText() const;

    /**
     * @brief 清空输入框。
     */
    void clearInput();

    /**
     * @brief 获取内部页面堆栈。
     * @return 页面堆栈指针。
     */
    QStackedWidget* stack() const;

    /**
     * @brief 获取会话头部头像标签。
     * @return 头像标签指针。
     */
    QLabel* avatarLabel() const;

    /**
     * @brief 获取会话标题标签。
     * @return 标题标签指针。
     */
    QLabel* titleLabel() const;

    /**
     * @brief 获取会话状态标签。
     * @return 状态标签指针。
     */
    QLabel* statusLabel() const;

    /**
     * @brief 获取消息列表控件。
     * @return 列表控件指针。
     */
    QListWidget* messageList() const;

signals:
    /// @brief 用户触发发送消息时发出。
    void sendRequested();
    /// @brief 用户请求打开群组列表时发出。
    void groupListRequested();

private:
    /**
     * @brief 在日期变化时插入日期分割线。
     * @param timestamp 当前消息时间戳。
     */
    void appendDateDividerIfNeeded(const QString& timestamp);

    /// @brief 页面堆栈（空态 / 会话）。
    QStackedWidget* stack_ = nullptr;
    /// @brief 会话头像。
    QLabel* avatar_label_ = nullptr;
    /// @brief 会话标题。
    QLabel* title_label_ = nullptr;
    /// @brief 会话状态。
    QLabel* status_label_ = nullptr;
    /// @brief 消息列表。
    QListWidget* message_list_ = nullptr;
    /// @brief 输入框。
    QTextEdit* message_input_ = nullptr;
    /// @brief 发送按钮。
    QPushButton* send_button_ = nullptr;
    /// @brief 群成员按钮（群聊模式下显示）。
    QPushButton* members_btn_ = nullptr;
    /// @brief 当前是否为群聊模式。
    bool is_group_ = false;
};
