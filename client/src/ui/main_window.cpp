// =============================================================
// client/src/ui/main_window.cpp
// 主窗口（消息 / 联系人 / 文件 / 我）
// =============================================================

#include "main_window.h"

#include "ui/chat_panel.h"
#include "ui/contact_panel.h"
#include "ui/file_panel.h"
#include "ui/group_list_dialog.h"
#include "ui/online_user_dialog.h"
#include "ui/profile_panel.h"
#include "ui/sidebar_panel.h"
#include "ui/share_file_dialog.h"
#include "ui/widget_helpers.h"

#include <algorithm>

#include <QAbstractItemView>
#include <QApplication>
#include <QCloseEvent>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

using namespace cv_ui;

namespace {

QString formatPresence(bool online) {
    return online ? QStringLiteral("在线") : QStringLiteral("离线");
}

const cloudvault::FriendProfile* findFriendProfile(
    const QList<cloudvault::FriendProfile>& friends,
    const QString& username) {
    const auto it = std::find_if(friends.cbegin(),
                                 friends.cend(),
                                 [&username](const cloudvault::FriendProfile& item) {
                                     return item.username == username;
                                 });
    return it != friends.cend() ? &(*it) : nullptr;
}

bool lookupFriendOnline(const QList<cloudvault::FriendProfile>& friends, const QString& username) {
    const cloudvault::FriendProfile* profile = findFriendProfile(friends, username);
    return profile && profile->online;
}

QString friendDisplayName(const cloudvault::FriendProfile* profile, const QString& fallback) {
    if (!profile) {
        return fallback;
    }
    const QString nickname = profile->nickname.trimmed();
    return nickname.isEmpty() ? profile->username : nickname;
}

QString formatConversationPreview(const QString& content, bool outgoing) {
    Q_UNUSED(outgoing);
    QString simplified = content.simplified();
    if (simplified.isEmpty()) {
        simplified = QStringLiteral("[空消息]");
    }

    constexpr int kMaxPreviewChars = 24;
    if (simplified.size() > kMaxPreviewChars) {
        simplified = simplified.left(kMaxPreviewChars) + QStringLiteral("...");
    }
    return simplified;
}

QDateTime parseConversationTimestamp(const QString& timestamp) {
    QString normalized = timestamp.trimmed();
    const int dot_index = normalized.indexOf('.');
    if (dot_index >= 0) {
        int fraction_end = dot_index + 1;
        while (fraction_end < normalized.size() && normalized.at(fraction_end).isDigit()) {
            ++fraction_end;
        }
        QString fraction = normalized.mid(dot_index + 1, fraction_end - dot_index - 1);
        fraction = fraction.left(3).leftJustified(3, QLatin1Char('0'));
        normalized = normalized.left(dot_index) + QStringLiteral(".") + fraction;
    }

    QDateTime dt = QDateTime::fromString(normalized, QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    if (!dt.isValid()) {
        dt = QDateTime::fromString(normalized.section('.', 0, 0),
                                   QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }
    return dt;
}

QString formatConversationTime(const QString& timestamp) {
    const QDateTime dt = parseConversationTimestamp(timestamp);
    if (!dt.isValid()) {
        return {};
    }

    const QDate today = QDate::currentDate();
    if (dt.date() == today) {
        return dt.time().toString(QStringLiteral("HH:mm"));
    }
    if (dt.date().year() == today.year()) {
        return dt.date().toString(QStringLiteral("MM-dd"));
    }
    return dt.date().toString(QStringLiteral("yyyy-MM-dd"));
}

} // namespace

MainWindow::MainWindow(const QString& username,
                       cloudvault::AuthService& auth_service,
                       cloudvault::GroupService& group_service,
                       cloudvault::ChatService& chat_service,
                       cloudvault::FriendService& friend_service,
                       cloudvault::FileService& file_service,
                       cloudvault::ShareService& share_service,
                       QWidget* parent)
    : QMainWindow(parent),
      current_username_(username),
      auth_service_(auth_service),
      group_service_(group_service),
      chat_service_(chat_service),
      friend_service_(friend_service),
      file_service_(file_service),
      share_service_(share_service) {
    setupUi();
    connectSignals();
    loadProfileDraft();
    friend_service_.flushFriends();
    group_service_.getGroupList();
    file_service_.listFiles(current_file_path_);
    switchMainTab(0);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    emit windowClosed();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("CloudVault - %1").arg(current_username_));
    setWindowIcon(QApplication::windowIcon());
    resize(1100, 700);
    setMinimumSize(960, 620);

    auto* shell = new QWidget(this);
    shell->setObjectName(QStringLiteral("mainShell"));
    auto* shell_layout = new QHBoxLayout(shell);
    shell_layout->setContentsMargins(0, 0, 0, 0);
    shell_layout->setSpacing(0);

    icon_bar_ = new QWidget(shell);
    icon_bar_->setObjectName(QStringLiteral("iconBar"));
    auto* icon_bar_layout = new QVBoxLayout(icon_bar_);
    icon_bar_layout->setContentsMargins(0, 8, 0, 8);
    icon_bar_layout->setSpacing(0);

    auto* avatar_wrap = new QWidget(icon_bar_);
    avatar_wrap->setFixedHeight(52);
    auto* avatar_layout = new QHBoxLayout(avatar_wrap);
    avatar_layout->setContentsMargins(0, 8, 0, 8);
    avatar_layout->setSpacing(0);
    avatar_tab_btn_ = new QPushButton(current_username_.left(1).toUpper(), avatar_wrap);
    avatar_tab_btn_->setObjectName(QStringLiteral("iconAvatarBtn"));
    avatar_tab_btn_->setProperty("variant", avatarVariantForSeed(current_username_));
    avatar_tab_btn_->setCursor(Qt::PointingHandCursor);
    avatar_tab_btn_->setFixedSize(36, 36);
    avatar_layout->addWidget(avatar_tab_btn_, 0, Qt::AlignHCenter);

    auto* avatar_dot = new QLabel(avatar_tab_btn_);
    avatar_dot->setObjectName(QStringLiteral("iconAvatarDot"));
    avatar_dot->setFixedSize(10, 10);
    avatar_dot->move(24, 24);
    avatar_dot->show();

    icon_bar_layout->addWidget(avatar_wrap);
    icon_bar_layout->addSpacing(16);

    message_tab_btn_ = createIconButton(QStringLiteral("💬"), QStringLiteral("消息"), 36, icon_bar_);
    message_tab_btn_->setObjectName(QStringLiteral("iconNavBtn"));
    message_tab_btn_->setProperty("active", false);
    icon_bar_layout->addWidget(message_tab_btn_, 0, Qt::AlignHCenter);

    contact_tab_btn_ = createIconButton(QStringLiteral("👥"), QStringLiteral("联系人"), 36, icon_bar_);
    contact_tab_btn_->setObjectName(QStringLiteral("iconNavBtn"));
    contact_tab_btn_->setProperty("active", false);
    icon_bar_layout->addWidget(contact_tab_btn_, 0, Qt::AlignHCenter);

    file_tab_btn_ = createIconButton(QStringLiteral("📁"), QStringLiteral("文件"), 36, icon_bar_);
    file_tab_btn_->setObjectName(QStringLiteral("iconNavBtn"));
    file_tab_btn_->setProperty("active", false);
    icon_bar_layout->addWidget(file_tab_btn_, 0, Qt::AlignHCenter);
    icon_bar_layout->addStretch();

    settings_tab_btn_ = createIconButton(QStringLiteral("⚙"), QStringLiteral("设置"), 36, icon_bar_);
    settings_tab_btn_->setObjectName(QStringLiteral("iconNavBtn"));
    settings_tab_btn_->setProperty("active", false);
    icon_bar_layout->addWidget(settings_tab_btn_, 0, Qt::AlignHCenter);

    shell_layout->addWidget(icon_bar_);

    content_root_ = new QWidget(shell);
    auto* content_root_layout = new QVBoxLayout(content_root_);
    content_root_layout->setContentsMargins(0, 0, 0, 0);
    content_root_layout->setSpacing(0);

    connection_banner_ = new QFrame(content_root_);
    connection_banner_->setObjectName(QStringLiteral("connectionBanner"));
    connection_banner_->hide();
    auto* connection_banner_layout = new QHBoxLayout(connection_banner_);
    connection_banner_layout->setContentsMargins(16, 0, 16, 0);
    connection_banner_layout->setSpacing(8);
    connection_banner_label_ = new QLabel(QStringLiteral("已断开连接，正在重连…"), connection_banner_);
    connection_banner_label_->setObjectName(QStringLiteral("connectionBannerLabel"));
    connection_banner_layout->addWidget(connection_banner_label_);
    connection_banner_layout->addStretch();
    content_root_layout->addWidget(connection_banner_);

    content_splitter_ = new QSplitter(Qt::Horizontal, content_root_);
    content_splitter_->setChildrenCollapsible(false);
    content_splitter_->setHandleWidth(1);
    content_root_layout->addWidget(content_splitter_, 1);

    sidebar_widget_ = new SidebarPanel(content_splitter_);
    sidebar_panel_ = sidebar_widget_;

    auto* center_panel = new QWidget(content_splitter_);
    center_panel->setObjectName(QStringLiteral("centerPanel"));
    auto* center_layout = new QVBoxLayout(center_panel);
    center_layout->setContentsMargins(0, 0, 0, 0);
    center_layout->setSpacing(0);

    center_stack_ = new QStackedWidget(center_panel);
    center_layout->addWidget(center_stack_, 1);

    {
        chat_panel_widget_ = new ChatPanel(current_username_, center_stack_);
        center_stack_->addWidget(chat_panel_widget_);
    }

    {
        contact_panel_widget_ = new ContactPanel(current_username_, center_stack_);
        center_stack_->addWidget(contact_panel_widget_);
    }

    {
        file_panel_widget_ = new FilePanel(center_stack_);
        center_stack_->addWidget(file_panel_widget_);
    }

    {
        profile_panel_widget_ = new ProfilePanel(current_username_, center_stack_);
        center_stack_->addWidget(profile_panel_widget_);
    }

    content_splitter_->addWidget(sidebar_widget_);
    content_splitter_->addWidget(center_panel);
    content_splitter_->setStretchFactor(1, 1);
    content_splitter_->setSizes({260, 780});
    shell_layout->addWidget(content_root_, 1);

    setCentralWidget(shell);
    showChatEmptyState();
    showContactEmptyState();
    hideTransferRow();
    resetFileActionSummary();
    refreshIconBarActiveState();
}

void MainWindow::connectSignals() {
    connect(message_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(0); });
    connect(contact_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(1); });
    connect(file_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(2); });
    connect(avatar_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(3); });
    connect(settings_tab_btn_, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, QStringLiteral("设置"), QStringLiteral("设置功能开发中。"));
    });

    connect(chat_panel_widget_, &ChatPanel::sendRequested,
            this, &MainWindow::sendCurrentMessage);
    connect(chat_panel_widget_, &ChatPanel::groupListRequested,
            this, &MainWindow::openGroupListDialog);
    connect(sidebar_widget_, &SidebarPanel::searchTextChanged,
            this, [this](const QString& text) {
                if (active_main_view_ == MainView::Contact) {
                    filterContactList(text);
                } else {
                    filterFriendList(text);
                }
            });
    connect(sidebar_widget_, &SidebarPanel::contactSelectionChanged,
            this, &MainWindow::applySelectedSidebarItem);
    connect(sidebar_widget_, &SidebarPanel::actionRequested,
            this, [this] {
                QMenu menu(this);
                QAction* add_friend = menu.addAction(QStringLiteral("添加好友"));
                QAction* create_group = menu.addAction(QStringLiteral("发起群聊"));
                QAction* open_group_list = nullptr;
                if (active_main_view_ == MainView::Message) {
                    open_group_list = menu.addAction(QStringLiteral("群组列表"));
                }
                QAction* chosen = menu.exec(QCursor::pos());
                if (chosen == add_friend) {
                    openOnlineUserDialog();
                } else if (chosen == create_group) {
                    openGroupListDialog();
                } else if (chosen == open_group_list) {
                    openGroupListDialog();
                }
            });

    connect(contact_panel_widget_, &ContactPanel::sendMessageRequested,
            this, &MainWindow::navigateToChatWithFriend);
    connect(contact_panel_widget_, &ContactPanel::shareFileRequested,
            this, [this](const QString& username) {
                switchMainTab(2);
                setFileStatus(QStringLiteral("请在文件页选择文件后分享给 %1。").arg(username));
            });
    connect(contact_panel_widget_, &ContactPanel::deleteFriendRequested,
            this, [this](const QString& username) {
                const auto reply = QMessageBox::question(
                    this,
                    QStringLiteral("删除好友"),
                    QStringLiteral("确认删除好友“%1”吗？").arg(username),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    friend_service_.deleteFriend(username);
                }
            });
    connect(contact_panel_widget_, &ContactPanel::acceptFriendRequestRequested,
            this, [this](const QString& username) {
                if (!username.isEmpty()) {
                    friend_service_.agreeFriend(username);
                }
            });
    connect(contact_panel_widget_, &ContactPanel::openGroupChatRequested,
            this, &MainWindow::navigateToChatWithGroup);
    connect(contact_panel_widget_, &ContactPanel::viewGroupMembersRequested,
            this, [this](int group_id) {
                QMessageBox::information(this,
                                         QStringLiteral("群成员"),
                                         QStringLiteral("群 %1 的成员详情面板后续补充。").arg(group_id));
            });
    connect(contact_panel_widget_, &ContactPanel::leaveGroupRequested,
            this, [this](int group_id) {
                const auto reply = QMessageBox::question(
                    this,
                    QStringLiteral("退出群组"),
                    QStringLiteral("确认退出该群组吗？"),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    group_service_.leaveGroup(group_id);
                }
            });

    connect(profile_panel_widget_, &ProfilePanel::saveRequested,
            this, &MainWindow::saveProfileDraft);
    connect(profile_panel_widget_, &ProfilePanel::cancelRequested,
            this, &MainWindow::loadProfileDraft);
    connect(profile_panel_widget_, &ProfilePanel::logoutRequested,
            this, &MainWindow::logoutRequested);
    connect(&auth_service_, &cloudvault::AuthService::profileUpdateSuccess,
            this, [this] {
                setFileStatus(QStringLiteral("个人资料已同步到服务端。"));
            });
    connect(&auth_service_, &cloudvault::AuthService::profileUpdateFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("资料更新失败"), reason);
            });

    connect(file_panel_widget_, &FilePanel::backRequested, this, &MainWindow::openCurrentParentDirectory);
    connect(file_panel_widget_, &FilePanel::uploadRequested, this, &MainWindow::uploadFile);
    connect(file_panel_widget_, &FilePanel::refreshRequested,
            this, [this] {
                if (file_search_mode_ && !current_file_query_.isEmpty()) {
                    setFileStatus(QStringLiteral("正在刷新搜索结果…"));
                    file_service_.search(current_file_query_);
                } else {
                    setFileStatus(QStringLiteral("正在刷新当前目录…"));
                    file_service_.listFiles(current_file_path_);
                }
            });
    connect(file_panel_widget_, &FilePanel::createRequested, this, &MainWindow::createDirectory);
    connect(file_panel_widget_, &FilePanel::searchRequested, this, &MainWindow::runFileSearch);
    connect(file_panel_widget_, &FilePanel::searchTextChanged, this, &MainWindow::clearFileSearchIfNeeded);
    connect(file_panel_widget_, &FilePanel::breadcrumbRequested,
            this, &MainWindow::navigateToFilePath);
    connect(file_panel_widget_, &FilePanel::selectionChanged, this, &MainWindow::applySelectedFile);
    connect(file_panel_widget_, &FilePanel::itemActivated,
            this, [this] {
                if (!(file_panel_widget_ ? file_panel_widget_->currentIsDir() : false)) {
                    return;
                }
                file_search_mode_ = false;
                current_file_query_.clear();
                if (file_panel_widget_) {
                    file_panel_widget_->clearSearch();
                }
                navigateToFilePath(file_panel_widget_ ? file_panel_widget_->currentPath() : QString());
            });
    connect(file_panel_widget_, &FilePanel::renameRequested, this, &MainWindow::renameSelectedFile);
    connect(file_panel_widget_, &FilePanel::moveRequested, this, &MainWindow::moveSelectedFile);
    connect(file_panel_widget_, &FilePanel::deleteRequested, this, &MainWindow::deleteSelectedFile);
    connect(file_panel_widget_, &FilePanel::downloadRequested, this, &MainWindow::downloadSelectedFile);
    connect(file_panel_widget_, &FilePanel::shareRequested, this, &MainWindow::openShareFileDialog);
    connect(file_panel_widget_, &FilePanel::transferCancelRequested,
            this, [this] {
                if (!pending_upload_paths_.isEmpty()) {
                    pending_upload_paths_.clear();
                    setFileStatus(QStringLiteral("已清空等待中的上传队列，当前正在进行的传输不会中断。"));
                    if (file_panel_widget_) {
                        file_panel_widget_->setTransferCancelEnabled(false);
                    }
                    return;
                }
                QMessageBox::information(this,
                                         QStringLiteral("当前不可取消"),
                                         QStringLiteral("当前版本暂不支持中途取消已开始的传输任务。"));
            });

    connect(&chat_service_, &cloudvault::ChatService::messageReceived,
            this, [this](const cloudvault::ChatMessage& message) {
                const QString peer = (message.from == current_username_) ? message.to : message.from;
                applyConversationMessage(
                    message,
                    message.from != current_username_ && peer != active_chat_peer_);
                if (peer == active_chat_peer_) {
                    if (chat_panel_widget_) {
                        chat_panel_widget_->appendMessage(message, current_username_);
                    }
                    showChatConversation(peer, lookupFriendOnline(friends_, peer));
                }
            });
    connect(&chat_service_, &cloudvault::ChatService::historyLoaded,
            this, [this](const QString& peer, const QList<cloudvault::ChatMessage>& messages) {
                pending_history_requests_.remove(peer);
                updateConversationSummary(peer, messages);
                if (peer != active_chat_peer_) {
                    return;
                }
                if (chat_panel_widget_) {
                    chat_panel_widget_->rebuildMessages(messages, current_username_);
                }

                QDateTime latest_incoming;
                for (const auto& message : messages) {
                    if (message.from != peer) {
                        continue;
                    }
                    const QDateTime message_time = parseConversationTimestamp(message.timestamp);
                    if (message_time.isValid() &&
                        (!latest_incoming.isValid() || message_time > latest_incoming)) {
                        latest_incoming = message_time;
                    }
                }
                saveConversationReadAt(peer, latest_incoming);
                showChatConversation(peer, lookupFriendOnline(friends_, peer));
            });
    connect(&chat_service_, &cloudvault::ChatService::historyLoadFailed,
            this, [this](const QString& peer, const QString& reason) {
                pending_history_requests_.remove(peer);
                if (peer != active_chat_peer_) {
                    return;
                }
                showChatConversation(peer, lookupFriendOnline(friends_, peer));
                if (chat_panel_widget_) {
                    chat_panel_widget_->setConversationStatus(reason);
                }
            });
    connect(&chat_service_, &cloudvault::ChatService::groupHistoryLoaded,
            this, [this](int group_id, const QList<cloudvault::ChatMessage>& messages) {
                group_messages_[group_id] = messages;
                if (group_id == active_group_id_ && chat_panel_widget_) {
                    chat_panel_widget_->rebuildMessages(messages, current_username_);
                }
            });

    connect(&group_service_, &cloudvault::GroupService::groupListReceived,
            this, &MainWindow::refreshGroupList);
    connect(&group_service_, &cloudvault::GroupService::groupCreated,
            this, [this](int group_id, const QString& name) {
                auto& summary = group_summaries_[group_id];
                summary.name = name;
                group_service_.getGroupList();
                QMessageBox::information(this,
                                         QStringLiteral("群组"),
                                         QStringLiteral("已创建群组：%1").arg(name));
                showGroupConversation(group_id);
            });
    connect(&group_service_, &cloudvault::GroupService::groupCreateFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("创建群组失败"), reason);
            });
    connect(&group_service_, &cloudvault::GroupService::joinedGroup,
            this, [this](int) { group_service_.getGroupList(); });
    connect(&group_service_, &cloudvault::GroupService::joinFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("加入群组失败"), reason);
            });
    connect(&group_service_, &cloudvault::GroupService::leftGroup,
            this, [this](int group_id) {
                group_messages_.remove(group_id);
                if (group_id == active_group_id_) {
                    showChatEmptyState();
                }
                if (group_id == active_contact_group_id_) {
                    showContactEmptyState();
                }
                group_service_.getGroupList();
            });
    connect(&group_service_, &cloudvault::GroupService::leaveFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("退出群组失败"), reason);
            });
    connect(&group_service_, &cloudvault::GroupService::groupMessageReceived,
            this, [this](const QString& from, int group_id, const QString& content, const QString& timestamp) {
                const QString group_name = group_summaries_.value(group_id).name;
                const cloudvault::ChatMessage message{
                    group_id,
                    from,
                    group_name,
                    content,
                    timestamp,
                };
                auto& cache = group_messages_[group_id];
                cache.append(message);

                auto it = group_summaries_.find(group_id);
                if (it != group_summaries_.end() && group_id != active_group_id_) {
                    ++it->unread_count;
                }

                if (group_id == active_group_id_ && chat_panel_widget_) {
                    chat_panel_widget_->appendMessage(message, current_username_);
                    if (auto summary = group_summaries_.value(group_id); !summary.name.isEmpty()) {
                        chat_panel_widget_->setConversationStatus(
                            QStringLiteral("%1 人在线").arg(summary.online_count));
                    }
                }
            });

    connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
            this, &MainWindow::refreshFriendList);
    connect(&friend_service_, &cloudvault::FriendService::friendAdded,
            this, [this](const QString& username) {
                incoming_friend_requests_.removeAll(username);
                outgoing_friend_requests_.removeAll(username);
                friend_service_.flushFriends();
                refreshSidebarForCurrentMode();
            });
    connect(&friend_service_, &cloudvault::FriendService::friendDeleted,
            this, [this](const QString& username) {
                if (username == active_chat_peer_) {
                    showChatEmptyState();
                }
                if (username == active_contact_friend_) {
                    showContactEmptyState();
                }
                friend_service_.flushFriends();
            });
    connect(&friend_service_, &cloudvault::FriendService::friendRequestSent,
            this, [this](const QString& target) {
                if (!target.isEmpty() && !outgoing_friend_requests_.contains(target)) {
                    outgoing_friend_requests_.prepend(target);
                }
                refreshSidebarForCurrentMode();
            });
    connect(&friend_service_, &cloudvault::FriendService::friendAddFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("添加失败"), reason);
            });
    connect(&friend_service_, &cloudvault::FriendService::incomingFriendRequest,
            this, [this](const QString& from) {
                if (!from.isEmpty() && !incoming_friend_requests_.contains(from)) {
                    incoming_friend_requests_.prepend(from);
                }
                refreshSidebarForCurrentMode();
            });

    connect(&file_service_, &cloudvault::FileService::filesListed,
            this, &MainWindow::refreshFileList);
    connect(&file_service_, &cloudvault::FileService::fileListFailed,
            this, [this](const QString& reason) { setFileStatus(reason, true); });
    connect(&file_service_, &cloudvault::FileService::searchCompleted,
            this, [this](const QString& keyword, const cloudvault::FileEntries& entries) {
                file_search_mode_ = true;
                current_file_query_ = keyword;
                active_file_context_key_ = QStringLiteral("file:search");
                refreshFileList(QStringLiteral("搜索：%1").arg(keyword), entries);
            });
    connect(&file_service_, &cloudvault::FileService::searchFailed,
            this, [this](const QString& reason) { setFileStatus(reason, true); });
    connect(&file_service_, &cloudvault::FileService::fileOperationSucceeded,
            this, [this](const QString& message) {
                setFileStatus(message);
                file_search_mode_ = false;
                current_file_query_.clear();
                active_file_context_key_ = QStringLiteral("file:current");
                if (file_panel_widget_) {
                    file_panel_widget_->clearSearch();
                }
                file_service_.listFiles(current_file_path_);
            });
    connect(&file_service_, &cloudvault::FileService::fileOperationFailed,
            this, [this](const QString& message) { setFileStatus(message, true); });
    connect(&file_service_, &cloudvault::FileService::uploadProgress,
            this, [this](const QString& filename, quint64 sent, quint64 total) {
                const int percent = (total == 0)
                    ? 0
                    : static_cast<int>((static_cast<double>(sent) * 100.0) / static_cast<double>(total));
                showTransferRow(QStringLiteral("上传 %1").arg(filename), percent, true);
                setFileStatus(QStringLiteral("正在上传 %1 · %2 / %3")
                                  .arg(filename, formatFileSize(sent), formatFileSize(total)));
                upsertRecentTransfer(QStringLiteral("upload:%1").arg(filename),
                                     filename,
                                     QStringLiteral("上传中 %1 / %2")
                                         .arg(formatFileSize(sent), formatFileSize(total)),
                                     QStringLiteral("%1%").arg(percent));
            });
    connect(&file_service_, &cloudvault::FileService::uploadFinished,
            this, [this](const QString& remote_path, const QString& message) {
                showTransferRow(QStringLiteral("上传完成"), 100, !pending_upload_paths_.isEmpty());
                setFileStatus(QStringLiteral("%1 · %2").arg(message, remote_path));
                file_search_mode_ = false;
                current_file_query_.clear();
                if (file_panel_widget_) {
                    file_panel_widget_->clearSearch();
                }
                file_service_.listFiles(current_file_path_);
                QTimer::singleShot(0, this, &MainWindow::startNextQueuedUpload);
                upsertRecentTransfer(QStringLiteral("upload:%1").arg(QFileInfo(remote_path).fileName()),
                                     QFileInfo(remote_path).fileName(),
                                     QStringLiteral("上传完成"),
                                     QStringLiteral("DONE"));
                QTimer::singleShot(1500, this, [this] {
                    if (!file_service_.hasActiveTransfer() && pending_upload_paths_.isEmpty()) {
                        hideTransferRow();
                    }
                });
            });
    connect(&file_service_, &cloudvault::FileService::uploadFailed,
            this, [this](const QString& message) {
                showTransferRow(QStringLiteral("上传失败"), 0, !pending_upload_paths_.isEmpty());
                setFileStatus(message, true);
                upsertRecentTransfer(QStringLiteral("upload:error:%1").arg(QDateTime::currentMSecsSinceEpoch()),
                                     QStringLiteral("上传失败"),
                                     message,
                                     QStringLiteral("ERR"));
                QTimer::singleShot(0, this, &MainWindow::startNextQueuedUpload);
                QTimer::singleShot(1500, this, [this] {
                    if (!file_service_.hasActiveTransfer() && pending_upload_paths_.isEmpty()) {
                        hideTransferRow();
                    }
                });
            });
    connect(&file_service_, &cloudvault::FileService::downloadProgress,
            this, [this](const QString& filename, quint64 received, quint64 total) {
                const int percent = (total == 0)
                    ? 0
                    : static_cast<int>((static_cast<double>(received) * 100.0) / static_cast<double>(total));
                showTransferRow(QStringLiteral("下载 %1").arg(filename), percent, true);
                setFileStatus(QStringLiteral("正在下载 %1 · %2 / %3")
                                  .arg(filename, formatFileSize(received), formatFileSize(total)));
                upsertRecentTransfer(QStringLiteral("download:%1").arg(filename),
                                     filename,
                                     QStringLiteral("下载中 %1 / %2")
                                         .arg(formatFileSize(received), formatFileSize(total)),
                                     QStringLiteral("%1%").arg(percent));
            });
    connect(&file_service_, &cloudvault::FileService::downloadFinished,
            this, [this](const QString& local_path, const QString& message) {
                showTransferRow(QStringLiteral("下载完成"), 100, false);
                setFileStatus(QStringLiteral("%1 · %2").arg(message, local_path));
                upsertRecentTransfer(QStringLiteral("download:%1").arg(QFileInfo(local_path).fileName()),
                                     QFileInfo(local_path).fileName(),
                                     QStringLiteral("下载完成"),
                                     QStringLiteral("DONE"));
                QTimer::singleShot(1500, this, &MainWindow::hideTransferRow);
                QMessageBox::information(this, QStringLiteral("下载完成"), local_path);
            });
    connect(&file_service_, &cloudvault::FileService::downloadFailed,
            this, [this](const QString& message) {
                showTransferRow(QStringLiteral("下载失败"), 0, false);
                setFileStatus(message, true);
                upsertRecentTransfer(QStringLiteral("download:error:%1").arg(QDateTime::currentMSecsSinceEpoch()),
                                     QStringLiteral("下载失败"),
                                     message,
                                     QStringLiteral("ERR"));
                QTimer::singleShot(1500, this, &MainWindow::hideTransferRow);
            });

    connect(&share_service_, &cloudvault::ShareService::shareRequestSent,
            this, [this](const QString& message) { setFileStatus(message); });
    connect(&share_service_, &cloudvault::ShareService::shareRequestFailed,
            this, [this](const QString& reason) { setFileStatus(reason, true); });
    connect(&share_service_, &cloudvault::ShareService::incomingShareRequest,
            this, [this](const QString& from, const QString& file_path) {
                QMessageBox box(this);
                box.setIcon(QMessageBox::Question);
                box.setWindowTitle(QStringLiteral("收到文件分享"));
                box.setText(QStringLiteral("%1 想与你共享文件").arg(from));
                box.setInformativeText(
                    QStringLiteral("文件：%1\n来源路径：%2\n接收后会复制到你的根目录。")
                        .arg(QFileInfo(file_path).fileName(), file_path));
                auto* accept_btn = box.addButton(QStringLiteral("接收"), QMessageBox::AcceptRole);
                box.addButton(QStringLiteral("稍后处理"), QMessageBox::RejectRole);
                box.exec();
                if (box.clickedButton() == accept_btn) {
                    setFileStatus(QStringLiteral("正在接收来自 %1 的分享…").arg(from));
                    share_service_.acceptShare(from, file_path);
                }
            });
    connect(&share_service_, &cloudvault::ShareService::shareAccepted,
            this, [this](const QString& message) {
                setFileStatus(message);
                file_search_mode_ = false;
                current_file_query_.clear();
                active_file_context_key_ = QStringLiteral("file:root");
                if (file_panel_widget_) {
                    file_panel_widget_->clearSearch();
                }
                file_service_.listFiles(QStringLiteral("/"));
            });
    connect(&share_service_, &cloudvault::ShareService::shareAcceptFailed,
            this, [this](const QString& reason) {
                setFileStatus(reason, true);
                QMessageBox::warning(this, QStringLiteral("接收失败"), reason);
            });

    auto* refresh_shortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(refresh_shortcut, &QShortcut::activated, this, [this] {
        if (active_main_view_ == MainView::Message || active_main_view_ == MainView::Contact) {
            friend_service_.flushFriends();
            group_service_.getGroupList();
            if (!active_chat_peer_.isEmpty()) {
                requestConversationSnapshot(active_chat_peer_);
            }
        } else if (active_main_view_ == MainView::File) {
            if (file_search_mode_ && !current_file_query_.isEmpty()) {
                file_service_.search(current_file_query_);
            } else {
                file_service_.listFiles(current_file_path_);
            }
        }
    });

    auto* delete_shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(delete_shortcut, &QShortcut::activated, this, [this] {
        if (active_main_view_ == MainView::File && file_panel_widget_ && file_panel_widget_->hasSelection()) {
            deleteSelectedFile();
        }
    });

    auto* rename_shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    connect(rename_shortcut, &QShortcut::activated, this, [this] {
        if (active_main_view_ == MainView::File && file_panel_widget_ && file_panel_widget_->hasSelection()) {
            renameSelectedFile();
        }
    });

    auto* back_shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
    connect(back_shortcut, &QShortcut::activated, this, [this] {
        if (active_main_view_ == MainView::File) {
            openCurrentParentDirectory();
        }
    });

    auto* focus_search_shortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+F")), this);
    connect(focus_search_shortcut, &QShortcut::activated, this, [this] {
        if (active_main_view_ == MainView::File) {
            if (file_panel_widget_) {
                file_panel_widget_->focusSearchSelectAll();
            }
        }
    });

    auto* upload_shortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+U")), this);
    connect(upload_shortcut, &QShortcut::activated, this, [this] {
        if (active_main_view_ == MainView::File) {
            uploadFile();
        }
    });
}

