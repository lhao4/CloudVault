// =============================================================
// client/src/ui/main_window.cpp
// 主窗口（三栏）
// =============================================================

#include "main_window.h"

#include "ui/online_user_dialog.h"
#include "ui/share_file_dialog.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QString mainWindowStyle() {
    return QStringLiteral(R"(
        QMainWindow,
        QWidget#windowShell {
            background: #F4F6F8;
            color: #111827;
            font-family: "DM Sans", "PingFang SC", "Microsoft YaHei", sans-serif;
        }

        QFrame#sidebarPanel,
        QFrame#centerPanel,
        QFrame#detailPanel,
        QFrame#chatHeader,
        QFrame#composeBar,
        QFrame#profileCard,
        QFrame#detailCard,
        QFrame#navBar {
            background: #FFFFFF;
            border: 1px solid #E2E6EA;
            border-radius: 12px;
        }

        QFrame#sidebarPanel,
        QFrame#centerPanel,
        QFrame#detailPanel {
            background: #FFFFFF;
        }

        QLabel#panelTitleLabel,
        QLabel#detailTitleLabel,
        QLabel#centerTitleLabel {
            color: #111827;
            font-size: 18px;
            font-weight: 700;
        }

        QLabel#sectionTitleLabel {
            color: #111827;
            font-size: 14px;
            font-weight: 600;
        }

        QLabel#secondaryLabel,
        QLabel#metaLabel,
        QLabel#timeLabel {
            color: #6B7280;
            font-size: 12px;
        }

        QLabel#mutedLabel {
            color: #9CA3AF;
            font-size: 12px;
        }

        QLineEdit {
            min-height: 44px;
            background: #F0F2F5;
            border: 1px solid #E2E6EA;
            border-radius: 8px;
            padding: 0 14px;
            font-size: 14px;
        }

        QLineEdit:focus {
            background: #FFFFFF;
            border-color: #3B82F6;
        }

        QPushButton {
            min-height: 34px;
            border-radius: 8px;
            padding: 0 12px;
            border: none;
            font-size: 13px;
            font-weight: 600;
        }

        QPushButton#iconButton {
            min-width: 30px;
            max-width: 30px;
            min-height: 30px;
            max-height: 30px;
            padding: 0;
            border: 1px solid #E2E6EA;
            background: #F0F2F5;
            color: #374151;
        }

        QPushButton#iconButton:hover {
            background: #EFF6FF;
            border-color: #93C5FD;
            color: #2563EB;
        }

        QPushButton#lightButton {
            background: #F8FAFC;
            color: #374151;
            border: 1px solid #E2E6EA;
        }

        QPushButton#lightButton:hover {
            background: #EFF6FF;
            border-color: #93C5FD;
            color: #2563EB;
        }

        QPushButton#primaryButton {
            background: #3B82F6;
            color: white;
        }

        QPushButton#primaryButton:hover {
            background: #2563EB;
        }

        QPushButton#dangerButton {
            background: #FEF2F2;
            color: #EF4444;
            border: 1px solid #FECACA;
        }

        QPushButton#dangerButton:hover {
            background: #FEE2E2;
        }

        QPushButton#dangerButton:disabled {
            background: #F8FAFC;
            color: #C4CAD4;
            border-color: #E5E7EB;
        }

        QListWidget#contactList {
            border: none;
            outline: none;
            background: transparent;
        }

        QListWidget#contactList::item {
            margin-bottom: 6px;
            border: none;
            padding: 0;
        }

        QFrame#contactItem {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 10px;
        }

        QFrame#contactItem[selected="true"] {
            background: #EFF6FF;
            border-color: #BFDBFE;
        }

        QFrame#contactAccent {
            min-width: 3px;
            max-width: 3px;
            border-radius: 1px;
            background: transparent;
        }

        QFrame#contactAccent[selected="true"] {
            background: #3B82F6;
        }

        QLabel#contactAvatar {
            min-width: 40px;
            min-height: 40px;
            max-width: 40px;
            max-height: 40px;
            border-radius: 20px;
            background: #E0ECFF;
            color: #2563EB;
            font-size: 13px;
            font-weight: 700;
        }

        QLabel#contactNameLabel {
            color: #111827;
            font-size: 14px;
            font-weight: 600;
        }

        QLabel#contactPreviewLabel,
        QLabel#contactTimeLabel {
            color: #6B7280;
            font-size: 12px;
        }

        QLabel#contactBadgeLabel {
            padding: 1px 6px;
            min-width: 18px;
            background: #3B82F6;
            color: white;
            border-radius: 9px;
            font-size: 10px;
            font-weight: 700;
        }

        QPushButton#bottomTabButton {
            min-height: 46px;
            border-radius: 10px;
            background: transparent;
            color: #9CA3AF;
            border: none;
            font-size: 14px;
            font-weight: 600;
        }

        QPushButton#bottomTabButton:hover {
            background: #F8FAFC;
            color: #374151;
        }

        QPushButton#bottomTabButton:checked {
            background: #EFF6FF;
            color: #3B82F6;
        }

        QLabel#incomingBubble {
            background: #FFFFFF;
            border: 1px solid #E2E6EA;
            border-radius: 12px 12px 12px 4px;
            padding: 10px 12px;
            color: #111827;
            font-size: 14px;
        }

        QLabel#outgoingBubble {
            background: #3B82F6;
            border: none;
            border-radius: 12px 12px 4px 12px;
            padding: 10px 12px;
            color: white;
            font-size: 14px;
        }

        QLabel#fileBubble {
            background: #FFFFFF;
            border: 1px solid #E2E6EA;
            border-radius: 12px;
            padding: 12px;
            color: #111827;
            font-size: 14px;
        }

        QLabel#dateDividerLabel {
            color: #9CA3AF;
            font-size: 12px;
        }

        QTextEdit#messageInput {
            min-height: 44px;
            max-height: 84px;
            background: #F0F2F5;
            border: 1.5px solid #E2E6EA;
            border-radius: 12px;
            padding: 8px 12px;
            font-size: 14px;
        }

        QTextEdit#messageInput:focus {
            border-color: #3B82F6;
        }

        QLineEdit#searchInput {
            min-height: 32px;
            max-height: 32px;
            background: #F0F2F5;
            border: none;
            border-radius: 8px;
            padding: 0 10px;
            font-size: 12px;
            color: #111827;
        }

        QLineEdit#searchInput:focus {
            background: #E5E7EB;
        }
    )");
}

