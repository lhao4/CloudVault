// =============================================================
// client/src/ui/main_window.cpp
// 主窗口（消息 / 文件 / 我）
// =============================================================

#include "main_window.h"

#include "ui/chat_panel.h"
#include "ui/detail_panel.h"
#include "ui/file_panel.h"
#include "ui/group_list_dialog.h"
#include "ui/online_user_dialog.h"
#include "ui/profile_panel.h"
#include "ui/sidebar_panel.h"
#include "ui/share_file_dialog.h"

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

namespace {

QString formatPresence(bool online) {
    return online ? QStringLiteral("在线") : QStringLiteral("离线");
}

int avatarVariantForSeed(const QString& seed) {
    return static_cast<int>(qHash(seed)) % 6;
}

void repolish(QWidget* widget) {
    if (!widget) {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void applyShadow(QWidget* widget, int blur = 40, int offset_y = 8, int alpha = 30) {
    auto* effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(blur);
    effect->setOffset(0, offset_y);
    effect->setColor(QColor(17, 24, 39, alpha));
    widget->setGraphicsEffect(effect);
}

void prepareAvatarBadge(QLabel* label, const QString& seed, int size) {
    if (!label) {
        return;
    }
    label->setObjectName(QStringLiteral("avatarBadge"));
    label->setProperty("variant", avatarVariantForSeed(seed));
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(size, size);
    label->setText(seed.left(1).toUpper());
    label->setStyleSheet(QStringLiteral("border-radius:%1px;").arg(size / 2));
    repolish(label);
}

QWidget* createAvatarWidget(const QString& seed,
                            int size,
                            bool online,
                            QWidget* parent = nullptr) {
    auto* wrap = new QWidget(parent);
    wrap->setFixedSize(size, size);

    auto* avatar = new QLabel(wrap);
    prepareAvatarBadge(avatar, seed, size);
    avatar->move(0, 0);

    if (size >= 34) {
        auto* dot = new QLabel(wrap);
        dot->setObjectName(QStringLiteral("onlineDot"));
        dot->setFixedSize(10, 10);
        dot->move(size - 12, size - 12);
        dot->setVisible(online);
    }

    return wrap;
}

QLabel* createStandardIconLabel(QStyle::StandardPixmap icon_type,
                                int size,
                                const QString& object_name,
                                QWidget* parent = nullptr) {
    auto* label = new QLabel(parent);
    label->setObjectName(object_name);
    label->setFixedSize(size, size);
    label->setAlignment(Qt::AlignCenter);
    if (parent) {
        label->setPixmap(parent->style()->standardIcon(icon_type).pixmap(size, size));
    }
    return label;
}

QPushButton* createIconButton(const QString& text,
                              const QString& tooltip,
                              int size,
                              QWidget* parent = nullptr) {
    auto* button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("iconBtn"));
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(size, size);
    button->setToolTip(tooltip);
    return button;
}

QPushButton* createNavButton(const QString& text, QWidget* parent = nullptr) {
    auto* button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("navBtn"));
    button->setCursor(Qt::PointingHandCursor);
    button->setProperty("active", false);
    return button;
}

QString formatConversationPreview(const QString& content, bool outgoing) {
    QString simplified = content.simplified();
    if (simplified.isEmpty()) {
        simplified = QStringLiteral("[空消息]");
    }

    const QString prefix = outgoing ? QStringLiteral("我: ")
                                    : QStringLiteral("对方: ");
    const int max_length = 24 - prefix.size();
    if (simplified.size() > max_length) {
        simplified = simplified.left(std::max(8, max_length)) + QStringLiteral("...");
    }
    return prefix + simplified;
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

QString formatFileSize(quint64 size) {
    static const char* units[] = {"B", "KB", "MB", "GB"};
    double value = static_cast<double>(size);
    int unit_index = 0;
    while (value >= 1024.0 && unit_index < 3) {
        value /= 1024.0;
        ++unit_index;
    }
    if (unit_index == 0) {
        return QStringLiteral("%1 %2").arg(static_cast<qulonglong>(size)).arg(units[unit_index]);
    }
    return QStringLiteral("%1 %2")
        .arg(QString::number(value, 'f', value >= 10.0 ? 1 : 2))
        .arg(units[unit_index]);
}

} // namespace

MainWindow::MainWindow(const QString& username,
                       cloudvault::ChatService& chat_service,
                       cloudvault::FriendService& friend_service,
                       cloudvault::FileService& file_service,
                       cloudvault::ShareService& share_service,
                       QWidget* parent)
    : QMainWindow(parent),
      current_username_(username),
      chat_service_(chat_service),
      friend_service_(friend_service),
      file_service_(file_service),
      share_service_(share_service) {
    setupUi();
    connectSignals();
    loadProfileDraft();
    friend_service_.flushFriends();
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
    resize(900, 580);
    setMinimumSize(900, 580);

    auto* shell = new QWidget(this);
    shell->setObjectName(QStringLiteral("mainShell"));
    auto* shell_layout = new QVBoxLayout(shell);
    shell_layout->setContentsMargins(0, 0, 0, 0);
    shell_layout->setSpacing(0);

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
        file_panel_widget_ = new FilePanel(center_stack_);
        center_stack_->addWidget(file_panel_widget_);
    }