void MainWindow::showChatEmptyState() {
    active_chat_peer_.clear();
    active_group_name_.clear();
    active_group_id_ = 0;
    if (chat_panel_widget_) {
        chat_panel_widget_->showEmptyState();
    }
}

void MainWindow::showContactEmptyState() {
    active_contact_friend_.clear();
    active_contact_group_id_ = 0;
    if (contact_panel_widget_) {
        contact_panel_widget_->showEmptyState();
    }
}

void MainWindow::showChatConversation(const QString& username, bool online) {
    active_group_name_.clear();
    active_group_id_ = 0;
    const cloudvault::FriendProfile* profile = findFriendProfile(friends_, username);
    const QString display_name = friendDisplayName(profile, username);
    if (chat_panel_widget_) {
        chat_panel_widget_->showConversation(display_name, formatPresence(online));
    }
}

void MainWindow::showFriendDetails(const QString& username, bool online) {
    active_contact_friend_ = username;
    active_contact_group_id_ = 0;
    const cloudvault::FriendProfile* profile = findFriendProfile(friends_, username);
    const QString display_name = friendDisplayName(profile, username);
    const QString signature = profile ? profile->signature.trimmed() : QString();

    if (contact_panel_widget_) {
        contact_panel_widget_->showFriendProfile(username,
                                                 display_name,
                                                 signature,
                                                 online);
    }
}

