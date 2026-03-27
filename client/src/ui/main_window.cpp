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

void allowViewToHandleMouseEvents(QWidget* widget) {
    if (!widget) {
        return;
    }
    widget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    const auto children = widget->findChildren<QWidget*>();
    for (QWidget* child : children) {
        child->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }
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

QWidget* createContactItemWidget(const QString& username,
                                 const QString& preview,
                                 const QString& time,
                                 int unread,
                                 bool online,
                                 bool selected,
                                 QWidget* parent = nullptr) {
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("contactCard"));
    frame->setProperty("selected", selected);

    auto* root = new QHBoxLayout(frame);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* accent = new QFrame(frame);
    accent->setObjectName(QStringLiteral("contactAccent"));
    accent->setProperty("selected", selected);
    accent->setFixedWidth(2);
    root->addWidget(accent);

    auto* body = new QWidget(frame);
    auto* body_layout = new QHBoxLayout(body);
    body_layout->setContentsMargins(10, 0, 12, 0);
    body_layout->setSpacing(10);

    body_layout->addWidget(createAvatarWidget(username, 40, online, body), 0, Qt::AlignVCenter);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 10, 0, 10);
    text_layout->setSpacing(2);

    auto* name_label = new QLabel(username, body);
    name_label->setObjectName(QStringLiteral("contactNameLabel"));
    text_layout->addWidget(name_label);

    auto* preview_label = new QLabel(preview, body);
    preview_label->setObjectName(QStringLiteral("contactPreviewLabel"));
    preview_label->setWordWrap(false);
    text_layout->addWidget(preview_label);
    body_layout->addLayout(text_layout, 1);

    auto* meta_layout = new QVBoxLayout();
    meta_layout->setContentsMargins(0, 10, 0, 10);
    meta_layout->setSpacing(6);

    auto* time_label = new QLabel(time, body);
    time_label->setObjectName(QStringLiteral("contactTimeLabel"));
    time_label->setAlignment(Qt::AlignRight);
    meta_layout->addWidget(time_label);

    if (unread > 0) {
        auto* badge = new QLabel(QString::number(unread), body);
        badge->setObjectName(QStringLiteral("contactBadgeLabel"));
        badge->setAlignment(Qt::AlignCenter);
        meta_layout->addWidget(badge, 0, Qt::AlignRight);
    } else {
        meta_layout->addStretch();
    }

    body_layout->addLayout(meta_layout);
    root->addWidget(body, 1);
    allowViewToHandleMouseEvents(frame);
    return frame;
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

    sidebar_panel_ = new QFrame(content_splitter_);
    sidebar_panel_->setObjectName(QStringLiteral("sidebarPanel"));
    sidebar_panel_->setMinimumWidth(260);
    sidebar_panel_->setMaximumWidth(260);
    auto* sidebar_layout = new QVBoxLayout(sidebar_panel_);
    sidebar_layout->setContentsMargins(0, 0, 0, 0);
    sidebar_layout->setSpacing(0);

    auto* sidebar_header = new QFrame(sidebar_panel_);
    sidebar_header->setObjectName(QStringLiteral("sidebarHeader"));
    sidebar_header->setFixedHeight(56);
    auto* sidebar_header_layout = new QHBoxLayout(sidebar_header);
    sidebar_header_layout->setContentsMargins(16, 0, 12, 0);
    sidebar_header_layout->setSpacing(8);

    sidebar_title_label_ = new QLabel(QStringLiteral("消息"), sidebar_header);
    sidebar_title_label_->setObjectName(QStringLiteral("sidebarTitle"));
    sidebar_header_layout->addWidget(sidebar_title_label_);
    sidebar_header_layout->addStretch();
    sidebar_action_btn_ = createIconButton(QStringLiteral("群"),
                                           QStringLiteral("查看在线用户"),
                                           30,
                                           sidebar_header);
    sidebar_header_layout->addWidget(sidebar_action_btn_);
    sidebar_layout->addWidget(sidebar_header);

    auto* sidebar_search_row = new QFrame(sidebar_panel_);
    sidebar_search_row->setObjectName(QStringLiteral("sidebarSearchRow"));
    sidebar_search_row->setFixedHeight(48);
    auto* sidebar_search_layout = new QHBoxLayout(sidebar_search_row);
    sidebar_search_layout->setContentsMargins(12, 8, 12, 8);
    sidebar_search_layout->setSpacing(0);

    contact_search_edit_ = new QLineEdit(sidebar_search_row);
    contact_search_edit_->setObjectName(QStringLiteral("sidebarSearchEdit"));
    contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
    sidebar_search_layout->addWidget(contact_search_edit_);
    sidebar_layout->addWidget(sidebar_search_row);

    contact_list_ = new QListWidget(sidebar_panel_);
    contact_list_->setObjectName(QStringLiteral("contactList"));
    contact_list_->setSpacing(0);
    contact_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    sidebar_layout->addWidget(contact_list_, 1);

    auto* center_panel = new QWidget(content_splitter_);
    center_panel->setObjectName(QStringLiteral("centerPanel"));
    auto* center_layout = new QVBoxLayout(center_panel);
    center_layout->setContentsMargins(0, 0, 0, 0);
    center_layout->setSpacing(0);

    center_stack_ = new QStackedWidget(center_panel);
    center_layout->addWidget(center_stack_, 1);

    {
        chat_panel_widget_ = new ChatPanel(current_username_, center_stack_);
        message_list_ = chat_panel_widget_->messageList();
        message_input_ = chat_panel_widget_->messageInput();
        send_btn_ = chat_panel_widget_->sendButton();
        group_list_btn_ = chat_panel_widget_->groupListButton();
        center_stack_->addWidget(chat_panel_widget_);
    }

    {
        file_panel_widget_ = new FilePanel(center_stack_);
        file_path_label_ = file_panel_widget_->pathLabel();
        file_search_edit_ = file_panel_widget_->searchEdit();
        file_list_ = file_panel_widget_->fileList();
        file_empty_state_label_ = file_panel_widget_->emptyStateLabel();
        file_back_btn_ = file_panel_widget_->backButton();
        file_upload_btn_ = file_panel_widget_->uploadButton();
        file_refresh_btn_ = file_panel_widget_->refreshButton();
        file_create_btn_ = file_panel_widget_->createButton();
        file_transfer_cancel_btn_ = file_panel_widget_->transferCancelButton();
        file_download_btn_ = file_panel_widget_->downloadButton();
        file_share_btn_ = file_panel_widget_->shareButton();
        file_rename_btn_ = file_panel_widget_->renameButton();
        file_move_btn_ = file_panel_widget_->moveButton();
        file_delete_btn_ = file_panel_widget_->deleteButton();
        center_stack_->addWidget(file_panel_widget_);
    }

    {
        auto* page = new QWidget(center_stack_);
        page->setObjectName(QStringLiteral("profilePage"));
        auto* page_layout = new QVBoxLayout(page);
        page_layout->setContentsMargins(0, 0, 0, 0);
        page_layout->setSpacing(0);
        page_layout->addStretch();

        auto* card_row = new QHBoxLayout();
        card_row->setContentsMargins(0, 0, 0, 0);
        card_row->setSpacing(0);
        card_row->addStretch();

        auto* card = new QFrame(page);
        card->setObjectName(QStringLiteral("profileCard"));
        card->setFixedWidth(360);
        applyShadow(card, 32, 6, 20);
        auto* card_layout = new QVBoxLayout(card);
        card_layout->setContentsMargins(32, 32, 32, 32);
        card_layout->setSpacing(12);

        auto* profile_avatar = new QLabel(card);
        prepareAvatarBadge(profile_avatar, current_username_, 64);
        card_layout->addWidget(profile_avatar, 0, Qt::AlignHCenter);

        profile_name_label_ = new QLabel(current_username_, card);
        profile_name_label_->setObjectName(QStringLiteral("profileName"));
        profile_name_label_->setAlignment(Qt::AlignCenter);
        card_layout->addWidget(profile_name_label_);

        profile_id_label_ = new QLabel(QStringLiteral("@%1").arg(current_username_), card);
        profile_id_label_->setObjectName(QStringLiteral("profileId"));
        profile_id_label_->setAlignment(Qt::AlignCenter);
        card_layout->addWidget(profile_id_label_);

        auto* divider_top = new QFrame(card);
        divider_top->setFrameShape(QFrame::HLine);
        card_layout->addWidget(divider_top);

        auto* nickname_label = new QLabel(QStringLiteral("昵称"), card);
        nickname_label->setObjectName(QStringLiteral("fieldLabel"));
        card_layout->addWidget(nickname_label);

        profile_nickname_edit_ = new QLineEdit(card);
        profile_nickname_edit_->setPlaceholderText(QStringLiteral("请输入昵称"));
        card_layout->addWidget(profile_nickname_edit_);

        auto* signature_label = new QLabel(QStringLiteral("个性签名"), card);
        signature_label->setObjectName(QStringLiteral("fieldLabel"));
        card_layout->addWidget(signature_label);

        profile_signature_edit_ = new QLineEdit(card);
        profile_signature_edit_->setPlaceholderText(QStringLiteral("写点什么介绍自己"));
        card_layout->addWidget(profile_signature_edit_);

        auto* action_row = new QHBoxLayout();
        action_row->setSpacing(10);
        auto* save_btn = new QPushButton(QStringLiteral("保存"), card);
        save_btn->setObjectName(QStringLiteral("primaryBtn"));
        auto* cancel_btn = new QPushButton(QStringLiteral("取消"), card);
        cancel_btn->setObjectName(QStringLiteral("secondaryBtn"));
        action_row->addWidget(save_btn);
        action_row->addWidget(cancel_btn);
        card_layout->addLayout(action_row);

        auto* divider_bottom = new QFrame(card);
        divider_bottom->setFrameShape(QFrame::HLine);
        card_layout->addWidget(divider_bottom);

        logout_btn_ = new QPushButton(QStringLiteral("退出登录"), card);
        logout_btn_->setObjectName(QStringLiteral("logoutBtn"));
        card_layout->addWidget(logout_btn_);

        connect(save_btn, &QPushButton::clicked, this, &MainWindow::saveProfileDraft);
        connect(cancel_btn, &QPushButton::clicked, this, &MainWindow::loadProfileDraft);

        card_row->addWidget(card);
        card_row->addStretch();
        page_layout->addLayout(card_row);
        page_layout->addStretch();

        center_stack_->addWidget(page);
    }

    detail_panel_widget_ = new DetailPanel(current_username_, content_splitter_);
    detail_panel_ = detail_panel_widget_;

    content_splitter_->addWidget(sidebar_panel_);
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

    connect(send_btn_, &QPushButton::clicked, this, &MainWindow::sendCurrentMessage);
    connect(chat_panel_widget_, &ChatPanel::sendRequested,
            this, &MainWindow::sendCurrentMessage);
    connect(group_list_btn_, &QPushButton::clicked, this, &MainWindow::openGroupListDialog);
    connect(contact_search_edit_, &QLineEdit::textChanged,
            this, &MainWindow::filterFriendList);
    connect(contact_list_, &QListWidget::itemSelectionChanged,
            this, &MainWindow::applySelectedFriend);
    connect(sidebar_action_btn_, &QPushButton::clicked,
            this, &MainWindow::openOnlineUserDialog);

    connect(logout_btn_, &QPushButton::clicked, this, &MainWindow::logoutRequested);
    connect(logout_btn_, &QPushButton::clicked, this, [this] {
        appendEventLog(QStringLiteral("用户请求退出登录"), QStringLiteral("→"));
    });

    connect(file_back_btn_, &QPushButton::clicked, this, &MainWindow::openCurrentParentDirectory);
    connect(file_upload_btn_, &QPushButton::clicked, this, &MainWindow::uploadFile);
    connect(file_refresh_btn_, &QPushButton::clicked,
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
    connect(file_create_btn_, &QPushButton::clicked, this, &MainWindow::createDirectory);
    connect(file_search_edit_, &QLineEdit::returnPressed, this, &MainWindow::runFileSearch);
    connect(file_search_edit_, &QLineEdit::textChanged, this, &MainWindow::clearFileSearchIfNeeded);
    connect(file_list_, &QListWidget::itemSelectionChanged, this, &MainWindow::applySelectedFile);
    connect(file_list_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                if (!item) {
                    return;
                }
                if (!item->data(Qt::UserRole + 1).toBool()) {
                    return;
                }
                file_search_mode_ = false;
                current_file_query_.clear();
                file_search_edit_->clear();
                navigateToFilePath(item->data(Qt::UserRole).toString());
            });
    connect(file_rename_btn_, &QPushButton::clicked, this, &MainWindow::renameSelectedFile);
    connect(file_move_btn_, &QPushButton::clicked, this, &MainWindow::moveSelectedFile);
    connect(file_delete_btn_, &QPushButton::clicked, this, &MainWindow::deleteSelectedFile);
    connect(file_download_btn_, &QPushButton::clicked, this, &MainWindow::downloadSelectedFile);
    connect(file_share_btn_, &QPushButton::clicked, this, &MainWindow::openShareFileDialog);
    connect(file_transfer_cancel_btn_, &QPushButton::clicked,
            this, [this] {
                if (!pending_upload_paths_.isEmpty()) {
                    pending_upload_paths_.clear();
                    setFileStatus(QStringLiteral("已清空等待中的上传队列，当前正在进行的传输不会中断。"));
                    file_transfer_cancel_btn_->setEnabled(false);
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
                    showChatConversation(peer, selectedFriendOnline());
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
                showChatConversation(peer, selectedFriendOnline());
            });
    connect(&chat_service_, &cloudvault::ChatService::historyLoadFailed,
            this, [this](const QString& peer, const QString& reason) {
                pending_history_requests_.remove(peer);
                if (peer != active_chat_peer_) {
                    return;
                }
                showChatConversation(peer, selectedFriendOnline());
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
                file_search_edit_->clear();
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
                file_search_edit_->clear();
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
                file_search_edit_->clear();
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
        if (center_stack_->currentIndex() == 1 && file_list_->currentItem()) {
            deleteSelectedFile();
        }
    });

    auto* rename_shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    connect(rename_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1 && file_list_->currentItem()) {
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
            file_search_edit_->setFocus();
            file_search_edit_->selectAll();
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
    filterFriendList(contact_search_edit_->text());
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
    filterFriendList(contact_search_edit_->text());
}

void MainWindow::clearConversationUnread(const QString& peer) {
    auto it = conversation_summaries_.find(peer);
    if (it == conversation_summaries_.end() || it->unread_count == 0) {
        return;
    }
    it->unread_count = 0;
    filterFriendList(contact_search_edit_->text());
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
    filterFriendList(contact_search_edit_->text());
}

void MainWindow::filterFriendList(const QString& keyword) {
    const QString current = !active_chat_peer_.isEmpty()
        ? active_chat_peer_
        : selectedFriend();
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
        const QSignalBlocker blocker(contact_list_);
        contact_list_->clear();

        for (const auto& [username, online] : ordered_friends) {
            if (!trimmed.isEmpty() && !username.contains(trimmed, Qt::CaseInsensitive)) {
                continue;
            }

            const auto summary_it = conversation_summaries_.constFind(username);
            const bool has_message = summary_it != conversation_summaries_.constEnd()
                                  && summary_it->has_message;
            const QString preview = has_message
                ? summary_it->preview
                : QStringLiteral("暂无消息");
            const QString time = has_message ? formatConversationTime(summary_it->timestamp) : QString();
            const int unread = has_message ? summary_it->unread_count : 0;

            auto* item = new QListWidgetItem(contact_list_);
            item->setSizeHint(QSize(0, 64));
            item->setData(Qt::UserRole, username);
            item->setData(Qt::UserRole + 1, online);

            const bool is_selected = username == current;
            auto* widget = createContactItemWidget(username,
                                                   preview,
                                                   time,
                                                   unread,
                                                   online,
                                                   is_selected,
                                                   contact_list_);
            contact_list_->setItemWidget(item, widget);
            if (is_selected) {
                contact_list_->setCurrentItem(item);
                item->setSelected(true);
            }
        }

        if (contact_list_->count() > 0 && current.isEmpty()) {
            contact_list_->setCurrentRow(0);
        }
    }

    if (contact_list_->count() == 0) {
        showChatEmptyState();
    }

    updateContactSelectionState();

    if (selectedFriend() != current) {
        applySelectedFriend();
    }
}