QPushButton* createIconButton(const QString& text, QWidget* parent = nullptr) {
    auto* button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("iconButton"));
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

QPushButton* createBottomTabButton(const QString& text, QWidget* parent = nullptr) {
    auto* button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("bottomTabButton"));
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

QLabel* createMetaLabel(const QString& text, QWidget* parent = nullptr) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("metaLabel"));
    label->setWordWrap(true);
    return label;
}

QFrame* createDetailCard(const QString& title,
                         const QString& line1,
                         const QString& line2,
                         QWidget* parent = nullptr) {
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("detailCard"));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(3);

    auto* title_label = new QLabel(title, card);
    title_label->setObjectName(QStringLiteral("sectionTitleLabel"));
    layout->addWidget(title_label);
    layout->addWidget(createMetaLabel(line1, card));
    layout->addWidget(createMetaLabel(line2, card));
    return card;
}

QWidget* createMessageBubble(const QString& text,
                             const QString& time,
                             bool outgoing,
                             const QString& senderName = QString(),
                             QWidget* parent = nullptr) {
    auto* wrap = new QWidget(parent);
    wrap->setStyleSheet(QStringLiteral("background: transparent;"));
    auto* outer = new QVBoxLayout(wrap);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* bubble_row = new QHBoxLayout();
    bubble_row->setContentsMargins(0, 0, 0, 0);
    bubble_row->setSpacing(6);
    bubble_row->setAlignment(Qt::AlignBottom);

    auto* bubble = new QLabel(text, wrap);
    bubble->setObjectName(outgoing ? QStringLiteral("outgoingBubble")
                                   : QStringLiteral("incomingBubble"));
    bubble->setWordWrap(true);
    bubble->setMaximumWidth(300);

    auto* time_label = new QLabel(time, wrap);
    time_label->setStyleSheet(QStringLiteral("color: #9CA3AF; font-size: 10px; background: transparent;"));
    time_label->setAlignment(Qt::AlignBottom);

    if (outgoing) {
        // outgoing: [stretch][time][bubble]
        bubble_row->addStretch();
        bubble_row->addWidget(time_label, 0, Qt::AlignBottom);
        bubble_row->addWidget(bubble);
    } else {
        // incoming: [avatar][bubble][time][stretch]
        auto* avatar = new QLabel(senderName.isEmpty() ? QStringLiteral("?") : senderName.left(1).toUpper(), wrap);
        avatar->setFixedSize(28, 28);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(QStringLiteral(
            "background: #3B82F6; color: white; font-size: 9pt; font-weight: bold;"
            "border-radius: 14px;"));
        bubble_row->addWidget(avatar, 0, Qt::AlignBottom);
        bubble_row->addWidget(bubble);
        bubble_row->addWidget(time_label, 0, Qt::AlignBottom);
        bubble_row->addStretch();
    }
    outer->addLayout(bubble_row);
    return wrap;
}

[[maybe_unused]] QWidget* createFileBubble(const QString& title,
                                           const QString& meta,
                                           const QString& time,
                                           QWidget* parent = nullptr) {
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* bubble = new QLabel(QStringLiteral("📄 %1\n%2    ↗").arg(title, meta), wrap);
    bubble->setObjectName(QStringLiteral("fileBubble"));
    bubble->setWordWrap(true);
    bubble->setMaximumWidth(340);
    layout->addWidget(bubble, 0, Qt::AlignLeft);

    auto* time_label = new QLabel(time, wrap);
    time_label->setObjectName(QStringLiteral("timeLabel"));
    layout->addWidget(time_label, 0, Qt::AlignLeft);
    return wrap;
}