void MainWindow::showFriendRequestDetails(const QString& username, bool incoming) {
    active_contact_group_id_ = 0;
    active_contact_friend_ = username;
    if (contact_panel_widget_) {
        contact_panel_widget_->showFriendRequest(username, incoming);
    }
}

void MainWindow::showGroupDetails(int group_id) {
    const auto it = group_summaries_.find(group_id);
    if (it == group_summaries_.end()) {
        showContactEmptyState();
        return;
    }

    active_contact_friend_.clear();
    active_contact_group_id_ = group_id;
    if (contact_panel_widget_) {
        contact_panel_widget_->showGroupProfile(group_id,
                                                it->name,
                                                it->owner_id,
                                                it->online_count);
    }
}

void MainWindow::requestConversationSnapshot(const QString& peer) {
    const QString trimmed = peer.trimmed();
    if (trimmed.isEmpty() || pending_history_requests_.contains(trimmed)) {
        return;
    }
    pending_history_requests_.insert(trimmed);
    chat_service_.loadHistory(trimmed);
}

void MainWindow::updateConversationSummary(const QString& peer,
                                           const QList<cloudvault::ChatMessage>& messages) {
    auto& summary = conversation_summaries_[peer];
    if (messages.isEmpty()) {
        summary.preview.clear();
        summary.timestamp.clear();
        summary.unread_count = 0;
        summary.has_message = false;
        summary.last_message_outgoing = false;
    } else {
        const auto& latest = messages.constLast();
        summary.last_message_outgoing = latest.from == current_username_;
        summary.preview = formatConversationPreview(latest.content, summary.last_message_outgoing);
        summary.timestamp = latest.timestamp;
        summary.has_message = true;
        summary.unread_count = (peer == active_chat_peer_) ? 0 : calculateUnreadCount(peer, messages);
    }
    refreshSidebarForCurrentMode();
}

