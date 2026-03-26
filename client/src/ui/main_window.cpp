// =============================================================
// client/src/ui/main_window.cpp
// 主窗口（三栏）
// =============================================================

#include "main_window.h"

#include "ui/online_user_dialog.h"
#include "ui/share_file_dialog.h"

#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
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

        QListWidget#fileList {
            border: none;
            outline: none;
            background: transparent;
        }

        QListWidget#fileList::item {
            margin-bottom: 8px;
            border: none;
            padding: 0;
        }

        QFrame#fileItem {
            background: #FFFFFF;
            border: 1px solid #E2E6EA;
            border-radius: 12px;
        }

        QFrame#fileItem[selected="true"] {
            background: #EFF6FF;
            border-color: #93C5FD;
        }

        QLabel#fileNameLabel {
            color: #111827;
            font-size: 14px;
            font-weight: 600;
        }

        QLabel#fileMetaLabel,
        QLabel#filePathHintLabel {
            color: #6B7280;
            font-size: 12px;
        }

        QLabel#fileStatusLabel {
            color: #2563EB;
            font-size: 12px;
            padding: 2px 0 0 0;
        }

        QLabel#fileStatusLabel[error="true"] {
            color: #DC2626;
        }

        QLabel#fileEmptyStateLabel {
            background: #F8FAFC;
            border: 1px dashed #CBD5E1;
            border-radius: 12px;
            color: #6B7280;
            font-size: 13px;
            padding: 24px 18px;
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
            border: 1px solid #E2E6EA;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 14px;
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
                             QWidget* parent = nullptr) {
    auto* wrap = new QWidget(parent);
    auto* outer = new QVBoxLayout(wrap);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(4);

    auto* bubble_row = new QHBoxLayout();
    bubble_row->setContentsMargins(0, 0, 0, 0);

    auto* bubble = new QLabel(text, wrap);
    bubble->setObjectName(outgoing ? QStringLiteral("outgoingBubble")
                                   : QStringLiteral("incomingBubble"));
    bubble->setWordWrap(true);
    bubble->setMaximumWidth(320);

    if (outgoing) {
        bubble_row->addStretch();
        bubble_row->addWidget(bubble);
    } else {
        bubble_row->addWidget(bubble);
        bubble_row->addStretch();
    }
    outer->addLayout(bubble_row);

    auto* time_label = new QLabel(time, wrap);
    time_label->setObjectName(QStringLiteral("timeLabel"));
    outer->addWidget(time_label, 0, outgoing ? Qt::AlignRight : Qt::AlignLeft);
    return wrap;
}

QWidget* createFileBubble(const QString& title,
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

QWidget* createFileItemWidget(const cloudvault::FileEntry& entry,
                              bool selected,
                              QWidget* parent = nullptr) {
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("fileItem"));
    frame->setProperty("selected", selected);

    auto* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(12);

    auto* icon = new QLabel(entry.is_dir ? QStringLiteral("📁")
                                         : QStringLiteral("📄"), frame);
    icon->setObjectName(QStringLiteral("centerTitleLabel"));
    icon->setFixedWidth(28);
    layout->addWidget(icon, 0, Qt::AlignTop);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(4);

    auto* name_label = new QLabel(entry.name, frame);
    name_label->setObjectName(QStringLiteral("fileNameLabel"));
    text_layout->addWidget(name_label);

    const QString meta = entry.is_dir
        ? QStringLiteral("文件夹 · %1").arg(entry.modified_at)
        : QStringLiteral("%1 · %2").arg(formatFileSize(entry.size), entry.modified_at);
    auto* meta_label = new QLabel(meta, frame);
    meta_label->setObjectName(QStringLiteral("fileMetaLabel"));
    text_layout->addWidget(meta_label);

    auto* path_label = new QLabel(entry.path, frame);
    path_label->setObjectName(QStringLiteral("filePathHintLabel"));
    text_layout->addWidget(path_label);
    layout->addLayout(text_layout, 1);
    frame->setToolTip(entry.path);

    return frame;
}

} // namespace