QWidget* createContactItemWidget(const QString& username,
                                 const QString& preview,
                                 const QString& time,
                                 int unread,
                                 bool selected,
                                 QWidget* parent = nullptr) {
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("contactItem"));
    frame->setProperty("selected", selected);

    auto* root = new QHBoxLayout(frame);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);

    auto* accent = new QFrame(frame);
    accent->setObjectName(QStringLiteral("contactAccent"));
    accent->setProperty("selected", selected);
    root->addWidget(accent);

    auto* body = new QWidget(frame);
    auto* body_layout = new QHBoxLayout(body);
    body_layout->setContentsMargins(8, 8, 10, 8);
    body_layout->setSpacing(10);

    auto* avatar = new QLabel(username.left(1).toUpper(), body);
    avatar->setObjectName(QStringLiteral("contactAvatar"));
    avatar->setAlignment(Qt::AlignCenter);
    body_layout->addWidget(avatar);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(2);
    auto* name_label = new QLabel(username, body);
    name_label->setObjectName(QStringLiteral("contactNameLabel"));
    auto* preview_label = new QLabel(preview, body);
    preview_label->setObjectName(QStringLiteral("contactPreviewLabel"));
    text_layout->addWidget(name_label);
    text_layout->addWidget(preview_label);
    body_layout->addLayout(text_layout, 1);

    auto* meta_layout = new QVBoxLayout();
    meta_layout->setContentsMargins(0, 2, 0, 2);
    meta_layout->setSpacing(4);
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
    return frame;
}