void MainWindow::applyConversationMessage(const cloudvault::ChatMessage& message, bool increment_unread) {
    const QString peer = (message.from == current_username_) ? message.to : message.from;
    if (peer.isEmpty()) {
        return;
    }

    auto& summary = conversation_summaries_[peer];
    summary.last_message_outgoing = message.from == current_username_;
    summary.preview = formatConversationPreview(message.content, summary.last_message_outgoing);
    summary.timestamp = message.timestamp;
    summary.has_message = true;
    if (peer == active_chat_peer_) {
        summary.unread_count = 0;
        if (message.from == peer) {
            saveConversationReadAt(peer, parseConversationTimestamp(message.timestamp));
        }
    } else if (increment_unread) {
        ++summary.unread_count;
    }
    refreshSidebarForCurrentMode();
}

void MainWindow::clearConversationUnread(const QString& peer) {
    auto it = conversation_summaries_.find(peer);
    if (it == conversation_summaries_.end() || it->unread_count == 0) {
        return;
    }
    it->unread_count = 0;
    refreshSidebarForCurrentMode();
}

int MainWindow::calculateUnreadCount(const QString& peer,
                                     const QList<cloudvault::ChatMessage>& messages) const {
    const QDateTime last_read_at = loadConversationReadAt(peer);
    if (!last_read_at.isValid()) {
        int unread_count = 0;
        for (const auto& message : messages) {
            if (message.from == peer) {
                ++unread_count;
            }
        }
        return unread_count;
    }

    int unread_count = 0;
    for (const auto& message : messages) {
        if (message.from != peer) {
            continue;
        }
        const QDateTime message_time = parseConversationTimestamp(message.timestamp);
        if (!message_time.isValid() || message_time > last_read_at) {
            ++unread_count;
        }
    }
    return unread_count;
}

QDateTime MainWindow::loadConversationReadAt(const QString& peer) const {
    if (peer.trimmed().isEmpty()) {
        return {};
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("chat/read-state/%1").arg(current_username_));
    const QDateTime value = settings.value(peer.trimmed()).toDateTime();
    settings.endGroup();
    return value;
}

void MainWindow::saveConversationReadAt(const QString& peer, const QDateTime& timestamp) const {
    const QString trimmed = peer.trimmed();
    if (trimmed.isEmpty() || !timestamp.isValid()) {
        return;
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("chat/read-state/%1").arg(current_username_));
    const QDateTime current = settings.value(trimmed).toDateTime();
    if (!current.isValid() || timestamp > current) {
        settings.setValue(trimmed, timestamp);
    }
    settings.endGroup();
}

void MainWindow::refreshFriendList(const QList<cloudvault::FriendProfile>& friends) {
    friends_ = friends;
    QSet<QString> current_peers;
    for (const auto& profile : friends_) {
        current_peers.insert(profile.username);
        requestConversationSnapshot(profile.username);
    }

    const auto known_peers = conversation_summaries_.keys();
    for (const auto& peer : known_peers) {
        if (!current_peers.contains(peer)) {
            conversation_summaries_.remove(peer);
            pending_history_requests_.remove(peer);
        }
    }
    if (!active_chat_peer_.isEmpty() && !current_peers.contains(active_chat_peer_)) {
        showChatEmptyState();
    }
    if (!active_contact_friend_.isEmpty() && !current_peers.contains(active_contact_friend_)) {
        showContactEmptyState();
    }
    refreshSidebarForCurrentMode();
}

void MainWindow::filterFriendList(const QString& keyword) {
    const QString current = !active_chat_peer_.isEmpty()
        ? active_chat_peer_
        : (sidebar_widget_ ? sidebar_widget_->currentKey() : QString());
    const QString trimmed = keyword.trimmed();
    QList<cloudvault::FriendProfile> ordered_friends = friends_;
    std::sort(ordered_friends.begin(), ordered_friends.end(),
              [this](const cloudvault::FriendProfile& lhs, const cloudvault::FriendProfile& rhs) {
                  const auto lhs_summary = conversation_summaries_.constFind(lhs.username);
                  const auto rhs_summary = conversation_summaries_.constFind(rhs.username);
                  const bool lhs_has_message = lhs_summary != conversation_summaries_.constEnd()
                                            && lhs_summary->has_message;
                  const bool rhs_has_message = rhs_summary != conversation_summaries_.constEnd()
                                            && rhs_summary->has_message;
                  if (lhs_has_message != rhs_has_message) {
                      return lhs_has_message;
                  }

                  if (lhs_has_message && rhs_has_message) {
                      const QDateTime lhs_time = parseConversationTimestamp(lhs_summary->timestamp);
                      const QDateTime rhs_time = parseConversationTimestamp(rhs_summary->timestamp);
                      if (lhs_time.isValid() && rhs_time.isValid() && lhs_time != rhs_time) {
                          return lhs_time > rhs_time;
                      }
                      if (lhs_summary->unread_count != rhs_summary->unread_count) {
                          return lhs_summary->unread_count > rhs_summary->unread_count;
                      }
                  }

                  if (lhs.online != rhs.online) {
                      return lhs.online;
                  }
                  const QString lhs_name = friendDisplayName(&lhs, lhs.username);
                  const QString rhs_name = friendDisplayName(&rhs, rhs.username);
                  return QString::localeAwareCompare(lhs_name, rhs_name) < 0;
              });

    {
        QList<SidebarEntry> visible_contacts;

        for (const auto& profile : ordered_friends) {
            const QString display_name = friendDisplayName(&profile, profile.username);
            if (!trimmed.isEmpty()
                && !profile.username.contains(trimmed, Qt::CaseInsensitive)
                && !display_name.contains(trimmed, Qt::CaseInsensitive)) {
                continue;
            }

            const auto summary_it = conversation_summaries_.constFind(profile.username);
            const bool has_message = summary_it != conversation_summaries_.constEnd()
                                  && summary_it->has_message;
            SidebarEntry entry;
            entry.key = profile.username;
            entry.title = display_name;
            entry.preview = has_message ? summary_it->preview : QStringLiteral("暂无消息");
            entry.time = has_message ? formatConversationTime(summary_it->timestamp) : QString();
            entry.unread = has_message ? summary_it->unread_count : 0;
            entry.online = profile.online;
            entry.type = SidebarItemType::Friend;
            visible_contacts.append(entry);
        }

        for (auto it = group_summaries_.cbegin(); it != group_summaries_.cend(); ++it) {
            if (!trimmed.isEmpty() && !it->name.contains(trimmed, Qt::CaseInsensitive)) {
                continue;
            }
            const auto& cached_msgs = group_messages_.value(it.key());
            const bool has_group_msg = !cached_msgs.isEmpty();
            SidebarEntry entry;
            entry.key = QStringLiteral("group:%1").arg(it.key());
            entry.title = it->name;
            entry.preview = has_group_msg
                ? formatConversationPreview(cached_msgs.last().content, false)
                : QStringLiteral("暂无消息");
            entry.time = has_group_msg
                ? formatConversationTime(cached_msgs.last().timestamp)
                : QString();
            entry.unread = it->unread_count;
            entry.online = false;
            entry.type = SidebarItemType::Group;
            entry.id = it.key();
            visible_contacts.append(entry);
        }

        if (sidebar_widget_) {
            sidebar_widget_->populateEntries(visible_contacts,
                                            current,
                                            current.isEmpty());
        }
    }

    if (!sidebar_widget_ || sidebar_widget_->itemCount() == 0) {
        showChatEmptyState();
    }

    updateContactSelectionState();

    if ((sidebar_widget_ ? sidebar_widget_->currentKey() : QString()) != current) {
        applySelectedSidebarItem();
    }
}