void MainWindow::updateContactSelectionState() {
    for (int i = 0; i < contact_list_->count(); ++i) {
        auto* item = contact_list_->item(i);
        auto* widget = contact_list_->itemWidget(item);
        if (!widget) {
            continue;
        }
        const bool selected = item->isSelected();
        widget->setProperty("selected", selected);
        if (auto* accent = widget->findChild<QFrame*>(QStringLiteral("contactAccent"))) {
            accent->setProperty("selected", selected);
            repolish(accent);
        }
        repolish(widget);
    }
}

void MainWindow::applySelectedFriend() {
    updateContactSelectionState();

    const QString username = selectedFriend();
    if (username.isEmpty()) {
        showChatEmptyState();
        return;
    }

    const bool online = selectedFriendOnline();
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

    const QString content = message_input_->toPlainText().trimmed();
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
    message_input_->clear();
    showChatConversation(active_chat_peer_, selectedFriendOnline());
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
            file_list_->count() == 0);
    }

    if (file_list_->count() > 0) {
        if (file_panel_widget_) {
            file_panel_widget_->selectFirstEntry();
        }
        setFileStatus(file_search_mode_
                          ? QStringLiteral("搜索完成，共找到 %1 项。").arg(file_list_->count())
                          : QStringLiteral("目录已刷新，共 %1 项。").arg(file_list_->count()));
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

    const bool has_selection = file_list_->currentItem() != nullptr;
    const bool is_file = has_selection && !selectedFileIsDir();
    file_download_btn_->setEnabled(is_file);
    file_share_btn_->setEnabled(is_file);
    file_rename_btn_->setEnabled(has_selection);
    file_move_btn_->setEnabled(has_selection);
    file_delete_btn_->setEnabled(has_selection);
}