void repolish(QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace

MainWindow::MainWindow(const QString& username,
                       cloudvault::FriendService& friend_service,
                       cloudvault::ChatService& chat_service,
                       QWidget* parent)
    : QMainWindow(parent),
      current_username_(username),
      friend_service_(friend_service),
      chat_service_(chat_service) {
    setupUi();
    connectSignals();
    friend_service_.flushFriends();
    switchMainTab(0);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    emit windowClosed();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("CloudVault - %1").arg(current_username_));
    resize(1080, 700);
    setMinimumSize(900, 580);
    setStyleSheet(mainWindowStyle());

    auto* shell = new QWidget(this);
    shell->setObjectName(QStringLiteral("windowShell"));
    auto* shell_layout = new QVBoxLayout(shell);
    shell_layout->setContentsMargins(14, 14, 14, 14);
    shell_layout->setSpacing(10);

    content_root_ = new QWidget(shell);
    content_layout_ = new QVBoxLayout(content_root_);
    content_layout_->setContentsMargins(0, 0, 0, 0);
    content_layout_->setSpacing(0);

    auto* columns = new QHBoxLayout();
    columns->setContentsMargins(0, 0, 0, 0);
    columns->setSpacing(10);
    content_layout_->addLayout(columns);

    sidebar_panel_ = new QFrame(content_root_);
    sidebar_panel_->setObjectName(QStringLiteral("sidebarPanel"));
    sidebar_panel_->setFixedWidth(260);
    auto* sidebar_layout = new QVBoxLayout(sidebar_panel_);
    sidebar_layout->setContentsMargins(14, 14, 14, 14);
    sidebar_layout->setSpacing(10);

    auto* sidebar_header = new QHBoxLayout();
    sidebar_header->setSpacing(8);
    sidebar_title_label_ = new QLabel(QStringLiteral("消息"), sidebar_panel_);
    sidebar_title_label_->setObjectName(QStringLiteral("panelTitleLabel"));
    sidebar_header->addWidget(sidebar_title_label_);
    sidebar_header->addStretch();
    sidebar_action_btn_ = createIconButton(QStringLiteral("✏"), sidebar_panel_);
    sidebar_header->addWidget(sidebar_action_btn_);
    sidebar_layout->addLayout(sidebar_header);

    contact_search_edit_ = new QLineEdit(sidebar_panel_);
    contact_search_edit_->setObjectName(QStringLiteral("searchInput"));
    contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
    sidebar_layout->addWidget(contact_search_edit_);

    contact_list_ = new QListWidget(sidebar_panel_);
    contact_list_->setObjectName(QStringLiteral("contactList"));
    contact_list_->setSpacing(0);
    sidebar_layout->addWidget(contact_list_, 1);
    columns->addWidget(sidebar_panel_);

    auto* center_panel = new QFrame(content_root_);
    center_panel->setObjectName(QStringLiteral("centerPanel"));
    auto* center_layout = new QVBoxLayout(center_panel);
    center_layout->setContentsMargins(12, 12, 12, 12);
    center_layout->setSpacing(10);

    center_stack_ = new QStackedWidget(center_panel);

    {
        auto* page = new QWidget(center_stack_);
        auto* page_layout = new QVBoxLayout(page);
        page_layout->setContentsMargins(0, 0, 0, 0);
        page_layout->setSpacing(8);

        auto* header = new QFrame(page);
        header->setObjectName(QStringLiteral("chatHeader"));
        auto* header_layout = new QHBoxLayout(header);
        header_layout->setContentsMargins(14, 12, 14, 12);
        header_layout->setSpacing(10);

        auto* title_layout = new QVBoxLayout();
        title_layout->setSpacing(2);
        chat_title_label_ = new QLabel(QStringLiteral("● 联系人"), header);
        chat_title_label_->setObjectName(QStringLiteral("centerTitleLabel"));
        chat_status_label_ = new QLabel(QStringLiteral("在线"), header);
        chat_status_label_->setObjectName(QStringLiteral("secondaryLabel"));
        title_layout->addWidget(chat_title_label_);
        title_layout->addWidget(chat_status_label_);
        header_layout->addLayout(title_layout, 1);
        header_layout->addWidget(createIconButton(QStringLiteral("📎"), header));
        header_layout->addWidget(createIconButton(QStringLiteral("⋯"), header));
        page_layout->addWidget(header);

        message_scroll_area_ = new QScrollArea(page);
        message_scroll_area_->setFrameShape(QFrame::NoFrame);
        message_scroll_area_->setWidgetResizable(true);
        message_scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        message_scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        message_scroll_area_->setStyleSheet(QStringLiteral(
            "QScrollArea { background: #F4F6F8; border: none; }"
            "QScrollArea > QWidget > QWidget { background: #F4F6F8; }"));

        message_list_widget_ = new QWidget(message_scroll_area_);
        message_list_widget_->setStyleSheet(QStringLiteral("background: #F4F6F8;"));
        message_list_layout_ = new QVBoxLayout(message_list_widget_);
        message_list_layout_->setContentsMargins(4, 2, 4, 2);
        message_list_layout_->setSpacing(8);
        message_list_layout_->addStretch();
        message_scroll_area_->setWidget(message_list_widget_);
        page_layout->addWidget(message_scroll_area_, 1);

        auto* compose = new QFrame(page);
        compose->setObjectName(QStringLiteral("composeBar"));
        auto* compose_layout = new QHBoxLayout(compose);
        compose_layout->setContentsMargins(10, 8, 10, 8);
        compose_layout->setSpacing(8);
        compose_layout->addWidget(createIconButton(QStringLiteral("😊"), compose));
        compose_layout->addWidget(createIconButton(QStringLiteral("📎"), compose));
        message_input_ = new QTextEdit(compose);
        message_input_->setObjectName(QStringLiteral("messageInput"));
        message_input_->setPlaceholderText(QStringLiteral("输入消息…"));
        compose_layout->addWidget(message_input_, 1);
        send_message_btn_ = new QPushButton(QStringLiteral("➤"), compose);
        send_message_btn_->setObjectName(QStringLiteral("primaryButton"));
        send_message_btn_->setFixedWidth(42);
        compose_layout->addWidget(send_message_btn_);
        page_layout->addWidget(compose);

        center_stack_->addWidget(page);
    }

    {
        auto* page = new QWidget(center_stack_);
        auto* page_layout = new QVBoxLayout(page);
        page_layout->setContentsMargins(0, 0, 0, 0);
        page_layout->setSpacing(10);

        auto* header = new QFrame(page);
        header->setObjectName(QStringLiteral("chatHeader"));
        auto* header_layout = new QVBoxLayout(header);
        header_layout->setContentsMargins(14, 12, 14, 12);
        header_layout->setSpacing(2);
        auto* title = new QLabel(QStringLiteral("文件"), header);
        title->setObjectName(QStringLiteral("centerTitleLabel"));
        file_path_label_ = new QLabel(QStringLiteral("📁 /未选择联系人/"), header);
        file_path_label_->setObjectName(QStringLiteral("secondaryLabel"));
        header_layout->addWidget(title);
        header_layout->addWidget(file_path_label_);
        page_layout->addWidget(header);

        auto* toolbar = new QFrame(page);
        toolbar->setObjectName(QStringLiteral("chatHeader"));
        auto* toolbar_layout = new QHBoxLayout(toolbar);
        toolbar_layout->setContentsMargins(10, 8, 10, 8);
        toolbar_layout->setSpacing(8);
        auto* search = new QLineEdit(toolbar);
        search->setPlaceholderText(QStringLiteral("搜索文件…"));
        toolbar_layout->addWidget(search, 1);
        auto* back_btn = new QPushButton(QStringLiteral("↑ 返回"), toolbar);
        back_btn->setObjectName(QStringLiteral("lightButton"));
        auto* create_btn = new QPushButton(QStringLiteral("+ 新建"), toolbar);
        create_btn->setObjectName(QStringLiteral("primaryButton"));
        toolbar_layout->addWidget(back_btn);
        toolbar_layout->addWidget(create_btn);
        page_layout->addWidget(toolbar);

        auto* files = new QWidget(page);
        auto* files_layout = new QVBoxLayout(files);
        files_layout->setContentsMargins(4, 4, 4, 4);
        files_layout->setSpacing(8);
        files_layout->addWidget(createDetailCard(QStringLiteral("📁 documents"),
                                                 QStringLiteral("最近修改：今天"),
                                                 QStringLiteral("目录"), files));
        files_layout->addWidget(createDetailCard(QStringLiteral("📁 images"),
                                                 QStringLiteral("最近修改：昨天"),
                                                 QStringLiteral("目录"), files));
        files_layout->addWidget(createDetailCard(QStringLiteral("📄 report.pdf"),
                                                 QStringLiteral("2.4MB"),
                                                 QStringLiteral("今天上传"), files));
        files_layout->addWidget(createDetailCard(QStringLiteral("📄 photo.jpg"),
                                                 QStringLiteral("1.1MB"),
                                                 QStringLiteral("昨天上传"), files));
        files_layout->addWidget(createDetailCard(QStringLiteral("📄 data.xlsx"),
                                                 QStringLiteral("856KB"),
                                                 QStringLiteral("周一上传"), files));

        auto* selection_card = new QFrame(files);
        selection_card->setObjectName(QStringLiteral("detailCard"));
        auto* selection_layout = new QVBoxLayout(selection_card);
        selection_layout->setContentsMargins(12, 12, 12, 12);
        selection_layout->setSpacing(8);
        detail_file_target_label_ = new QLabel(QStringLiteral("选中：report.pdf"), selection_card);
        detail_file_target_label_->setObjectName(QStringLiteral("sectionTitleLabel"));
        selection_layout->addWidget(detail_file_target_label_);

        auto* file_action_row = new QHBoxLayout();
        file_action_row->setSpacing(8);
        auto* download_btn = new QPushButton(QStringLiteral("⬇ 下载"), selection_card);
        download_btn->setObjectName(QStringLiteral("lightButton"));
        auto* rename_btn = new QPushButton(QStringLiteral("✏ 重命名"), selection_card);
        rename_btn->setObjectName(QStringLiteral("lightButton"));
        auto* delete_btn = new QPushButton(QStringLiteral("🗑 删除"), selection_card);
        delete_btn->setObjectName(QStringLiteral("dangerButton"));
        file_share_btn_ = new QPushButton(QStringLiteral("↗ 分享"), selection_card);
        file_share_btn_->setObjectName(QStringLiteral("primaryButton"));
        file_action_row->addWidget(download_btn);
        file_action_row->addWidget(rename_btn);
        file_action_row->addWidget(delete_btn);
        file_action_row->addWidget(file_share_btn_);
        selection_layout->addLayout(file_action_row);

        auto* progress_title = new QLabel(QStringLiteral("上传进度"), selection_card);
        progress_title->setObjectName(QStringLiteral("sectionTitleLabel"));
        selection_layout->addWidget(progress_title);
        selection_layout->addWidget(createMetaLabel(QStringLiteral("████████░░ 80%  3.2/4MB")));
        files_layout->addWidget(selection_card);
        files_layout->addStretch();
        page_layout->addWidget(files, 1);

        center_stack_->addWidget(page);
    }

    {
        auto* page = new QWidget(center_stack_);
        auto* page_layout = new QVBoxLayout(page);
        page_layout->setContentsMargins(0, 0, 0, 0);
        page_layout->setSpacing(0);
        page_layout->addStretch();

        auto* card = new QFrame(page);
        card->setObjectName(QStringLiteral("profileCard"));
        card->setMaximumWidth(420);
        auto* card_layout = new QVBoxLayout(card);
        card_layout->setContentsMargins(24, 24, 24, 24);
        card_layout->setSpacing(12);

        auto* avatar = new QLabel(current_username_.left(1).toUpper(), card);
        avatar->setObjectName(QStringLiteral("contactAvatar"));
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setFixedSize(72, 72);
        card_layout->addWidget(avatar, 0, Qt::AlignHCenter);

        profile_name_label_ = new QLabel(current_username_, card);
        profile_name_label_->setObjectName(QStringLiteral("centerTitleLabel"));
        profile_name_label_->setAlignment(Qt::AlignCenter);
        card_layout->addWidget(profile_name_label_);

        profile_id_label_ = new QLabel(QStringLiteral("@%1").arg(current_username_), card);
        profile_id_label_->setObjectName(QStringLiteral("secondaryLabel"));
        profile_id_label_->setAlignment(Qt::AlignCenter);
        card_layout->addWidget(profile_id_label_);

        auto* nickname = new QLineEdit(card);
        nickname->setPlaceholderText(QStringLiteral("昵称"));
        nickname->setText(current_username_);
        card_layout->addWidget(nickname);

        auto* signature = new QLineEdit(card);
        signature->setPlaceholderText(QStringLiteral("个性签名"));
        card_layout->addWidget(signature);

        auto* action_row = new QHBoxLayout();
        action_row->setSpacing(8);
        auto* save_btn = new QPushButton(QStringLiteral("保存"), card);
        save_btn->setObjectName(QStringLiteral("primaryButton"));
        auto* cancel_btn = new QPushButton(QStringLiteral("取消"), card);
        cancel_btn->setObjectName(QStringLiteral("lightButton"));
        action_row->addWidget(save_btn);
        action_row->addWidget(cancel_btn);
        card_layout->addLayout(action_row);

        logout_btn_ = new QPushButton(QStringLiteral("退出登录"), card);
        logout_btn_->setObjectName(QStringLiteral("dangerButton"));
        card_layout->addWidget(logout_btn_);

        page_layout->addWidget(card, 0, Qt::AlignHCenter);
        page_layout->addStretch();
        center_stack_->addWidget(page);
    }

    center_layout->addWidget(center_stack_);
    columns->addWidget(center_panel, 1);

    detail_panel_ = new QFrame(content_root_);
    detail_panel_->setObjectName(QStringLiteral("detailPanel"));
    detail_panel_->setFixedWidth(220);
    auto* detail_layout = new QVBoxLayout(detail_panel_);
    detail_layout->setContentsMargins(14, 14, 14, 14);
    detail_layout->setSpacing(10);

    auto* detail_title = new QLabel(QStringLiteral("详情"), detail_panel_);
    detail_title->setObjectName(QStringLiteral("detailTitleLabel"));
    detail_layout->addWidget(detail_title);

    auto* contact_card = new QFrame(detail_panel_);
    contact_card->setObjectName(QStringLiteral("detailCard"));
    auto* contact_layout = new QVBoxLayout(contact_card);
    contact_layout->setContentsMargins(12, 12, 12, 12);
    contact_layout->setSpacing(4);
    auto* contact_title = new QLabel(QStringLiteral("联系人"), contact_card);
    contact_title->setObjectName(QStringLiteral("sectionTitleLabel"));
    detail_contact_name_label_ = new QLabel(QStringLiteral("未选择联系人"), contact_card);
    detail_contact_name_label_->setObjectName(QStringLiteral("sectionTitleLabel"));
    detail_contact_status_label_ = new QLabel(QStringLiteral("请选择左侧联系人"), contact_card);
    detail_contact_status_label_->setObjectName(QStringLiteral("secondaryLabel"));
    contact_layout->addWidget(contact_title);
    contact_layout->addWidget(detail_contact_name_label_);
    contact_layout->addWidget(detail_contact_status_label_);
    delete_friend_btn_ = new QPushButton(QStringLiteral("删除好友"), contact_card);
    delete_friend_btn_->setObjectName(QStringLiteral("dangerButton"));
    delete_friend_btn_->setEnabled(false);
    delete_friend_btn_->hide();
    detail_layout->addWidget(contact_card);

    auto* shared_title = new QLabel(QStringLiteral("共享文件"), detail_panel_);
    shared_title->setObjectName(QStringLiteral("sectionTitleLabel"));
    detail_layout->addWidget(shared_title);
    detail_layout->addWidget(createDetailCard(QStringLiteral("📄 需求文档"),
                                              QStringLiteral("2.4MB"),
                                              QStringLiteral("今天"), detail_panel_));
    detail_layout->addWidget(createDetailCard(QStringLiteral("📊 数据统计"),
                                              QStringLiteral("856KB"),
                                              QStringLiteral("昨天"), detail_panel_));
    detail_layout->addWidget(createDetailCard(QStringLiteral("🖼️ UI截图"),
                                              QStringLiteral("1.1MB"),
                                              QStringLiteral("周一"), detail_panel_));
    detail_layout->addStretch();
    columns->addWidget(detail_panel_);

    shell_layout->addWidget(content_root_, 1);

    auto* nav_bar = new QFrame(shell);
    nav_bar->setObjectName(QStringLiteral("navBar"));
    auto* nav_layout = new QHBoxLayout(nav_bar);
    nav_layout->setContentsMargins(10, 8, 10, 8);
    nav_layout->setSpacing(6);
    message_tab_btn_ = createBottomTabButton(QStringLiteral("💬 消息"), nav_bar);
    file_tab_btn_ = createBottomTabButton(QStringLiteral("📁 文件"), nav_bar);
    profile_tab_btn_ = createBottomTabButton(QStringLiteral("👤 我"), nav_bar);
    nav_layout->addWidget(message_tab_btn_);
    nav_layout->addWidget(file_tab_btn_);
    nav_layout->addWidget(profile_tab_btn_);
    shell_layout->addWidget(nav_bar);

    setCentralWidget(shell);
}