void MainWindow::filterContactList(const QString& keyword) {
    const QString trimmed = keyword.trimmed();
    QString current;
    if (!active_contact_friend_.isEmpty()) {
        if (incoming_friend_requests_.contains(active_contact_friend_)) {
            current = QStringLiteral("request:incoming:%1").arg(active_contact_friend_);
        } else if (outgoing_friend_requests_.contains(active_contact_friend_)) {
            current = QStringLiteral("request:outgoing:%1").arg(active_contact_friend_);
        } else {
            current = active_contact_friend_;
        }
    } else if (active_contact_group_id_ > 0) {
        current = QStringLiteral("group:%1").arg(active_contact_group_id_);
    }

    QList<SidebarEntry> entries;

    SidebarEntry entry;
    entry.type = SidebarItemType::Section;
    entry.title = QStringLiteral("新朋友");
    entry.tag = QStringLiteral("快捷入口");
    entries.append(entry);

    entry = {};
    entry.key = QStringLiteral("contact:new-friend");
    entry.title = QStringLiteral("添加好友");
    entry.preview = QStringLiteral("搜索用户并发送好友申请");
    entry.type = SidebarItemType::Action;
    entry.tag = QStringLiteral("NEW");
    entries.append(entry);

    entry = {};
    entry.type = SidebarItemType::Section;
    entry.title = QStringLiteral("我的好友");
    entry.tag = QStringLiteral("%1").arg(friends_.size());
    entries.append(entry);

    for (const QString& username : incoming_friend_requests_) {
        if (!trimmed.isEmpty() && !username.contains(trimmed, Qt::CaseInsensitive)) {
            continue;
        }
        SidebarEntry req;
        req.key = QStringLiteral("request:incoming:%1").arg(username);
        req.title = username;
        req.preview = QStringLiteral("收到的好友申请");
        req.type = SidebarItemType::Action;
        req.tag = QStringLiteral("IN");
        entries.append(req);
    }

    for (const QString& username : outgoing_friend_requests_) {
        if (!trimmed.isEmpty() && !username.contains(trimmed, Qt::CaseInsensitive)) {
            continue;
        }
        SidebarEntry req;
        req.key = QStringLiteral("request:outgoing:%1").arg(username);
        req.title = username;
        req.preview = QStringLiteral("已发送的好友申请");
        req.type = SidebarItemType::Action;
        req.tag = QStringLiteral("OUT");
        entries.append(req);
    }

    for (const auto& profile : friends_) {
        const QString display_name = friendDisplayName(&profile, profile.username);
        if (!trimmed.isEmpty()
            && !profile.username.contains(trimmed, Qt::CaseInsensitive)
            && !display_name.contains(trimmed, Qt::CaseInsensitive)) {
            continue;
        }
        SidebarEntry entry;
        entry.key = profile.username;
        entry.title = display_name;
        entry.preview = profile.signature.trimmed().isEmpty()
            ? (profile.online ? QStringLiteral("好友 · 在线") : QStringLiteral("好友 · 离线"))
            : profile.signature.trimmed();
        entry.online = profile.online;
        entry.type = SidebarItemType::Friend;
        entry.tag = !profile.nickname.trimmed().isEmpty() ? QStringLiteral("PROFILE") : QString();
        entries.append(entry);
    }
    QList<SidebarEntry> friend_entries;
    QList<SidebarEntry> group_entries;
    for (const auto& item : entries) {
        if (item.type == SidebarItemType::Friend) {
            friend_entries.append(item);
        }
    }
    std::sort(friend_entries.begin(), friend_entries.end(), [](const SidebarEntry& lhs, const SidebarEntry& rhs) {
        if (lhs.online != rhs.online) {
            return lhs.online;
        }
        return QString::localeAwareCompare(lhs.title, rhs.title) < 0;
    });
    for (auto it = group_summaries_.cbegin(); it != group_summaries_.cend(); ++it) {
        if (!trimmed.isEmpty() && !it->name.contains(trimmed, Qt::CaseInsensitive)) {
            continue;
        }
        const auto& cached_msgs = group_messages_.value(it.key());
        const bool has_group_msg = !cached_msgs.isEmpty();
        SidebarEntry entry;
        entry.key = QStringLiteral("group:%1").arg(it.key());
        entry.title = it->name;
        entry.preview = has_group_msg
            ? formatConversationPreview(cached_msgs.last().content, false)
            : QStringLiteral("%1 人在线").arg(it->online_count);
        entry.unread = it->unread_count;
        entry.type = SidebarItemType::Group;
        entry.id = it.key();
        group_entries.append(entry);
    }
    std::sort(group_entries.begin(), group_entries.end(), [](const SidebarEntry& lhs, const SidebarEntry& rhs) {
        if (lhs.unread != rhs.unread) {
            return lhs.unread > rhs.unread;
        }
        return QString::localeAwareCompare(lhs.title, rhs.title) < 0;
    });

    QList<SidebarEntry> merged_entries;
    merged_entries.reserve(2 + friend_entries.size() + 1 + group_entries.size());
    merged_entries.append(entries.at(0));
    merged_entries.append(entries.at(1));
    merged_entries.append(entries.at(2));
    for (const auto& item : friend_entries) {
        merged_entries.append(item);
    }
    SidebarEntry group_section;
    group_section.type = SidebarItemType::Section;
    group_section.title = QStringLiteral("群组");
    group_section.tag = QStringLiteral("%1").arg(group_entries.size());
    merged_entries.append(group_section);

    SidebarEntry create_group_entry;
    create_group_entry.key = QStringLiteral("contact:create-group");
    create_group_entry.title = QStringLiteral("发起群聊");
    create_group_entry.preview = QStringLiteral("创建新群组并邀请好友加入");
    create_group_entry.type = SidebarItemType::Action;
    create_group_entry.tag = QStringLiteral("NEW");
    merged_entries.append(create_group_entry);

    for (const auto& item : group_entries) {
        merged_entries.append(item);
    }

    if (sidebar_widget_) {
        // 联系人侧栏不自动选中第一项——Action 类型的快捷入口（如"添加好友"）
        // 被自动选中会立即触发弹窗，导致切换到联系人选项卡时直接阻塞界面。
        // 仅当已有激活的联系人 / 群组时才恢复对应选中态。
        sidebar_widget_->populateEntries(merged_entries, current, false);
    }

    if (!sidebar_widget_ || sidebar_widget_->itemCount() == 0) {
        showContactEmptyState();
    }

    updateContactSelectionState();
    if (!current.isEmpty()
        && (sidebar_widget_ ? sidebar_widget_->currentKey() : QString()) != current) {
        applySelectedSidebarItem();
    }
}

void MainWindow::updateContactSelectionState() {
    if (sidebar_widget_) {
        sidebar_widget_->refreshSelectionHighlights();
    }
}

void MainWindow::applySelectedSidebarItem() {
    updateContactSelectionState();

    const QString key = sidebar_widget_ ? sidebar_widget_->currentKey() : QString();
    if (key.isEmpty()) {
        if (active_main_view_ == MainView::Contact) {
            showContactEmptyState();
        } else if (active_main_view_ == MainView::File) {
            syncFileContextChrome();
        } else {
            showChatEmptyState();
        }
        return;
    }

    if (active_main_view_ == MainView::File) {
        const SidebarItemType item_type = sidebar_widget_
            ? sidebar_widget_->currentItemType()
            : SidebarItemType::FileContext;
        const QString current_key = sidebar_widget_ ? sidebar_widget_->currentKey() : QString();
        active_file_context_key_ = current_key;
        if (item_type == SidebarItemType::Action && current_key == QStringLiteral("file:upload")) {
            uploadFile();
            return;
        }
        if (current_key == QStringLiteral("file:root")) {
            navigateToFilePath(QStringLiteral("/"));
            return;
        }
        if (current_key.startsWith(QStringLiteral("path:"))) {
            navigateToFilePath(current_key.mid(QStringLiteral("path:").size()));
            return;
        }
        if (current_key == QStringLiteral("file:current")) {
            if (file_panel_widget_) {
                file_panel_widget_->clearCurrentSelection();
            }
            syncFileContextChrome();
            return;
        }
        if (current_key == QStringLiteral("file:search")) {
            if (file_panel_widget_) {
                file_panel_widget_->clearCurrentSelection();
            }
            syncFileContextChrome();
            return;
        }
        if (current_key == QStringLiteral("file:transfer")) {
            if (file_panel_widget_) {
                file_panel_widget_->clearCurrentSelection();
            }
            syncFileContextChrome();
            return;
        }
        const bool is_recent_transfer = std::any_of(
            recent_transfers_.cbegin(),
            recent_transfers_.cend(),
            [&current_key](const RecentTransferItem& item) { return item.key == current_key; });
        if (is_recent_transfer) {
            if (file_panel_widget_) {
                file_panel_widget_->clearCurrentSelection();
            }
            syncFileContextChrome();
            return;
        }
        syncFileContextChrome();
        return;
    }

    const SidebarItemType item_type = sidebar_widget_
        ? sidebar_widget_->currentItemType()
        : SidebarItemType::Friend;
    const bool online = sidebar_widget_ ? sidebar_widget_->currentOnline() : false;

    if (active_main_view_ == MainView::Contact) {
        if (item_type == SidebarItemType::Action && key == QStringLiteral("contact:new-friend")) {
            openOnlineUserDialog();
            return;
        }
        if (item_type == SidebarItemType::Action && key == QStringLiteral("contact:create-group")) {
            openGroupListDialog();
            return;
        }
        if (item_type == SidebarItemType::Action && key.startsWith(QStringLiteral("request:incoming:"))) {
            showFriendRequestDetails(key.mid(QStringLiteral("request:incoming:").size()), true);
            return;
        }
        if (item_type == SidebarItemType::Action && key.startsWith(QStringLiteral("request:outgoing:"))) {
            showFriendRequestDetails(key.mid(QStringLiteral("request:outgoing:").size()), false);
            return;
        }
        if (item_type == SidebarItemType::Group) {
            showGroupDetails(sidebar_widget_ ? sidebar_widget_->currentItemId() : 0);
        } else {
            showFriendDetails(key, online);
        }
        return;
    }

    if (item_type == SidebarItemType::Group) {
        active_chat_peer_.clear();
        showGroupConversation(sidebar_widget_ ? sidebar_widget_->currentItemId() : 0);
        return;
    }

    active_chat_peer_ = key;
    active_group_id_ = 0;
    clearConversationUnread(key);
    showChatConversation(key, online);
    if (chat_panel_widget_) {
        chat_panel_widget_->setConversationStatus(QStringLiteral("正在加载历史消息…"));
    }
    requestConversationSnapshot(key);
}