    {
        profile_panel_widget_ = new ProfilePanel(current_username_, center_stack_);
        center_stack_->addWidget(profile_panel_widget_);
    }

    detail_panel_widget_ = new DetailPanel(current_username_, content_splitter_);
    detail_panel_ = detail_panel_widget_;

    content_splitter_->addWidget(sidebar_widget_);
    content_splitter_->addWidget(center_panel);
    content_splitter_->addWidget(detail_panel_widget_);
    content_splitter_->setStretchFactor(1, 1);
    content_splitter_->setSizes({260, 420, 220});

    shell_layout->addWidget(content_root_, 1);

    event_log_panel_ = new QFrame(shell);
    event_log_panel_->setObjectName(QStringLiteral("eventLogPanel"));
    auto* event_log_layout = new QVBoxLayout(event_log_panel_);
    event_log_layout->setContentsMargins(0, 0, 0, 0);
    event_log_layout->setSpacing(0);

    auto* event_log_header = new QFrame(event_log_panel_);
    event_log_header->setObjectName(QStringLiteral("eventLogHeader"));
    event_log_header->setFixedHeight(36);
    auto* event_log_header_layout = new QHBoxLayout(event_log_header);
    event_log_header_layout->setContentsMargins(16, 0, 12, 0);
    event_log_header_layout->setSpacing(8);

    auto* event_log_title = new QLabel(QStringLiteral("● 事件日志"), event_log_header);
    event_log_title->setObjectName(QStringLiteral("eventLogTitle"));
    event_log_header_layout->addWidget(event_log_title);
    event_log_header_layout->addStretch();

    event_log_toggle_btn_ = new QToolButton(event_log_header);
    event_log_toggle_btn_->setObjectName(QStringLiteral("eventLogToggleBtn"));
    event_log_toggle_btn_->setText(QStringLiteral("折叠 ▲"));
    event_log_toggle_btn_->setCursor(Qt::PointingHandCursor);
    event_log_header_layout->addWidget(event_log_toggle_btn_);
    event_log_layout->addWidget(event_log_header);

    event_log_list_ = new QListWidget(event_log_panel_);
    event_log_list_->setObjectName(QStringLiteral("eventLogList"));
    event_log_list_->setSelectionMode(QAbstractItemView::NoSelection);
    event_log_list_->setFocusPolicy(Qt::NoFocus);
    event_log_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    event_log_list_->setUniformItemSizes(true);
    event_log_list_->setFixedHeight(108);
    event_log_layout->addWidget(event_log_list_);
    shell_layout->addWidget(event_log_panel_);