void MainWindow::connectSignals() {
    connect(message_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(0); });
    connect(file_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(1); });
    connect(profile_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(2); });

    connect(contact_search_edit_, &QLineEdit::textChanged,
            this, &MainWindow::filterFriendList);
    connect(contact_list_, &QListWidget::itemSelectionChanged,
            this, &MainWindow::applySelectedFriend);
    connect(sidebar_action_btn_, &QPushButton::clicked,
            this, &MainWindow::openOnlineUserDialog);

    connect(logout_btn_, &QPushButton::clicked, this, &MainWindow::logoutRequested);
    connect(file_share_btn_, &QPushButton::clicked, this, &MainWindow::openShareFileDialog);
    connect(delete_friend_btn_, &QPushButton::clicked,
            this, &MainWindow::confirmDeleteSelectedFriend);
    connect(send_message_btn_, &QPushButton::clicked,
            this, &MainWindow::sendCurrentMessage);

    connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
            this, &MainWindow::refreshFriendList);
    connect(&friend_service_, &cloudvault::FriendService::friendAdded,
            this, [this](const QString&) { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendDeleted,
            this, [this](const QString& username) {
                conversations_.remove(username);
                friend_service_.flushFriends();
                refreshCurrentConversation();
            });
    connect(&friend_service_, &cloudvault::FriendService::friendRequestSent,
            this, [this](const QString& target) {
                QMessageBox::information(this, QStringLiteral("好友申请"),
                    QStringLiteral("已向 %1 发送好友申请，等待对方同意。").arg(target));
            });
    connect(&friend_service_, &cloudvault::FriendService::friendAddFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("添加失败"), reason);
            });
    connect(&friend_service_, &cloudvault::FriendService::friendDeleteFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("删除失败"), reason);
            });
    connect(&friend_service_, &cloudvault::FriendService::incomingFriendRequest,
            this, [this](const QString& from) {
                auto reply = QMessageBox::question(
                    this, QStringLiteral("好友申请"),
                    QStringLiteral("%1 想添加你为好友，是否同意？").arg(from),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    friend_service_.agreeFriend(from);
                }
            });
    connect(&chat_service_, &cloudvault::ChatService::messageReceived,
            this, &MainWindow::appendConversationMessage);
    connect(&chat_service_, &cloudvault::ChatService::historyLoaded,
            this, [this](const QString& peer, const QList<cloudvault::ChatMessage>& messages) {
                auto& state = conversations_[peer];
                state.messages = messages;
                state.history_loaded = true;
                if (selectedFriend() == peer) {
                    state.unread_count = 0;
                    renderConversation(peer);
                    filterFriendList(contact_search_edit_->text());
                }
            });
    connect(&chat_service_, &cloudvault::ChatService::historyLoadFailed,
            this, [this](const QString& peer, const QString& reason) {
                if (!peer.isEmpty() && selectedFriend() == peer) {
                    QMessageBox::warning(this, QStringLiteral("历史消息"), reason);
                }
            });
}