void MainWindow::sendCurrentMessage() {
    if (active_chat_peer_.isEmpty() && active_group_id_ == 0) {
        QMessageBox::warning(this, QStringLiteral("无法发送"),
                             QStringLiteral("请先选择联系人或群组。"));
        return;
    }

    const bool is_group = active_group_id_ > 0;
    const QString content = chat_panel_widget_
        ? chat_panel_widget_->inputText().trimmed()
        : QString();
    if (content.isEmpty()) {
        return;
    }

    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    if (is_group) {
        group_service_.sendGroupMessage(active_group_id_, content);
        cloudvault::ChatMessage local_message{
            active_group_id_,
            current_username_,
            active_group_name_,
            content,
            now,
        };
        group_messages_[active_group_id_].append(local_message);
        if (chat_panel_widget_) {
            chat_panel_widget_->appendMessage(local_message, current_username_);
            chat_panel_widget_->clearInput();
            chat_panel_widget_->setConversationStatus(
                QStringLiteral("%1 人在线").arg(group_summaries_.value(active_group_id_).online_count));
        }
        return;
    }

    chat_service_.sendMessage(active_chat_peer_, content);
    const cloudvault::ChatMessage local_message{
        0,
        current_username_,
        active_chat_peer_,
        content,
        now,
    };
    if (chat_panel_widget_) {
        chat_panel_widget_->appendMessage(local_message, current_username_);
        chat_panel_widget_->clearInput();
    }
    applyConversationMessage(local_message, false);
    showChatConversation(active_chat_peer_, lookupFriendOnline(friends_, active_chat_peer_));
    QTimer::singleShot(120, this, [this, peer = active_chat_peer_] {
        if (!peer.isEmpty() && peer == active_chat_peer_) {
            requestConversationSnapshot(peer);
        }
    });
}

void MainWindow::setFileStatus(const QString& message, bool error) {
    if (file_panel_widget_) {
        file_panel_widget_->setStatusMessage(message, error);
    }
}

void MainWindow::resetFileActionSummary() {
    if (file_panel_widget_) {
        file_panel_widget_->resetSelectionSummary();
    }
}

void MainWindow::refreshFileList(const QString& path,
                                 const cloudvault::FileEntries& entries) {
    if (!file_search_mode_) {
        current_file_path_ = path.isEmpty() ? QStringLiteral("/") : path;
    }
    current_file_entries_ = entries;
    if (file_panel_widget_) {
        file_panel_widget_->populateEntries(entries);
    }

    if (file_panel_widget_) {
        file_panel_widget_->setEmptyState(
            file_search_mode_
                ? QStringLiteral("没有匹配的文件\n修改关键词后回车重新搜索")
                : QStringLiteral("当前目录为空\n点击“+ 新建”或使用“上传”开始管理文件"),
            file_panel_widget_->itemCount() == 0);
    }

    if (file_panel_widget_ && file_panel_widget_->itemCount() > 0) {
        if (file_panel_widget_) {
            file_panel_widget_->selectFirstEntry();
        }
        setFileStatus(file_search_mode_
                          ? QStringLiteral("搜索完成，共找到 %1 项。").arg(file_panel_widget_->itemCount())
                          : QStringLiteral("目录已刷新，共 %1 项。").arg(file_panel_widget_->itemCount()));
    } else {
        resetFileActionSummary();
        setFileStatus(file_search_mode_
                          ? QStringLiteral("没有找到与“%1”匹配的文件。").arg(current_file_query_)
                          : QStringLiteral("当前目录为空。"));
    }

    updateFileSelectionState();
    syncFileContextChrome();
    refreshSidebarForCurrentMode();
}

void MainWindow::updateFileSelectionState() {
    if (file_panel_widget_) {
        file_panel_widget_->refreshSelectionHighlights();
    }

    const bool has_selection = file_panel_widget_ && file_panel_widget_->hasSelection();
    const bool is_file = has_selection && !(file_panel_widget_ ? file_panel_widget_->currentIsDir() : false);
    if (file_panel_widget_) {
        file_panel_widget_->setActionButtonsEnabled(has_selection, is_file);
    }
}

void MainWindow::applySelectedFile() {
    updateFileSelectionState();

    const QString path = file_panel_widget_ ? file_panel_widget_->currentPath() : QString();
    if (path.isEmpty()) {
        resetFileActionSummary();
        syncFileContextChrome();
        return;
    }

    const bool is_dir = file_panel_widget_ ? file_panel_widget_->currentIsDir() : false;
    const QString name = file_panel_widget_ ? file_panel_widget_->currentName() : QString();
    quint64 file_size = 0;
    const auto entry_it = std::find_if(current_file_entries_.cbegin(),
                                       current_file_entries_.cend(),
                                       [&path](const cloudvault::FileEntry& entry) {
                                           return entry.path == path;
                                       });
    if (entry_it != current_file_entries_.cend()) {
        file_size = entry_it->size;
    }
    if (file_panel_widget_) {
        file_panel_widget_->setSelectionSummary(
            name,
            is_dir ? QStringLiteral("目录")
                   : QStringLiteral("文件"),
            path);
    }
    if (file_panel_widget_) {
        file_panel_widget_->setContextSummary(
            name,
            is_dir ? QStringLiteral("目录")
                   : QStringLiteral("文件 · %1").arg(formatFileSize(file_size)));
    }
}

void MainWindow::navigateToFilePath(const QString& path) {
    file_search_mode_ = false;
    current_file_query_.clear();
    active_file_context_key_ = (path.isEmpty() || path == QStringLiteral("/"))
        ? QStringLiteral("file:root")
        : QStringLiteral("path:%1").arg(path);
    setFileStatus(QStringLiteral("正在打开 %1 …").arg(path.isEmpty() ? QStringLiteral("/") : path));
    file_service_.listFiles(path.isEmpty() ? QStringLiteral("/") : path);
}

void MainWindow::openCurrentParentDirectory() {
    if (file_search_mode_) {
        file_search_mode_ = false;
        current_file_query_.clear();
        if (file_panel_widget_) {
            file_panel_widget_->clearSearch();
        }
        setFileStatus(QStringLiteral("返回当前目录…"));
        file_service_.listFiles(current_file_path_);
        return;
    }

    if (current_file_path_ == QStringLiteral("/") || current_file_path_.isEmpty()) {
        return;
    }

    const QString normalized = current_file_path_.endsWith('/')
        ? current_file_path_.left(current_file_path_.size() - 1)
        : current_file_path_;
    const int index = normalized.lastIndexOf('/');
    const QString parent = (index <= 0) ? QStringLiteral("/")
                                        : normalized.left(index);
    navigateToFilePath(parent);
}

void MainWindow::createDirectory() {
    const QString name = QInputDialog::getText(
        this,
        QStringLiteral("新建文件夹"),
        QStringLiteral("请输入新文件夹名称：")).trimmed();
    if (name.isEmpty()) {
        return;
    }
    setFileStatus(QStringLiteral("正在创建“%1”…").arg(name));
    file_service_.createDirectory(current_file_path_, name);
}

void MainWindow::renameSelectedFile() {
    const QString path = file_panel_widget_ ? file_panel_widget_->currentPath() : QString();
    if (path.isEmpty()) {
        return;
    }

    const QString current_name = file_panel_widget_ ? file_panel_widget_->currentName() : QString();
    const QString new_name = QInputDialog::getText(
        this,
        QStringLiteral("重命名"),
        QStringLiteral("请输入新名称："),
        QLineEdit::Normal,
        current_name).trimmed();
    if (new_name.isEmpty() || new_name == current_name) {
        return;
    }
    setFileStatus(QStringLiteral("正在将“%1”重命名为“%2”…").arg(current_name, new_name));
    file_service_.renamePath(path, new_name);
}

void MainWindow::moveSelectedFile() {
    const QString path = file_panel_widget_ ? file_panel_widget_->currentPath() : QString();
    if (path.isEmpty()) {
        return;
    }

    const QString destination = QInputDialog::getText(
        this,
        QStringLiteral("移动"),
        QStringLiteral("请输入目标目录路径（例如 /archive）："),
        QLineEdit::Normal,
        current_file_path_).trimmed();
    if (destination.isEmpty()) {
        return;
    }
    setFileStatus(QStringLiteral("正在移动到 %1 …").arg(destination));
    file_service_.movePath(path, destination);
}