    auto* nav_bar = new QFrame(shell);
    nav_bar->setObjectName(QStringLiteral("navBar"));
    nav_bar->setFixedHeight(52);
    auto* nav_layout = new QHBoxLayout(nav_bar);
    nav_layout->setContentsMargins(0, 0, 0, 0);
    nav_layout->setSpacing(0);
    message_tab_btn_ = createNavButton(QStringLiteral("消息"), nav_bar);
    file_tab_btn_ = createNavButton(QStringLiteral("文件"), nav_bar);
    profile_tab_btn_ = createNavButton(QStringLiteral("我"), nav_bar);
    nav_layout->addWidget(message_tab_btn_);
    nav_layout->addWidget(file_tab_btn_);
    nav_layout->addWidget(profile_tab_btn_);
    shell_layout->addWidget(nav_bar);

    setCentralWidget(shell);
    showChatEmptyState();
    hideTransferRow();
    resetFileActionSummary();
    appendEventLog(QStringLiteral("已打开客户端，等待连接服务器"), QStringLiteral("●"));
}

void MainWindow::connectSignals() {
    connect(message_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(0); });
    connect(file_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(1); });
    connect(profile_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(2); });
    connect(event_log_toggle_btn_, &QToolButton::clicked, this, [this] {
        event_log_expanded_ = !event_log_expanded_;
        event_log_list_->setVisible(event_log_expanded_);
        event_log_toggle_btn_->setText(event_log_expanded_
            ? QStringLiteral("折叠 ▲")
            : QStringLiteral("展开 ▼"));
    });

    connect(chat_panel_widget_, &ChatPanel::sendRequested,
            this, &MainWindow::sendCurrentMessage);
    connect(chat_panel_widget_, &ChatPanel::groupListRequested,
            this, &MainWindow::openGroupListDialog);
    connect(sidebar_widget_, &SidebarPanel::searchTextChanged,
            this, &MainWindow::filterFriendList);
    connect(sidebar_widget_, &SidebarPanel::contactSelectionChanged,
            this, &MainWindow::applySelectedFriend);
    connect(sidebar_widget_, &SidebarPanel::actionRequested,
            this, &MainWindow::openOnlineUserDialog);

    connect(profile_panel_widget_, &ProfilePanel::saveRequested,
            this, &MainWindow::saveProfileDraft);
    connect(profile_panel_widget_, &ProfilePanel::cancelRequested,
            this, &MainWindow::loadProfileDraft);
    connect(profile_panel_widget_, &ProfilePanel::logoutRequested,
            this, &MainWindow::logoutRequested);
    connect(profile_panel_widget_, &ProfilePanel::logoutRequested,
            this, [this] { appendEventLog(QStringLiteral("用户请求退出登录"), QStringLiteral("→")); });

    connect(file_panel_widget_, &FilePanel::backRequested, this, &MainWindow::openCurrentParentDirectory);
    connect(file_panel_widget_, &FilePanel::uploadRequested, this, &MainWindow::uploadFile);
    connect(file_panel_widget_, &FilePanel::refreshRequested,
            this, [this] {
                if (file_search_mode_ && !current_file_query_.isEmpty()) {
                    setFileStatus(QStringLiteral("正在刷新搜索结果…"));
                    appendEventLog(QStringLiteral("刷新当前搜索结果"), QStringLiteral("→"));
                    file_service_.search(current_file_query_);
                } else {
                    setFileStatus(QStringLiteral("正在刷新当前目录…"));
                    appendEventLog(QStringLiteral("刷新文件列表 %1").arg(current_file_path_), QStringLiteral("→"));
                    file_service_.listFiles(current_file_path_);
                }
            });
    connect(file_panel_widget_, &FilePanel::createRequested, this, &MainWindow::createDirectory);
    connect(file_panel_widget_, &FilePanel::searchRequested, this, &MainWindow::runFileSearch);
    connect(file_panel_widget_, &FilePanel::searchTextChanged, this, &MainWindow::clearFileSearchIfNeeded);
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
                appendEventLog(QStringLiteral("%1 %2: %3")
                                   .arg(message.from == current_username_ ? QStringLiteral("发送给")
                                                                          : QStringLiteral("收到来自"))
                                   .arg(peer)
                                   .arg(message.content.left(24)),
                               message.from == current_username_ ? QStringLiteral("→")
                                                                 : QStringLiteral("●"));
                if (peer == active_chat_peer_) {
                    if (chat_panel_widget_) {
                        chat_panel_widget_->appendMessage(message, current_username_);
                    }
                    showChatConversation(peer, sidebar_widget_ ? sidebar_widget_->currentOnline() : false);
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
                showChatConversation(peer, sidebar_widget_ ? sidebar_widget_->currentOnline() : false);
            });
    connect(&chat_service_, &cloudvault::ChatService::historyLoadFailed,
            this, [this](const QString& peer, const QString& reason) {
                pending_history_requests_.remove(peer);
                if (peer != active_chat_peer_) {
                    return;
                }
                showChatConversation(peer, sidebar_widget_ ? sidebar_widget_->currentOnline() : false);
                if (chat_panel_widget_) {
                    chat_panel_widget_->setConversationStatus(reason);
                }
            });

    connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
            this, &MainWindow::refreshFriendList);
    connect(&friend_service_, &cloudvault::FriendService::friendAdded,
            this, [this](const QString&) { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendDeleted,
            this, [this](const QString&) { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendRequestSent,
            this, [this](const QString& target) {
                QMessageBox::information(this,
                                         QStringLiteral("好友申请"),
                                         QStringLiteral("已向 %1 发送好友申请。").arg(target));
            });
    connect(&friend_service_, &cloudvault::FriendService::friendAddFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("添加失败"), reason);
            });
    connect(&friend_service_, &cloudvault::FriendService::incomingFriendRequest,
            this, [this](const QString& from) {
                const auto reply = QMessageBox::question(
                    this,
                    QStringLiteral("好友申请"),
                    QStringLiteral("%1 想添加你为好友，是否同意？").arg(from),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    friend_service_.agreeFriend(from);
                }
            });

    connect(&file_service_, &cloudvault::FileService::filesListed,
            this, &MainWindow::refreshFileList);
    connect(&file_service_, &cloudvault::FileService::fileListFailed,
            this, [this](const QString& reason) { setFileStatus(reason, true); });
    connect(&file_service_, &cloudvault::FileService::searchCompleted,
            this, [this](const QString& keyword, const cloudvault::FileEntries& entries) {
                file_search_mode_ = true;
                current_file_query_ = keyword;
                refreshFileList(QStringLiteral("搜索：%1").arg(keyword), entries);
            });
    connect(&file_service_, &cloudvault::FileService::searchFailed,
            this, [this](const QString& reason) { setFileStatus(reason, true); });
    connect(&file_service_, &cloudvault::FileService::fileOperationSucceeded,
            this, [this](const QString& message) {
                setFileStatus(message);
                appendEventLog(message, QStringLiteral("✓"));
                file_search_mode_ = false;
                current_file_query_.clear();
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
                if (percent == 0) {
                    appendEventLog(QStringLiteral("开始上传文件 %1").arg(filename), QStringLiteral("↑"));
                }
            });
    connect(&file_service_, &cloudvault::FileService::uploadFinished,
            this, [this](const QString& remote_path, const QString& message) {
                showTransferRow(QStringLiteral("上传完成"), 100, !pending_upload_paths_.isEmpty());
                setFileStatus(QStringLiteral("%1 · %2").arg(message, remote_path));
                appendEventLog(QStringLiteral("文件上传完成 %1").arg(QFileInfo(remote_path).fileName()),
                               QStringLiteral("✓"));
                file_search_mode_ = false;
                current_file_query_.clear();
                if (file_panel_widget_) {
                    file_panel_widget_->clearSearch();
                }
                file_service_.listFiles(current_file_path_);
                QTimer::singleShot(0, this, &MainWindow::startNextQueuedUpload);
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
            });
    connect(&file_service_, &cloudvault::FileService::downloadFinished,
            this, [this](const QString& local_path, const QString& message) {
                showTransferRow(QStringLiteral("下载完成"), 100, false);
                setFileStatus(QStringLiteral("%1 · %2").arg(message, local_path));
                appendEventLog(QStringLiteral("文件下载完成 %1").arg(QFileInfo(local_path).fileName()),
                               QStringLiteral("✓"));
                QTimer::singleShot(1500, this, &MainWindow::hideTransferRow);
                QMessageBox::information(this, QStringLiteral("下载完成"), local_path);
            });
    connect(&file_service_, &cloudvault::FileService::downloadFailed,
            this, [this](const QString& message) {
                showTransferRow(QStringLiteral("下载失败"), 0, false);
                setFileStatus(message, true);
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
        if (center_stack_->currentIndex() == 0) {
            friend_service_.flushFriends();
            if (!active_chat_peer_.isEmpty()) {
                requestConversationSnapshot(active_chat_peer_);
            }
            appendEventLog(QStringLiteral("快捷键刷新消息面板"), QStringLiteral("→"));
        } else if (center_stack_->currentIndex() == 1) {
            if (file_search_mode_ && !current_file_query_.isEmpty()) {
                file_service_.search(current_file_query_);
            } else {
                file_service_.listFiles(current_file_path_);
            }
            appendEventLog(QStringLiteral("快捷键刷新文件面板"), QStringLiteral("→"));
        }
    });

    auto* delete_shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(delete_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1 && file_panel_widget_ && file_panel_widget_->hasSelection()) {
            deleteSelectedFile();
        }
    });

    auto* rename_shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    connect(rename_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1 && file_panel_widget_ && file_panel_widget_->hasSelection()) {
            renameSelectedFile();
        }
    });

    auto* back_shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
    connect(back_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1) {
            openCurrentParentDirectory();
        }
    });

    auto* focus_search_shortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+F")), this);
    connect(focus_search_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1) {
            if (file_panel_widget_) {
                file_panel_widget_->focusSearchSelectAll();
            }
        }
    });

    auto* upload_shortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+U")), this);
    connect(upload_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1) {
            uploadFile();
        }
    });
}