void MainWindow::applySelectedFile() {
    updateFileSelectionState();

    const QString path = selectedFilePath();
    if (path.isEmpty()) {
        resetFileActionSummary();
        return;
    }

    const bool is_dir = selectedFileIsDir();
    const QString name = file_list_->currentItem()->data(Qt::UserRole + 2).toString();
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
        file_search_edit_->clear();
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
    const QString path = selectedFilePath();
    if (path.isEmpty()) {
        return;
    }

    const QString current_name = file_list_->currentItem()->data(Qt::UserRole + 2).toString();
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
    const QString path = selectedFilePath();
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
    const QString path = selectedFilePath();
    if (path.isEmpty()) {
        return;
    }

    const QString current_name = file_list_->currentItem()->data(Qt::UserRole + 2).toString();
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
    const QString keyword = file_search_edit_->text().trimmed();
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
        sidebar_title_label_->setText(QStringLiteral("消息"));
        contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
        sidebar_action_btn_->setVisible(true);
    } else if (index == 1) {
        sidebar_title_label_->setText(QStringLiteral("文件"));
        contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
        sidebar_action_btn_->setVisible(false);
    } else {
        sidebar_action_btn_->setVisible(false);
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
    const QString path = selectedFilePath();
    if (path.isEmpty() || selectedFileIsDir()) {
        return;
    }

    const QString file_name = file_list_->currentItem()->data(Qt::UserRole + 2).toString();
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
    const QString remote_path = selectedFilePath();
    if (remote_path.isEmpty() || selectedFileIsDir()) {
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
        file_transfer_cancel_btn_->setEnabled(!pending_upload_paths_.isEmpty());
        return;
    }

    const QString next_path = pending_upload_paths_.takeFirst();
    const QString filename = QFileInfo(next_path).fileName();
    showTransferRow(QStringLiteral("准备上传 %1").arg(filename), 0, true);
    setFileStatus(QStringLiteral("正在准备上传 %1 …").arg(filename));
    file_transfer_cancel_btn_->setEnabled(!pending_upload_paths_.isEmpty());
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

    profile_nickname_edit_->setText(nickname);
    profile_signature_edit_->setText(signature);
    profile_name_label_->setText(nickname.trimmed().isEmpty() ? current_username_ : nickname.trimmed());
}

void MainWindow::saveProfileDraft() {
    const QString nickname = profile_nickname_edit_->text().trimmed();
    const QString signature = profile_signature_edit_->text().trimmed();

    QSettings settings;
    settings.beginGroup(QStringLiteral("profile/%1").arg(current_username_));
    settings.setValue(QStringLiteral("nickname"), nickname);
    settings.setValue(QStringLiteral("signature"), signature);
    settings.endGroup();

    profile_name_label_->setText(nickname.isEmpty() ? current_username_ : nickname);
}

QString MainWindow::selectedFriend() const {
    if (auto* item = contact_list_->currentItem()) {
        return item->data(Qt::UserRole).toString();
    }
    return {};
}

bool MainWindow::selectedFriendOnline() const {
    if (auto* item = contact_list_->currentItem()) {
        return item->data(Qt::UserRole + 1).toBool();
    }
    return false;
}

QString MainWindow::selectedFilePath() const {
    if (auto* item = file_list_->currentItem()) {
        return item->data(Qt::UserRole).toString();
    }
    return {};
}

bool MainWindow::selectedFileIsDir() const {
    if (auto* item = file_list_->currentItem()) {
        return item->data(Qt::UserRole + 1).toBool();
    }
    return false;
}