void MainWindow::deleteSelectedFile() {
    const QString path = file_panel_widget_ ? file_panel_widget_->currentPath() : QString();
    if (path.isEmpty()) {
        return;
    }

    const QString current_name = file_panel_widget_ ? file_panel_widget_->currentName() : QString();
    const auto reply = QMessageBox::question(
        this,
        QStringLiteral("删除"),
        QStringLiteral("确认删除“%1”吗？此操作不可恢复。").arg(current_name),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    setFileStatus(QStringLiteral("正在删除“%1”…").arg(current_name));
    file_service_.deletePath(path);
}

void MainWindow::runFileSearch() {
    const QString keyword = file_panel_widget_ ? file_panel_widget_->searchText().trimmed()
                                               : QString();
    if (keyword.isEmpty()) {
        file_search_mode_ = false;
        current_file_query_.clear();
        active_file_context_key_ = QStringLiteral("file:current");
        setFileStatus(QStringLiteral("已清空搜索，返回当前目录。"));
        file_service_.listFiles(current_file_path_);
        return;
    }
    active_file_context_key_ = QStringLiteral("file:search");
    setFileStatus(QStringLiteral("正在搜索“%1”…").arg(keyword));
    file_service_.search(keyword);
}

void MainWindow::clearFileSearchIfNeeded(const QString& text) {
    if (!text.trimmed().isEmpty() || !file_search_mode_) {
        return;
    }
    file_search_mode_ = false;
    current_file_query_.clear();
    active_file_context_key_ = QStringLiteral("file:current");
    setFileStatus(QStringLiteral("搜索已清空，返回当前目录。"));
    file_service_.listFiles(current_file_path_);
}

QList<QPair<QString, QString>> MainWindow::buildFileBreadcrumbs() const {
    QList<QPair<QString, QString>> crumbs;
    crumbs.append({QStringLiteral("根目录"), QStringLiteral("/")});
    if (file_search_mode_) {
        if (!current_file_query_.trimmed().isEmpty()) {
            crumbs.append({QStringLiteral("搜索结果"), current_file_path_.isEmpty()
                ? QStringLiteral("/")
                : current_file_path_});
        }
        return crumbs;
    }

    const QString normalized = current_file_path_.isEmpty() ? QStringLiteral("/") : current_file_path_;
    const QStringList segments = normalized.split('/', Qt::SkipEmptyParts);
    QString cumulative = QStringLiteral("/");
    for (const QString& segment : segments) {
        cumulative = (cumulative == QStringLiteral("/"))
            ? QStringLiteral("/%1").arg(segment)
            : QStringLiteral("%1/%2").arg(cumulative, segment);
        crumbs.append({segment, cumulative});
    }
    return crumbs;
}

void MainWindow::syncFileContextChrome() {
    if (!file_panel_widget_) {
        return;
    }

    const QString path_title = file_search_mode_
        ? QStringLiteral("搜索结果")
        : (current_file_path_.isEmpty() ? QStringLiteral("/") : current_file_path_);
    const QString path_meta = file_search_mode_
        ? (current_file_query_.trimmed().isEmpty()
               ? QString()
               : QStringLiteral("关键词：%1").arg(current_file_query_))
        : QStringLiteral("%1 项").arg(current_file_entries_.size());
    file_panel_widget_->setContextSummary(path_title, path_meta);
    file_panel_widget_->setPathState(
        file_search_mode_
            ? QStringLiteral("搜索：%1").arg(current_file_query_)
            : current_file_path_,
        file_search_mode_ ? current_file_query_ : current_file_path_,
        file_search_mode_ || current_file_path_ != QStringLiteral("/"));
    file_panel_widget_->setBreadcrumbs(buildFileBreadcrumbs(),
                                       current_file_path_.isEmpty() ? QStringLiteral("/") : current_file_path_);
}

void MainWindow::refreshSidebarForCurrentMode() {
    if (active_main_view_ == MainView::Profile) {
        return;
    }
    const QString keyword = sidebar_widget_ ? sidebar_widget_->searchText() : QString();
    if (active_main_view_ == MainView::Contact) {
        filterContactList(keyword);
    } else if (active_main_view_ == MainView::File) {
        QList<SidebarEntry> entries;

        SidebarEntry section;
        section.type = SidebarItemType::Section;
        section.title = QStringLiteral("浏览");
        section.tag = file_search_mode_ ? QStringLiteral("搜索中") : QStringLiteral("目录");
        entries.append(section);

        SidebarEntry root_dir;
        root_dir.key = QStringLiteral("file:root");
        root_dir.title = QStringLiteral("根目录");
        root_dir.preview = QStringLiteral("/");
        root_dir.type = SidebarItemType::FileContext;
        root_dir.tag = current_file_path_ == QStringLiteral("/") ? QStringLiteral("当前") : QString();
        entries.append(root_dir);

        SidebarEntry current_dir;
        current_dir.key = QStringLiteral("file:current");
        current_dir.title = QStringLiteral("当前目录");
        current_dir.preview = file_search_mode_
            ? QStringLiteral("搜索结果：%1").arg(current_file_query_)
            : current_file_path_;
        current_dir.type = SidebarItemType::FileContext;
        entries.append(current_dir);

        if (!file_search_mode_) {
            QString normalized = current_file_path_.isEmpty() ? QStringLiteral("/") : current_file_path_;
            const QStringList segments = normalized.split('/', Qt::SkipEmptyParts);
            QString cumulative = QStringLiteral("/");
            for (const QString& segment : segments) {
                if (cumulative == QStringLiteral("/")) {
                    cumulative += segment;
                } else {
                    cumulative += QStringLiteral("/") + segment;
                }
                SidebarEntry path_item;
                path_item.key = QStringLiteral("path:%1").arg(cumulative);
                path_item.title = segment;
                path_item.preview = cumulative;
                path_item.type = SidebarItemType::FileContext;
                path_item.tag = cumulative == current_file_path_ ? QStringLiteral("当前") : QString();
                entries.append(path_item);
            }
        }

        if (file_search_mode_) {
            SidebarEntry search;
            search.key = QStringLiteral("file:search");
            search.title = QStringLiteral("搜索结果");
            search.preview = current_file_query_.isEmpty()
                ? QStringLiteral("无关键词")
                : QStringLiteral("关键词：%1").arg(current_file_query_);
            search.type = SidebarItemType::FileContext;
            search.tag = QStringLiteral("%1 项").arg(current_file_entries_.size());
            entries.append(search);
        }

        section = {};
        section.type = SidebarItemType::Section;
        section.title = QStringLiteral("工具");
        section.tag = QStringLiteral("快捷操作");
        entries.append(section);

        SidebarEntry upload;
        upload.key = QStringLiteral("file:upload");
        upload.title = QStringLiteral("上传文件");
        upload.preview = QStringLiteral("选择本地文件加入上传队列");
        upload.type = SidebarItemType::Action;
        upload.tag = QStringLiteral("GO");
        entries.append(upload);

        SidebarEntry transfer;
        transfer.key = QStringLiteral("file:transfer");
        transfer.title = QStringLiteral("传输状态");
        transfer.preview = file_service_.hasActiveTransfer()
            ? QStringLiteral("有任务正在执行")
            : QStringLiteral("当前没有活动传输");
        transfer.type = SidebarItemType::FileContext;
        transfer.tag = !pending_upload_paths_.isEmpty()
            ? QStringLiteral("%1").arg(pending_upload_paths_.size())
            : QString();
        entries.append(transfer);

        SidebarEntry recent_section;
        recent_section.type = SidebarItemType::Section;
        recent_section.title = QStringLiteral("最近传输");
        recent_section.tag = QStringLiteral("%1").arg(recent_transfers_.size());
        entries.append(recent_section);

        for (const auto& item : recent_transfers_) {
            SidebarEntry transfer_item;
            transfer_item.key = item.key;
            transfer_item.title = item.title;
            transfer_item.preview = item.detail;
            transfer_item.type = SidebarItemType::FileContext;
            transfer_item.tag = item.tag;
            entries.append(transfer_item);
        }

        if (sidebar_widget_) {
            QString selected_key = active_file_context_key_;
            if (selected_key.isEmpty()) {
                selected_key = file_search_mode_
                    ? QStringLiteral("file:search")
                    : QStringLiteral("file:current");
            }
            sidebar_widget_->populateEntries(entries, selected_key, true);
        }
    } else {
        filterFriendList(keyword);
    }
}

void MainWindow::switchMainTab(int index) {
    active_main_view_ = static_cast<MainView>(index);

    const bool show_sidebar = (active_main_view_ != MainView::Profile);
    sidebar_panel_->setVisible(show_sidebar);

    if (active_main_view_ == MainView::Message) {
        center_stack_->setCurrentWidget(chat_panel_widget_);
        if (sidebar_widget_) {
            sidebar_widget_->setMode(SidebarMode::Message);
            sidebar_widget_->setHeader(QStringLiteral("消息"),
                                       QStringLiteral("搜索会话…"),
                                       true);
        }
        content_splitter_->setSizes({260, std::max(520, width() - 312)});
        if (active_chat_peer_.isEmpty() && active_group_id_ == 0) {
            showChatEmptyState();
        }
    } else if (active_main_view_ == MainView::Contact) {
        center_stack_->setCurrentWidget(contact_panel_widget_);
        if (sidebar_widget_) {
            sidebar_widget_->setMode(SidebarMode::Contact);
            sidebar_widget_->setHeader(QStringLiteral("联系人"),
                                       QStringLiteral("搜索好友或群组…"),
                                       true);
        }
        content_splitter_->setSizes({260, std::max(560, width() - 312)});
        if (active_contact_friend_.isEmpty() && active_contact_group_id_ == 0) {
            showContactEmptyState();
        }
    } else if (active_main_view_ == MainView::File) {
        center_stack_->setCurrentWidget(file_panel_widget_);
        if (sidebar_widget_) {
            sidebar_widget_->setMode(SidebarMode::File);
            sidebar_widget_->setHeader(QStringLiteral("文件"),
                                       QStringLiteral("搜索文件模式上下文…"),
                                       false);
        }
        content_splitter_->setSizes({260, std::max(560, width() - 312)});
        syncFileContextChrome();
    } else {
        center_stack_->setCurrentWidget(profile_panel_widget_);
        if (sidebar_widget_) {
            sidebar_widget_->setHeader(QStringLiteral("我"),
                                       QStringLiteral(""),
                                       false);
        }
    }
    refreshIconBarActiveState();
    refreshSidebarForCurrentMode();

    if (active_main_view_ == MainView::Profile) {
        content_splitter_->setSizes({0, width()});
    }
}

void MainWindow::navigateToChatWithFriend(const QString& username) {
    if (username.isEmpty()) {
        return;
    }
    switchMainTab(static_cast<int>(MainView::Message));
    active_chat_peer_ = username;
    refreshSidebarForCurrentMode();
    showChatConversation(username, lookupFriendOnline(friends_, username));
    requestConversationSnapshot(username);
}

void MainWindow::navigateToChatWithGroup(int group_id) {
    if (group_id <= 0) {
        return;
    }
    switchMainTab(static_cast<int>(MainView::Message));
    showGroupConversation(group_id);
    refreshSidebarForCurrentMode();
}

void MainWindow::openOnlineUserDialog() {
    if (!sidebar_panel_->isVisible()) {
        return;
    }

    OnlineUserDialog dialog(friends_, this);
    connect(&dialog, &OnlineUserDialog::refreshRequested,
            this, [this] { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
            &dialog, &OnlineUserDialog::setUsers);
    connect(&dialog, &OnlineUserDialog::userChosen,
            this, [this](const QString& username) { friend_service_.addFriend(username); });
    dialog.exec();
}

void MainWindow::openGroupListDialog() {
    QList<GroupListEntry> groups;
    for (auto it = group_summaries_.cbegin(); it != group_summaries_.cend(); ++it) {
        GroupListEntry entry;
        entry.group_id = it.key();
        entry.name = it->name;
        entry.owner_id = it->owner_id;
        entry.online_count = it->online_count;
        entry.unread_count = it->unread_count;
        groups.append(entry);
    }
    GroupListDialog dialog(groups, this);
    connect(&group_service_, &cloudvault::GroupService::groupListReceived,
            &dialog, &GroupListDialog::setGroups);
    connect(&dialog, &GroupListDialog::groupChosen, this, [this](int group_id) {
        active_chat_peer_.clear();
        showGroupConversation(group_id);
    });
    connect(&dialog, &GroupListDialog::createRequested,
            this, [this](const QString& name) { group_service_.createGroup(name); });
    connect(&dialog, &GroupListDialog::leaveRequested,
            this, [this](int group_id) { group_service_.leaveGroup(group_id); });
    group_service_.getGroupList();
    dialog.exec();
}

void MainWindow::openShareFileDialog() {
    const QString path = file_panel_widget_ ? file_panel_widget_->currentPath() : QString();
    if (path.isEmpty() || (file_panel_widget_ ? file_panel_widget_->currentIsDir() : false)) {
        return;
    }

    const QString file_name = file_panel_widget_ ? file_panel_widget_->currentName() : QString();
    ShareFileDialog dialog(file_name, friends_, this);
    connect(&dialog, &ShareFileDialog::shareConfirmed,
            this, [this, path](const QString&, const QStringList& targets) {
                if (targets.isEmpty()) {
                    return;
                }
                setFileStatus(QStringLiteral("正在发送 %1 的分享请求…")
                                  .arg(QFileInfo(path).fileName()));
                share_service_.shareFile(path, targets);
            });
    dialog.exec();
}

void MainWindow::uploadFile() {
    const QStringList local_paths = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("选择要上传的文件"),
        QDir::homePath(),
        QStringLiteral("所有文件 (*.*)"));
    if (local_paths.isEmpty()) {
        return;
    }

    enqueueUploads(local_paths);
}

void MainWindow::downloadSelectedFile() {
    const QString remote_path = file_panel_widget_ ? file_panel_widget_->currentPath() : QString();
    if (remote_path.isEmpty() || (file_panel_widget_ ? file_panel_widget_->currentIsDir() : false)) {
        return;
    }

    const QString default_path = QDir::homePath() + QStringLiteral("/") + QFileInfo(remote_path).fileName();
    const QString save_path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存文件"),
        default_path);
    if (save_path.isEmpty()) {
        return;
    }

    showTransferRow(QStringLiteral("准备下载"), 0, true);
    setFileStatus(QStringLiteral("正在准备下载 %1 …").arg(QFileInfo(remote_path).fileName()));
    file_service_.downloadFile(remote_path, save_path);
}