void MainWindow::showChatEmptyState() {
    active_chat_peer_.clear();
    active_group_name_.clear();
    if (chat_panel_widget_) {
        chat_panel_widget_->showEmptyState();
    }
    if (detail_panel_widget_) {
        detail_panel_widget_->showEmptyState(current_username_);
    }
}

void MainWindow::showChatConversation(const QString& username, bool online) {
    active_group_name_.clear();
    if (chat_panel_widget_) {
        chat_panel_widget_->showConversationHeader(username, formatPresence(online));
    }
    if (detail_panel_widget_) {
        detail_panel_widget_->showContact(username, online);
    }

    const auto summary_it = conversation_summaries_.constFind(username);
    if (summary_it != conversation_summaries_.constEnd() && summary_it->has_message) {
        if (detail_panel_widget_) {
            detail_panel_widget_->addSharedSummary(QStringLiteral("最近会话"),
                                                   summary_it->preview);
        }
    } else {
        if (detail_panel_widget_) {
            detail_panel_widget_->showSharedEmptyMessage(
                QStringLiteral("暂无与 %1 的共享文件").arg(username));
        }
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
    filterFriendList(sidebar_widget_ ? sidebar_widget_->searchText() : QString());
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
    filterFriendList(sidebar_widget_ ? sidebar_widget_->searchText() : QString());
}

void MainWindow::clearConversationUnread(const QString& peer) {
    auto it = conversation_summaries_.find(peer);
    if (it == conversation_summaries_.end() || it->unread_count == 0) {
        return;
    }
    it->unread_count = 0;
    filterFriendList(sidebar_widget_ ? sidebar_widget_->searchText() : QString());
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

void MainWindow::refreshFriendList(const QList<QPair<QString, bool>>& friends) {
    friends_ = friends;
    QSet<QString> current_peers;
    for (const auto& [username, online] : friends_) {
        Q_UNUSED(online);
        current_peers.insert(username);
        requestConversationSnapshot(username);
    }

    const auto known_peers = conversation_summaries_.keys();
    for (const auto& peer : known_peers) {
        if (!current_peers.contains(peer)) {
            conversation_summaries_.remove(peer);
            pending_history_requests_.remove(peer);
        }
    }
    filterFriendList(sidebar_widget_ ? sidebar_widget_->searchText() : QString());
}

void MainWindow::filterFriendList(const QString& keyword) {
    const QString current = !active_chat_peer_.isEmpty()
        ? active_chat_peer_
        : (sidebar_widget_ ? sidebar_widget_->currentUsername() : QString());
    const QString trimmed = keyword.trimmed();
    QList<QPair<QString, bool>> ordered_friends = friends_;
    std::sort(ordered_friends.begin(), ordered_friends.end(),
              [this](const QPair<QString, bool>& lhs, const QPair<QString, bool>& rhs) {
                  const auto lhs_summary = conversation_summaries_.constFind(lhs.first);
                  const auto rhs_summary = conversation_summaries_.constFind(rhs.first);
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

                  if (lhs.second != rhs.second) {
                      return lhs.second;
                  }
                  return QString::localeAwareCompare(lhs.first, rhs.first) < 0;
              });

    {
        QList<SidebarContactEntry> visible_contacts;

        for (const auto& [username, online] : ordered_friends) {
            if (!trimmed.isEmpty() && !username.contains(trimmed, Qt::CaseInsensitive)) {
                continue;
            }

            const auto summary_it = conversation_summaries_.constFind(username);
            const bool has_message = summary_it != conversation_summaries_.constEnd()
                                  && summary_it->has_message;
            SidebarContactEntry entry;
            entry.username = username;
            entry.preview = has_message ? summary_it->preview : QStringLiteral("暂无消息");
            entry.time = has_message ? formatConversationTime(summary_it->timestamp) : QString();
            entry.unread = has_message ? summary_it->unread_count : 0;
            entry.online = online;
            visible_contacts.append(entry);
        }

        if (sidebar_widget_) {
            sidebar_widget_->populateContacts(visible_contacts,
                                             current,
                                             current.isEmpty());
        }
    }

    if (!sidebar_widget_ || sidebar_widget_->contactCount() == 0) {
        showChatEmptyState();
    }

    updateContactSelectionState();

    if ((sidebar_widget_ ? sidebar_widget_->currentUsername() : QString()) != current) {
        applySelectedFriend();
    }
}

void MainWindow::updateContactSelectionState() {
    if (sidebar_widget_) {
        sidebar_widget_->refreshSelectionHighlights();
    }
}

void MainWindow::applySelectedFriend() {
    updateContactSelectionState();

    const QString username = sidebar_widget_ ? sidebar_widget_->currentUsername() : QString();
    if (username.isEmpty()) {
        showChatEmptyState();
        return;
    }

    const bool online = sidebar_widget_ ? sidebar_widget_->currentOnline() : false;
    active_chat_peer_ = username;
    clearConversationUnread(username);
    showChatConversation(username, online);
    if (chat_panel_widget_) {
        chat_panel_widget_->setConversationStatus(QStringLiteral("正在加载历史消息…"));
    }
    requestConversationSnapshot(username);
}

void MainWindow::sendCurrentMessage() {
    if (active_chat_peer_.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法发送"),
                             QStringLiteral("请先选择一个联系人。"));
        return;
    }

    const QString content = chat_panel_widget_ ? chat_panel_widget_->inputText().trimmed()
                                               : QString();
    if (content.isEmpty()) {
        return;
    }

    chat_service_.sendMessage(active_chat_peer_, content);
    const cloudvault::ChatMessage local_message{
        current_username_,
        active_chat_peer_,
        content,
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
    };
    if (chat_panel_widget_) {
        chat_panel_widget_->appendMessage(local_message, current_username_);
    }
    applyConversationMessage(local_message, false);
    if (chat_panel_widget_) {
        chat_panel_widget_->clearInput();
    }
    showChatConversation(active_chat_peer_, sidebar_widget_ ? sidebar_widget_->currentOnline() : false);
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
        file_panel_widget_->setPathState(
            file_search_mode_
                ? QStringLiteral("搜索：%1").arg(current_file_query_)
                : current_file_path_,
            file_search_mode_ ? current_file_query_ : current_file_path_,
            file_search_mode_ || current_file_path_ != QStringLiteral("/"));
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
        return;
    }

    const bool is_dir = file_panel_widget_ ? file_panel_widget_->currentIsDir() : false;
    const QString name = file_panel_widget_ ? file_panel_widget_->currentName() : QString();
    if (file_panel_widget_) {
        file_panel_widget_->setSelectionSummary(
            name,
            is_dir ? QStringLiteral("目录 · 双击可进入")
                   : QStringLiteral("文件 · 可下载 / 分享 / 重命名 / 移动 / 删除"),
            path);
    }
}