void MainWindow::refreshFriendList(const QList<QPair<QString, bool>>& friends) {
    friends_ = friends;
    filterFriendList(contact_search_edit_->text());
    refreshCurrentConversation();
}

void MainWindow::filterFriendList(const QString& keyword) {
    const QString current = selectedFriend();
    const QString trimmed = keyword.trimmed();
    QSignalBlocker blocker(contact_list_);
    contact_list_->clear();

    int index = 0;
    for (const auto& [username, online] : friends_) {
        if (!trimmed.isEmpty() && !username.contains(trimmed, Qt::CaseInsensitive)) {
            continue;
        }

        const auto it = conversations_.find(username);
        const bool has_messages = (it != conversations_.end() && !it->messages.isEmpty());
        const QString preview = has_messages
            ? it->messages.back().content
            : (online ? QStringLiteral("在线，可直接开始交流")
                      : QStringLiteral("离线，等待下次上线"));
        const QString time = has_messages
            ? formatMessageTime(it->messages.back().timestamp)
            : (online ? QStringLiteral("在线") : QStringLiteral("离线"));
        const int unread = (it != conversations_.end()) ? it->unread_count : 0;

        auto* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 64));
        item->setData(Qt::UserRole, username);
        item->setData(Qt::UserRole + 1, online);
        item->setData(Qt::UserRole + 2, time);
        item->setData(Qt::UserRole + 3, unread);

        const bool is_selected = username == current;
        auto* widget = createContactItemWidget(username, preview, time, unread, is_selected, contact_list_);
        contact_list_->addItem(item);
        contact_list_->setItemWidget(item, widget);
        if (is_selected) {
            item->setSelected(true);
        }
        ++index;
    }

    if (contact_list_->count() > 0 && selectedFriend().isEmpty()) {
        contact_list_->setCurrentRow(0);
    }

    if (contact_list_->count() == 0) {
        chat_title_label_->setText(QStringLiteral("联系人"));
        chat_status_label_->setText(QStringLiteral("选择一个好友开始聊天"));
        chat_status_label_->setStyleSheet(QStringLiteral("color: #9CA3AF; font-size: 11px;"));
        detail_contact_name_label_->setText(QStringLiteral("未选择联系人"));
        detail_contact_status_label_->setText(QStringLiteral("请选择左侧联系人"));
        file_path_label_->setText(QStringLiteral("📁 /未选择联系人/"));
        renderConversation(QString());
    }

    updateContactSelectionState();
    updateDeleteFriendAction();
    applySelectedFriend();
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