void MainWindow::enqueueUploads(const QStringList& local_paths) {
    for (const QString& path : local_paths) {
        if (!path.trimmed().isEmpty()) {
            pending_upload_paths_.append(path);
        }
    }
    if (pending_upload_paths_.isEmpty()) {
        return;
    }

    showTransferRow(QStringLiteral("等待上传队列"), 0, true);
    startNextQueuedUpload();
}

void MainWindow::startNextQueuedUpload() {
    if (file_service_.hasActiveTransfer() || pending_upload_paths_.isEmpty()) {
        if (file_panel_widget_) {
            file_panel_widget_->setTransferCancelEnabled(!pending_upload_paths_.isEmpty());
        }
        return;
    }

    const QString next_path = pending_upload_paths_.takeFirst();
    const QString filename = QFileInfo(next_path).fileName();
    showTransferRow(QStringLiteral("准备上传 %1").arg(filename), 0, true);
    setFileStatus(QStringLiteral("正在准备上传 %1 …").arg(filename));
    if (file_panel_widget_) {
        file_panel_widget_->setTransferCancelEnabled(!pending_upload_paths_.isEmpty());
    }
    file_service_.uploadFile(next_path, current_file_path_);
}

void MainWindow::showTransferRow(const QString& title, int percent, bool cancellable) {
    if (file_panel_widget_) {
        file_panel_widget_->setTransferState(title, percent, cancellable);
    }
}

void MainWindow::hideTransferRow() {
    if (file_panel_widget_) {
        file_panel_widget_->clearTransferState();
    }
}

void MainWindow::refreshIconBarActiveState() {
    if (!message_tab_btn_ || !contact_tab_btn_ || !file_tab_btn_ || !avatar_tab_btn_) {
        return;
    }
    message_tab_btn_->setProperty("active", active_main_view_ == MainView::Message);
    contact_tab_btn_->setProperty("active", active_main_view_ == MainView::Contact);
    file_tab_btn_->setProperty("active", active_main_view_ == MainView::File);
    avatar_tab_btn_->setProperty("active", active_main_view_ == MainView::Profile);
    repolish(message_tab_btn_);
    repolish(contact_tab_btn_);
    repolish(file_tab_btn_);
    repolish(avatar_tab_btn_);
}

void MainWindow::upsertRecentTransfer(const QString& key,
                                      const QString& title,
                                      const QString& detail,
                                      const QString& tag) {
    if (key.isEmpty()) {
        return;
    }
    auto it = std::find_if(recent_transfers_.begin(),
                           recent_transfers_.end(),
                           [&key](const RecentTransferItem& item) { return item.key == key; });
    if (it != recent_transfers_.end()) {
        it->title = title;
        it->detail = detail;
        it->tag = tag;
        const RecentTransferItem updated = *it;
        recent_transfers_.erase(it);
        recent_transfers_.prepend(updated);
    } else {
        recent_transfers_.prepend({key, title, detail, tag});
    }
    while (recent_transfers_.size() > 5) {
        recent_transfers_.removeLast();
    }
    if (active_main_view_ == MainView::File) {
        refreshSidebarForCurrentMode();
        if (active_file_context_key_ == key && (!file_panel_widget_ || !file_panel_widget_->hasSelection())) {
            syncFileContextChrome();
        }
    }
}

void MainWindow::showConnectionBanner(const QString& message) {
    if (!connection_banner_ || !connection_banner_label_) {
        return;
    }
    connection_banner_label_->setText(message);
    connection_banner_->show();
}

void MainWindow::hideConnectionBanner() {
    if (connection_banner_) {
        connection_banner_->hide();
    }
}

void MainWindow::loadProfileDraft() {
    QSettings settings;
    settings.beginGroup(QStringLiteral("profile/%1").arg(current_username_));
    const QString nickname = settings.value(QStringLiteral("nickname"), current_username_).toString();
    const QString signature = settings.value(QStringLiteral("signature")).toString();
    settings.endGroup();

    if (profile_panel_widget_) {
        profile_panel_widget_->setDraftValues(nickname, signature);
        profile_panel_widget_->setDisplayName(
            nickname.trimmed().isEmpty() ? current_username_ : nickname.trimmed());
    }
}

void MainWindow::saveProfileDraft() {
    const QString nickname = profile_panel_widget_ ? profile_panel_widget_->nicknameText().trimmed()
                                                   : QString();
    const QString signature = profile_panel_widget_ ? profile_panel_widget_->signatureText().trimmed()
                                                    : QString();

    QSettings settings;
    settings.beginGroup(QStringLiteral("profile/%1").arg(current_username_));
    settings.setValue(QStringLiteral("nickname"), nickname);
    settings.setValue(QStringLiteral("signature"), signature);
    settings.endGroup();

    if (profile_panel_widget_) {
        profile_panel_widget_->setDisplayName(nickname.isEmpty() ? current_username_ : nickname);
    }
    auth_service_.updateProfile(nickname, signature);
}

void MainWindow::refreshGroupList(const QList<GroupListEntry>& groups) {
    QHash<int, int> unread_snapshot;
    for (auto it = group_summaries_.cbegin(); it != group_summaries_.cend(); ++it) {
        unread_snapshot.insert(it.key(), it->unread_count);
    }

    group_summaries_.clear();
    for (const auto& group : groups) {
        GroupSummary summary;
        summary.name = group.name;
        summary.owner_id = group.owner_id;
        summary.online_count = group.online_count;
        summary.unread_count = unread_snapshot.value(group.group_id, 0);
        group_summaries_.insert(group.group_id, summary);
    }

    if (active_group_id_ > 0 && !group_summaries_.contains(active_group_id_)) {
        showChatEmptyState();
    } else if (active_group_id_ > 0) {
        showGroupConversation(active_group_id_);
    }
    if (active_contact_group_id_ > 0 && !group_summaries_.contains(active_contact_group_id_)) {
        showContactEmptyState();
    } else if (active_contact_group_id_ > 0) {
        showGroupDetails(active_contact_group_id_);
    }
    refreshSidebarForCurrentMode();
}

void MainWindow::showGroupConversation(int group_id) {
    const auto it = group_summaries_.find(group_id);
    if (it == group_summaries_.end()) {
        return;
    }

    active_chat_peer_.clear();
    active_group_id_ = group_id;
    active_group_name_ = it->name;
    it->unread_count = 0;
    if (chat_panel_widget_) {
        chat_panel_widget_->showConversation(it->name,
                                             QStringLiteral("%1 人在线").arg(it->online_count),
                                             true);
        chat_panel_widget_->rebuildMessages(group_messages_.value(group_id), current_username_);
    }
    chat_service_.loadGroupHistory(group_id);
    refreshSidebarForCurrentMode();
}