MainWindow::MainWindow(const QString& username,
                       cloudvault::FriendService& friend_service,
                       cloudvault::FileService& file_service,
                       QWidget* parent)
    : QMainWindow(parent),
      current_username_(username),
      friend_service_(friend_service),
      file_service_(file_service) {
    setupUi();
    connectSignals();
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

        auto* message_area = new QWidget(page);
        auto* msg_layout = new QVBoxLayout(message_area);
        msg_layout->setContentsMargins(4, 2, 4, 2);
        msg_layout->setSpacing(8);
        auto* day_label = new QLabel(QStringLiteral("·今天·"), message_area);
        day_label->setObjectName(QStringLiteral("dateDividerLabel"));
        day_label->setAlignment(Qt::AlignCenter);
        msg_layout->addWidget(day_label);
        msg_layout->addWidget(createMessageBubble(QStringLiteral("嘿，项目文档准备好了吗？"),
                                                  QStringLiteral("14:28"),
                                                  false,
                                                  message_area));
        msg_layout->addWidget(createMessageBubble(QStringLiteral("快了，马上发给你。"),
                                                  QStringLiteral("14:29"),
                                                  true,
                                                  message_area));
        msg_layout->addWidget(createFileBubble(QStringLiteral("需求文档_v3.pdf"),
                                               QStringLiteral("2.4 MB · PDF"),
                                               QStringLiteral("14:30"),
                                               message_area));
        msg_layout->addWidget(createMessageBubble(QStringLiteral("收到，谢谢！我看一下。"),
                                                  QStringLiteral("14:31"),
                                                  false,
                                                  message_area));
        msg_layout->addStretch();
        page_layout->addWidget(message_area, 1);

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
        auto* send_btn = new QPushButton(QStringLiteral("➤"), compose);
        send_btn->setObjectName(QStringLiteral("primaryButton"));
        send_btn->setFixedWidth(42);
        compose_layout->addWidget(send_btn);
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
        file_path_label_ = new QLabel(QStringLiteral("📁 /"), header);
        file_path_label_->setObjectName(QStringLiteral("secondaryLabel"));
        file_status_label_ = new QLabel(QStringLiteral("正在加载个人文件空间…"), header);
        file_status_label_->setObjectName(QStringLiteral("fileStatusLabel"));
        file_status_label_->setWordWrap(true);
        header_layout->addWidget(title);
        header_layout->addWidget(file_path_label_);
        header_layout->addWidget(file_status_label_);
        page_layout->addWidget(header);

        auto* toolbar = new QFrame(page);
        toolbar->setObjectName(QStringLiteral("chatHeader"));
        auto* toolbar_layout = new QHBoxLayout(toolbar);
        toolbar_layout->setContentsMargins(10, 8, 10, 8);
        toolbar_layout->setSpacing(8);
        file_search_edit_ = new QLineEdit(toolbar);
        file_search_edit_->setPlaceholderText(QStringLiteral("搜索文件，回车执行…"));
        toolbar_layout->addWidget(file_search_edit_, 1);
        file_back_btn_ = new QPushButton(QStringLiteral("↑ 返回"), toolbar);
        file_back_btn_->setObjectName(QStringLiteral("lightButton"));
        file_refresh_btn_ = new QPushButton(QStringLiteral("⟳ 刷新"), toolbar);
        file_refresh_btn_->setObjectName(QStringLiteral("lightButton"));
        file_create_btn_ = new QPushButton(QStringLiteral("+ 新建"), toolbar);
        file_create_btn_->setObjectName(QStringLiteral("primaryButton"));
        toolbar_layout->addWidget(file_back_btn_);
        toolbar_layout->addWidget(file_refresh_btn_);
        toolbar_layout->addWidget(file_create_btn_);
        page_layout->addWidget(toolbar);

        file_list_ = new QListWidget(page);
        file_list_->setObjectName(QStringLiteral("fileList"));
        file_list_->setSpacing(0);
        page_layout->addWidget(file_list_, 1);

        file_empty_state_label_ = new QLabel(
            QStringLiteral("当前目录为空。\n点击“+ 新建”创建第一个文件夹，或使用搜索查看历史结果。"),
            page);
        file_empty_state_label_->setObjectName(QStringLiteral("fileEmptyStateLabel"));
        file_empty_state_label_->setAlignment(Qt::AlignCenter);
        file_empty_state_label_->hide();
        page_layout->addWidget(file_empty_state_label_);

        auto* selection_card = new QFrame(page);
        selection_card->setObjectName(QStringLiteral("detailCard"));
        auto* selection_layout = new QVBoxLayout(selection_card);
        selection_layout->setContentsMargins(12, 12, 12, 12);
        selection_layout->setSpacing(8);
        detail_file_target_label_ = new QLabel(QStringLiteral("选中：未选择文件"), selection_card);
        detail_file_target_label_->setObjectName(QStringLiteral("sectionTitleLabel"));
        selection_layout->addWidget(detail_file_target_label_);
        detail_file_meta_label_ = new QLabel(QStringLiteral("支持：双击进入目录 / 重命名 / 移动 / 删除"), selection_card);
        detail_file_meta_label_->setObjectName(QStringLiteral("secondaryLabel"));
        detail_file_meta_label_->setWordWrap(true);
        selection_layout->addWidget(detail_file_meta_label_);

        auto* file_action_row = new QHBoxLayout();
        file_action_row->setSpacing(8);
        file_rename_btn_ = new QPushButton(QStringLiteral("✏ 重命名"), selection_card);
        file_rename_btn_->setObjectName(QStringLiteral("lightButton"));
        file_move_btn_ = new QPushButton(QStringLiteral("✂ 移动"), selection_card);
        file_move_btn_->setObjectName(QStringLiteral("lightButton"));
        file_delete_btn_ = new QPushButton(QStringLiteral("🗑 删除"), selection_card);
        file_delete_btn_->setObjectName(QStringLiteral("dangerButton"));
        file_action_row->addWidget(file_rename_btn_);
        file_action_row->addWidget(file_move_btn_);
        file_action_row->addWidget(file_delete_btn_);
        selection_layout->addLayout(file_action_row);
        page_layout->addWidget(selection_card);

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
    connect(file_back_btn_, &QPushButton::clicked, this, &MainWindow::openCurrentParentDirectory);
    connect(file_refresh_btn_, &QPushButton::clicked,
            this, [this] {
                setFileStatus(file_search_mode_ && !current_file_query_.isEmpty()
                                  ? QStringLiteral("正在刷新搜索结果…")
                                  : QStringLiteral("正在刷新当前目录…"));
                if (file_search_mode_ && !current_file_query_.isEmpty()) {
                    file_service_.search(current_file_query_);
                } else {
                    file_service_.listFiles(current_file_path_);
                }
            });
    connect(file_create_btn_, &QPushButton::clicked, this, &MainWindow::createDirectory);
    connect(file_search_edit_, &QLineEdit::returnPressed, this, &MainWindow::runFileSearch);
    connect(file_search_edit_, &QLineEdit::textChanged, this, &MainWindow::clearFileSearchIfNeeded);
    connect(file_list_, &QListWidget::itemSelectionChanged, this, &MainWindow::applySelectedFile);
    connect(file_list_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                if (!item || !item->data(Qt::UserRole + 1).toBool()) {
                    setFileStatus(QStringLiteral("当前仅支持双击目录进入下一层。"));
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

    connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
            this, &MainWindow::refreshFriendList);
    connect(&friend_service_, &cloudvault::FriendService::friendAdded,
            this, [this](const QString&) { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendDeleted,
            this, [this](const QString&) { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendRequestSent,
            this, [this](const QString& target) {
                QMessageBox::information(this, QStringLiteral("好友申请"),
                    QStringLiteral("已向 %1 发送好友申请，等待对方同意。").arg(target));
            });
    connect(&friend_service_, &cloudvault::FriendService::friendAddFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("添加失败"), reason);
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
    connect(&file_service_, &cloudvault::FileService::filesListed,
            this, &MainWindow::refreshFileList);
    connect(&file_service_, &cloudvault::FileService::fileListFailed,
            this, [this](const QString& reason) {
                setFileStatus(reason, true);
            });
    connect(&file_service_, &cloudvault::FileService::searchCompleted,
            this, [this](const QString& keyword, const cloudvault::FileEntries& entries) {
                file_search_mode_ = true;
                current_file_query_ = keyword;
                refreshFileList(QStringLiteral("搜索：%1").arg(keyword), entries);
            });
    connect(&file_service_, &cloudvault::FileService::searchFailed,
            this, [this](const QString& reason) {
                setFileStatus(reason, true);
            });
    connect(&file_service_, &cloudvault::FileService::fileOperationSucceeded,
            this, [this](const QString& message) {
                setFileStatus(message);
                file_search_mode_ = false;
                current_file_query_.clear();
                file_search_edit_->clear();
                file_service_.listFiles(current_file_path_);
            });
    connect(&file_service_, &cloudvault::FileService::fileOperationFailed,
            this, [this](const QString& message) {
                setFileStatus(message, true);
            });
}

void MainWindow::refreshFriendList(const QList<QPair<QString, bool>>& friends) {
    friends_ = friends;
    filterFriendList(contact_search_edit_->text());
}

void MainWindow::filterFriendList(const QString& keyword) {
    const QString current = selectedFriend();
    const QString trimmed = keyword.trimmed();
    contact_list_->clear();

    int index = 0;
    for (const auto& [username, online] : friends_) {
        if (!trimmed.isEmpty() && !username.contains(trimmed, Qt::CaseInsensitive)) {
            continue;
        }

        const QString preview = online ? QStringLiteral("在线，可直接开始交流")
                                       : QStringLiteral("离线，等待下次上线");
        const QString time = online ? QStringLiteral("14:32") : QStringLiteral("周一");
        const int unread = (index == 0 && online) ? 3 : 0;

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
        chat_title_label_->setText(QStringLiteral("● 联系人"));
        chat_status_label_->setText(QStringLiteral("在线"));
        detail_contact_name_label_->setText(QStringLiteral("未选择联系人"));
        detail_contact_status_label_->setText(QStringLiteral("请选择左侧联系人"));
    }

    updateContactSelectionState();
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
        return;
    }

    const bool online = selectedFriendOnline();
    chat_title_label_->setText(QStringLiteral("● %1").arg(username));
    chat_status_label_->setText(online ? QStringLiteral("在线") : QStringLiteral("离线"));
    detail_contact_name_label_->setText(username);
    detail_contact_status_label_->setText(
        online ? QStringLiteral("在线 · 可直接聊天")
               : QStringLiteral("离线 · 静默等待状态更新"));
}

void MainWindow::setFileStatus(const QString& message, bool error) {
    file_status_label_->setProperty("error", error);
    file_status_label_->setText(message);
    repolish(file_status_label_);
}

void MainWindow::refreshFileList(const QString& path,
                                 const cloudvault::FileEntries& entries) {
    if (!file_search_mode_) {
        current_file_path_ = path.isEmpty() ? QStringLiteral("/") : path;
    }
    current_file_entries_ = entries;
    file_list_->clear();

    for (const auto& entry : entries) {
        auto* item = new QListWidgetItem(file_list_);
        item->setSizeHint(QSize(0, 76));
        item->setData(Qt::UserRole, entry.path);
        item->setData(Qt::UserRole + 1, entry.is_dir);
        item->setData(Qt::UserRole + 2, entry.name);
        auto* widget = createFileItemWidget(entry, false, file_list_);
        file_list_->setItemWidget(item, widget);
    }

    if (file_search_mode_) {
        file_path_label_->setText(QStringLiteral("🔎 搜索结果：%1").arg(current_file_query_));
    } else {
        file_path_label_->setText(QStringLiteral("📁 %1").arg(current_file_path_));
    }
    file_path_label_->setToolTip(file_search_mode_ ? current_file_query_ : current_file_path_);
    file_back_btn_->setEnabled(file_search_mode_ || current_file_path_ != QStringLiteral("/"));
    file_empty_state_label_->setVisible(file_list_->count() == 0);

    if (file_list_->count() > 0) {
        file_list_->setCurrentRow(0);
        setFileStatus(file_search_mode_
                          ? QStringLiteral("搜索完成，共找到 %1 项。").arg(file_list_->count())
                          : QStringLiteral("目录已刷新，共 %1 项。").arg(file_list_->count()));
    } else {
        detail_file_target_label_->setText(QStringLiteral("选中：未选择文件"));
        detail_file_meta_label_->setText(file_search_mode_
            ? QStringLiteral("没有匹配结果，修改关键词后回车重试。")
            : QStringLiteral("当前目录为空，可以新建文件夹开始管理。"));
        setFileStatus(file_search_mode_
                          ? QStringLiteral("没有找到与“%1”匹配的文件。").arg(current_file_query_)
                          : QStringLiteral("当前目录为空。"));
    }

    updateFileSelectionState();
}

void MainWindow::updateFileSelectionState() {
    for (int i = 0; i < file_list_->count(); ++i) {
        auto* item = file_list_->item(i);
        auto* widget = file_list_->itemWidget(item);
        if (!widget) {
            continue;
        }
        widget->setProperty("selected", item->isSelected());
        repolish(widget);
    }

    const bool has_selection = file_list_->currentItem() != nullptr;
    file_rename_btn_->setEnabled(has_selection);
    file_move_btn_->setEnabled(has_selection);
    file_delete_btn_->setEnabled(has_selection);
}

void MainWindow::applySelectedFile() {
    updateFileSelectionState();

    const QString path = selectedFilePath();
    if (path.isEmpty()) {
        detail_file_target_label_->setText(QStringLiteral("选中：未选择文件"));
        detail_file_meta_label_->setText(QStringLiteral("请选择一个文件或文件夹。"));
        return;
    }

    const bool is_dir = selectedFileIsDir();
    const QString name = file_list_->currentItem()->data(Qt::UserRole + 2).toString();
    detail_file_target_label_->setText(QStringLiteral("选中：%1").arg(name));
    detail_file_meta_label_->setText(
        is_dir
            ? QStringLiteral("目录 · 路径：%1\n双击可进入；支持重命名、移动、删除。").arg(path)
            : QStringLiteral("文件 · 路径：%1\n支持重命名、移动、删除。").arg(path));
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
        this, QStringLiteral("新建文件夹"),
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
        this, QStringLiteral("重命名"),
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
        this, QStringLiteral("移动"),
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
        this, QStringLiteral("删除"),
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

bool MainWindow::fileListShowingSearchResults() const {
    return file_search_mode_;
}
