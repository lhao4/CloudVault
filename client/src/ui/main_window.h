// =============================================================
// client/src/ui/main_window.h
// 最小主窗口
// =============================================================

#pragma once

#include "service/chat_service.h"
#include "service/friend_service.h"
#include "service/file_service.h"
#include "service/share_service.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QFrame;
class QSplitter;
class QVBoxLayout;
class QStackedWidget;
class ChatPanel;
class FilePanel;
class DetailPanel;
class SidebarPanel;
class ProfilePanel;

/**
 * @brief 客户端主窗口。
 *
 * 负责组织 QQ 风格四栏壳层布局（图标栏 + 联系人栏 + 中央内容 + 详情栏），
 * 协调聊天、好友、文件、分享、个人信息等业务服务，并维护页面级状态。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief 构造主窗口。
     * @param username 当前登录用户名。
     * @param chat_service 聊天服务引用。
     * @param friend_service 好友服务引用。
     * @param file_service 文件服务引用。
     * @param share_service 分享服务引用。
     * @param parent Qt 父对象。
     */
    MainWindow(const QString& username,
               cloudvault::ChatService& chat_service,
               cloudvault::FriendService& friend_service,
               cloudvault::FileService& file_service,
               cloudvault::ShareService& share_service,
               QWidget* parent = nullptr);

    /**
     * @brief 析构主窗口。
     */
    ~MainWindow() override;

    /**
     * @brief 追加事件日志（当前已保留为空实现用于兼容旧调用点）。
     * @param message 日志内容。
     * @param icon 日志图标文本。
     */
    void appendEventLog(const QString& message, const QString& icon = QStringLiteral("●"));

    /**
     * @brief 显示顶部连接状态横幅。
     * @param message 横幅文本。
     */
    void showConnectionBanner(const QString& message);

    /**
     * @brief 隐藏顶部连接状态横幅。
     */
    void hideConnectionBanner();

signals:
    /// @brief 窗口关闭时发出，供 App 层执行退出流程。
    void windowClosed();
    /// @brief 用户请求退出登录时发出。
    void logoutRequested();

protected:
    /**
     * @brief 处理窗口关闭事件并转发 windowClosed 信号。
     * @param event Qt 关闭事件。
     */
    void closeEvent(QCloseEvent* event) override;