void MainWindow::updateDeleteFriendAction() {
    if (!delete_friend_btn_) {
        return;
    }

    const QString username = selectedFriend();
    delete_friend_btn_->setEnabled(!username.isEmpty());
    if (username.isEmpty()) {
        delete_friend_btn_->setText(QStringLiteral("删除好友"));
        return;
    }

    delete_friend_btn_->setText(QStringLiteral("删除 %1").arg(username));
}

void MainWindow::renderConversation(const QString& peer) {
    if (!message_list_layout_ || !message_list_widget_) {
        return;
    }

    while (message_list_layout_->count() > 0) {
        auto* item = message_list_layout_->takeAt(0);
        if (auto* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    if (peer.isEmpty()) {
        auto* empty = new QLabel(QStringLiteral("选择左侧好友后即可查看历史消息并开始聊天。"),
                                 message_list_widget_);
        empty->setObjectName(QStringLiteral("mutedLabel"));
        empty->setAlignment(Qt::AlignCenter);
        empty->setWordWrap(true);
        message_list_layout_->addStretch();
        message_list_layout_->addWidget(empty);
        message_list_layout_->addStretch();
        return;
    }

    const auto it = conversations_.find(peer);
    if (it == conversations_.end() || it->messages.isEmpty()) {
        auto* empty = new QLabel(QStringLiteral("还没有聊天记录，发一条消息开始吧。"),
                                 message_list_widget_);
        empty->setObjectName(QStringLiteral("mutedLabel"));
        empty->setAlignment(Qt::AlignCenter);
        message_list_layout_->addStretch();
        message_list_layout_->addWidget(empty);
        message_list_layout_->addStretch();
        return;
    }

    QString last_day;
    for (const auto& message : it->messages) {
        const auto ts = QDateTime::fromString(message.timestamp, QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
        const QString day_label = ts.isValid()
            ? ts.date().toString(QStringLiteral("yyyy-MM-dd"))
            : message.timestamp.left(10);
        if (!day_label.isEmpty() && day_label != last_day) {
            auto* day = new QLabel(day_label, message_list_widget_);
            day->setObjectName(QStringLiteral("dateDividerLabel"));
            day->setAlignment(Qt::AlignCenter);
            message_list_layout_->addWidget(day);
            last_day = day_label;
        }

        const bool outgoing = (message.from == current_username_);
        message_list_layout_->addWidget(createMessageBubble(
            message.content,
            formatMessageTime(message.timestamp),
            outgoing,
            outgoing ? QString() : message.from,
            message_list_widget_));
    }
    message_list_layout_->addStretch();

    if (message_scroll_area_ && message_scroll_area_->verticalScrollBar()) {
        QTimer::singleShot(0, this, [this] {
            if (message_scroll_area_ && message_scroll_area_->verticalScrollBar()) {
                auto* bar = message_scroll_area_->verticalScrollBar();
                bar->setValue(bar->maximum());
            }
        });
    }
}

void MainWindow::appendConversationMessage(const cloudvault::ChatMessage& message) {
    const QString peer = (message.from == current_username_) ? message.to : message.from;
    auto& state = conversations_[peer];
    state.messages.append(message);
    if (selectedFriend() == peer) {
        state.unread_count = 0;
        renderConversation(peer);
    } else if (message.from != current_username_) {
        ++state.unread_count;
    }
    filterFriendList(contact_search_edit_->text());
}

void MainWindow::sendCurrentMessage() {
    const QString peer = selectedFriend();
    const QString content = message_input_ ? message_input_->toPlainText().trimmed() : QString();
    if (peer.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("发送消息"),
                                 QStringLiteral("请先选择一个好友。"));
        return;
    }
    if (content.isEmpty()) {
        return;
    }
    if (content.size() > 2000) {
        QMessageBox::warning(this, QStringLiteral("发送消息"),
                             QStringLiteral("单条消息长度不能超过 2000 个字符。"));
        return;
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    appendConversationMessage(cloudvault::ChatMessage{
        current_username_,
        peer,
        content,
        timestamp,
    });
    chat_service_.sendMessage(peer, content);
    message_input_->clear();
}

void MainWindow::refreshCurrentConversation() {
    const QString peer = selectedFriend();
    renderConversation(peer);
}

QString MainWindow::formatMessageTime(const QString& timestamp) const {
    const auto ts = QDateTime::fromString(timestamp, QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    if (!ts.isValid()) {
        return timestamp;
    }

    const auto now = QDateTime::currentDateTime();
    if (ts.date() == now.date()) {
        return ts.time().toString(QStringLiteral("HH:mm"));
    }
    return ts.date().toString(QStringLiteral("MM-dd"));
}

void MainWindow::applySelectedFriend() {
    updateContactSelectionState();

    const QString username = selectedFriend();
    if (username.isEmpty()) {
        renderConversation(QString());
        updateDeleteFriendAction();
        return;
    }

    const bool online = selectedFriendOnline();
    chat_title_label_->setText(username);
    if (online) {
        chat_status_label_->setText(QStringLiteral("● 在线"));
        chat_status_label_->setStyleSheet(QStringLiteral("color: #22C55E; font-size: 11px;"));
    } else {
        chat_status_label_->setText(QStringLiteral("● 离线"));
        chat_status_label_->setStyleSheet(QStringLiteral("color: #9CA3AF; font-size: 11px;"));
    }
    detail_contact_name_label_->setText(username);
    detail_contact_status_label_->setText(
        online ? QStringLiteral("在线 · 可直接聊天")
               : QStringLiteral("离线 · 静默等待状态更新"));
    file_path_label_->setText(QStringLiteral("📁 /%1/").arg(username));
    auto& state = conversations_[username];
    state.unread_count = 0;
    renderConversation(username);
    if (!state.history_loaded) {
        chat_service_.loadHistory(username);
    }
    updateDeleteFriendAction();
}

void MainWindow::switchMainTab(int index) {
    center_stack_->setCurrentIndex(index);
    message_tab_btn_->setChecked(index == 0);
    file_tab_btn_->setChecked(index == 1);
    profile_tab_btn_->setChecked(index == 2);

    if (index == 0) {
        sidebar_panel_->show();
        detail_panel_->show();
        sidebar_title_label_->setText(QStringLiteral("消息"));
        contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
        sidebar_action_btn_->setText(QStringLiteral("🌐"));
        content_root_->layout()->setContentsMargins(0, 0, 0, 0);
    } else if (index == 1) {
        sidebar_panel_->show();
        detail_panel_->hide();
        sidebar_title_label_->setText(QStringLiteral("文件"));
        contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
        sidebar_action_btn_->setText(QStringLiteral("🌐"));
    } else {
        sidebar_panel_->hide();
        detail_panel_->hide();
    }
}

void MainWindow::confirmDeleteSelectedFriend() {
    const QString username = selectedFriend();
    if (username.isEmpty()) {
        return;
    }

    const auto reply = QMessageBox::question(
        this,
        QStringLiteral("删除好友"),
        QStringLiteral("确认删除好友 %1 吗？\n删除后双方好友列表都会移除彼此。").arg(username),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    friend_service_.deleteFriend(username);
}

void MainWindow::openOnlineUserDialog() {
    if (message_tab_btn_->isChecked() || file_tab_btn_->isChecked()) {
        OnlineUserDialog dialog(friends_, this);
        connect(&dialog, &OnlineUserDialog::refreshRequested,
                this, [this] { friend_service_.flushFriends(); });
        connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
                &dialog, &OnlineUserDialog::setUsers);
        connect(&dialog, &OnlineUserDialog::userChosen,
                this, [this](const QString& username) {
                    friend_service_.addFriend(username);
                });
        dialog.exec();
    }
}

void MainWindow::openShareFileDialog() {
    ShareFileDialog dialog(QStringLiteral("report.pdf"), friends_, this);
    connect(&dialog, &ShareFileDialog::shareConfirmed,
            this, [this](const QString&, const QStringList& targets) {
                if (!targets.isEmpty()) {
                    detail_file_target_label_->setText(
                        QStringLiteral("已选择分享对象：%1").arg(targets.join(", ")));
                }
            });
    dialog.exec();
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