void MainWindow::navigateToFilePath(const QString& path) {
    file_search_mode_ = false;
    current_file_query_.clear();
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
        setFileStatus(QStringLiteral("已清空搜索，返回当前目录。"));
        file_service_.listFiles(current_file_path_);
        return;
    }
    setFileStatus(QStringLiteral("正在搜索“%1”…").arg(keyword));
    file_service_.search(keyword);
}

void MainWindow::clearFileSearchIfNeeded(const QString& text) {
    if (!text.trimmed().isEmpty() || !file_search_mode_) {
        return;
    }
    file_search_mode_ = false;
    current_file_query_.clear();
    setFileStatus(QStringLiteral("搜索已清空，返回当前目录。"));
    file_service_.listFiles(current_file_path_);
}

void MainWindow::switchMainTab(int index) {
    center_stack_->setCurrentIndex(index);

    const bool show_sidebar = (index != 2);
    const bool show_detail = (index == 0);
    sidebar_panel_->setVisible(show_sidebar);
    detail_panel_->setVisible(show_detail);

    if (index == 0) {
        if (sidebar_widget_) {
            sidebar_widget_->setHeader(QStringLiteral("消息"),
                                       QStringLiteral("搜索联系人…"),
                                       true);
        }
    } else if (index == 1) {
        if (sidebar_widget_) {
            sidebar_widget_->setHeader(QStringLiteral("文件"),
                                       QStringLiteral("搜索联系人…"),
                                       false);
        }
    } else {
        if (sidebar_widget_) {
            sidebar_widget_->setHeader(QStringLiteral("我"),
                                       QStringLiteral("搜索联系人…"),
                                       false);
        }
    }

    for (int i = 0; i < 3; ++i) {
        QPushButton* button = (i == 0) ? message_tab_btn_
                              : (i == 1) ? file_tab_btn_
                                         : profile_tab_btn_;
        button->setProperty("active", i == index);
        repolish(button);
    }

    if (index == 0) {
        content_splitter_->setSizes({260, std::max(420, width() - 480), 220});
    } else if (index == 1) {
        content_splitter_->setSizes({260, std::max(500, width() - 260), 0});
    } else {
        content_splitter_->setSizes({0, width(), 0});
    }
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
    const QList<GroupListEntry> groups;
    GroupListDialog dialog(groups, this);
    connect(&dialog, &GroupListDialog::groupChosen, this, [this](const QString& group_name) {
        active_chat_peer_.clear();
        active_group_name_ = group_name;
        if (chat_panel_widget_) {
            chat_panel_widget_->showGroupPlaceholder(group_name);
        }
        appendEventLog(QStringLiteral("进入群聊 %1").arg(group_name), QStringLiteral("→"));
    });
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

void MainWindow::appendEventLog(const QString& message, const QString& icon) {
    if (!event_log_list_) {
        return;
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    auto* item = new QListWidgetItem(
        QStringLiteral("[%1] %2 %3").arg(timestamp, icon, message),
        event_log_list_);
    item->setToolTip(message);
    while (event_log_list_->count() > 50) {
        delete event_log_list_->takeItem(0);
    }
    event_log_list_->scrollToBottom();
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
}