private:
    /**
     * @brief 会话摘要信息，用于联系人列表展示预览/时间/未读。
     */
    struct ConversationSummary {
        /// @brief 会话预览文本。
        QString preview;
        /// @brief 最新消息时间戳（原始字符串）。
        QString timestamp;
        /// @brief 未读消息数量。
        int unread_count = 0;
        /// @brief 是否已有历史消息。
        bool has_message = false;
        /// @brief 最新消息是否由当前用户发送。
        bool last_message_outgoing = false;
    };

    /**
     * @brief 构建主窗口 UI 树。
     */
    void setupUi();
    /**
     * @brief 连接主窗口内外部信号槽。
     */
    void connectSignals();
    /**
     * @brief 切换聊天区域到空状态。
     */
    void showChatEmptyState();
    /**
     * @brief 切换聊天区域到联系人会话状态。
     * @param username 联系人用户名。
     * @param online 联系人在线状态。
     */
    void showChatConversation(const QString& username, bool online);
    /**
     * @brief 请求指定联系人的历史消息快照。
     * @param peer 联系人用户名。
     */
    void requestConversationSnapshot(const QString& peer);
    /**
     * @brief 根据历史消息刷新某个联系人的摘要信息。
     * @param peer 联系人用户名。
     * @param messages 历史消息列表。
     */
    void updateConversationSummary(const QString& peer,
                                   const QList<cloudvault::ChatMessage>& messages);
    /**
     * @brief 应用单条消息到会话摘要。
     * @param message 消息对象。
     * @param increment_unread 是否累加未读数。
     */
    void applyConversationMessage(const cloudvault::ChatMessage& message, bool increment_unread);
    /**
     * @brief 清空指定联系人的未读计数。
     * @param peer 联系人用户名。
     */
    void clearConversationUnread(const QString& peer);
    /**
     * @brief 计算某联系人未读消息数。
     * @param peer 联系人用户名。
     * @param messages 历史消息列表。
     * @return 未读数量。
     */
    int calculateUnreadCount(const QString& peer,
                             const QList<cloudvault::ChatMessage>& messages) const;
    /**
     * @brief 读取某联系人已读时间。
     * @param peer 联系人用户名。
     * @return 最近已读时间。
     */
    QDateTime loadConversationReadAt(const QString& peer) const;
    /**
     * @brief 持久化某联系人已读时间。
     * @param peer 联系人用户名。
     * @param timestamp 已读时间。
     */
    void saveConversationReadAt(const QString& peer, const QDateTime& timestamp) const;
    /**
     * @brief 刷新好友缓存并触发侧栏重绘。
     * @param friends 好友列表（用户名 + 在线状态）。
     */
    void refreshFriendList(const QList<QPair<QString, bool>>& friends);
    /**
     * @brief 按关键词过滤并重建联系人列表。
     * @param keyword 搜索关键词。
     */
    void filterFriendList(const QString& keyword);
    /**
     * @brief 刷新联系人列表选中高亮。
     */
    void updateContactSelectionState();
    /**
     * @brief 应用当前选中的联系人并加载聊天内容。
     */
    void applySelectedFriend();
    /**
     * @brief 发送当前输入框消息。
     */
    void sendCurrentMessage();
    /**
     * @brief 刷新文件列表 UI 与状态。
     * @param path 当前目录路径或搜索上下文路径。
     * @param entries 文件条目列表。
     */
    void refreshFileList(const QString& path, const cloudvault::FileEntries& entries);
    /**
     * @brief 刷新文件选中态对应按钮状态。
     */
    void updateFileSelectionState();
    /**
     * @brief 应用当前选中文件摘要信息。
     */
    void applySelectedFile();
    /**
     * @brief 设置文件页状态文案。
     * @param message 状态文本。
     * @param error 是否错误态。
     */
    void setFileStatus(const QString& message, bool error = false);
    /**
     * @brief 重置文件页底部摘要。
     */
    void resetFileActionSummary();
    /**
     * @brief 打开指定路径并触发列表刷新。
     * @param path 目标路径。
     */
    void navigateToFilePath(const QString& path);
    /**
     * @brief 打开当前目录的父目录。
     */
    void openCurrentParentDirectory();
    /**
     * @brief 创建目录动作。
     */
    void createDirectory();
    /**
     * @brief 重命名当前选中项。
     */
    void renameSelectedFile();
    /**
     * @brief 移动当前选中项。
     */
    void moveSelectedFile();
    /**
     * @brief 删除当前选中项。
     */
    void deleteSelectedFile();
    /**
     * @brief 执行文件搜索。
     */
    void runFileSearch();
    /**
     * @brief 在搜索框清空时自动退出搜索模式。
     * @param text 搜索框当前文本。
     */
    void clearFileSearchIfNeeded(const QString& text);
    /**
     * @brief 切换主内容页签。
     * @param index 页面索引（0=消息，1=文件，2=我）。
     */
    void switchMainTab(int index);
    /**
     * @brief 打开在线用户弹窗。
     */
    void openOnlineUserDialog();
    /**
     * @brief 打开群组列表弹窗。
     */
    void openGroupListDialog();
    /**
     * @brief 打开文件分享弹窗。
     */
    void openShareFileDialog();
    /**
     * @brief 选择并入队上传文件。
     */
    void uploadFile();
    /**
     * @brief 下载当前选中文件。
     */
    void downloadSelectedFile();
    /**
     * @brief 将本地路径批量加入上传队列。
     * @param local_paths 本地文件路径列表。
     */
    void enqueueUploads(const QStringList& local_paths);
    /**
     * @brief 从队列启动下一个上传任务。
     */
    void startNextQueuedUpload();
    /**
     * @brief 展示传输进度行。
     * @param title 传输标题。
     * @param percent 百分比。
     * @param cancellable 是否可取消。
     */
    void showTransferRow(const QString& title, int percent, bool cancellable);
    /**
     * @brief 隐藏传输进度行。
     */
    void hideTransferRow();
    /**
     * @brief 从本地配置加载个人资料草稿。
     */
    void loadProfileDraft();
    /**
     * @brief 保存个人资料草稿到本地配置。
     */
    void saveProfileDraft();
    /**
     * @brief 刷新左侧图标栏激活态样式。
     */
    void refreshIconBarActiveState();

    /// @brief 当前登录用户名。
    QString current_username_;
    /// @brief 聊天服务引用。
    cloudvault::ChatService&   chat_service_;
    /// @brief 好友服务引用。
    cloudvault::FriendService& friend_service_;
    /// @brief 文件服务引用。
    cloudvault::FileService&   file_service_;
    /// @brief 分享服务引用。
    cloudvault::ShareService&  share_service_;
    /// @brief 当前好友缓存。
    QList<QPair<QString, bool>> friends_;
    /// @brief 各联系人会话摘要缓存。
    QHash<QString, ConversationSummary> conversation_summaries_;
    /// @brief 正在请求历史消息的联系人集合（防重入）。
    QSet<QString> pending_history_requests_;
    /// @brief 当前文件列表缓存。
    cloudvault::FileEntries current_file_entries_;
    /// @brief 当前激活聊天联系人。
    QString active_chat_peer_;
    /// @brief 当前文件目录路径。
    QString current_file_path_ = QStringLiteral("/");
    /// @brief 当前搜索关键词。
    QString current_file_query_;
    /// @brief 是否处于文件搜索模式。
    bool file_search_mode_ = false;
    /// @brief 当前激活群组名（仅 UI 占位流程）。
    QString active_group_name_;

    /// @brief 主内容根容器。
    QWidget* content_root_ = nullptr;
    /// @brief 三栏分割器（联系人 / 中央内容 / 详情）。
    QSplitter* content_splitter_ = nullptr;
    /// @brief 左侧联系人栏容器。
    QFrame* sidebar_panel_ = nullptr;
    /// @brief 右侧详情栏容器。
    QWidget* detail_panel_ = nullptr;
    /// @brief 顶部连接状态横幅。
    QFrame* connection_banner_ = nullptr;
    /// @brief 连接状态横幅文本。
    QLabel* connection_banner_label_ = nullptr;
    /// @brief 左侧 52px 图标导航栏。
    QWidget* icon_bar_ = nullptr;
    /// @brief 头像按钮（进入“我”页）。
    QPushButton* avatar_tab_btn_ = nullptr;
    /// @brief 消息页按钮。
    QPushButton* message_tab_btn_ = nullptr;
    /// @brief 联系人按钮（打开在线用户弹窗）。
    QPushButton* contact_tab_btn_ = nullptr;
    /// @brief 文件页按钮。
    QPushButton* file_tab_btn_ = nullptr;
    /// @brief 设置按钮（占位）。
    QPushButton* settings_tab_btn_ = nullptr;
    /// @brief 当前主标签索引。
    int active_main_tab_ = 0;

    /// @brief 中央页面堆栈（聊天 / 文件 / 我）。
    QStackedWidget* center_stack_ = nullptr;
    /// @brief 左侧联系人面板。
    SidebarPanel* sidebar_widget_ = nullptr;
    /// @brief 聊天面板。
    ChatPanel* chat_panel_widget_ = nullptr;
    /// @brief 文件面板。
    FilePanel* file_panel_widget_ = nullptr;
    /// @brief 右侧详情面板。
    DetailPanel* detail_panel_widget_ = nullptr;
    /// @brief 个人资料面板。
    ProfilePanel* profile_panel_widget_ = nullptr;
    /// @brief 等待上传的本地文件队列。
    QStringList pending_upload_paths_;
};
