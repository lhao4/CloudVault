// =============================================================
// client/src/ui/main_window.cpp
// 主窗口（三栏）
// =============================================================

#include "main_window.h"

#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QScrollArea>
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
            font-family: "DM Sans", "PingFang SC", "Microsoft YaHei", sans-serif;
            color: #111827;
        }

        QFrame#sidebarPanel,
        QFrame#centerPanel,
        QFrame#detailPanel,
        QFrame#chatHeaderCard,
        QFrame#messageComposerCard,
        QFrame#profileCard,
        QFrame#detailCard,
        QFrame#fileToolbarCard,
        QFrame#fileSelectionCard {
            background: #FFFFFF;
            border: 1px solid #E2E6EA;
            border-radius: 12px;
        }

        QFrame#sidebarPanel,
        QFrame#detailPanel {
            background: #FFFFFF;
        }

        QLabel#columnTitleLabel,
        QLabel#centerTitleLabel,
        QLabel#detailTitleLabel {
            color: #111827;
            font-size: 18px;
            font-weight: 700;
        }

        QLabel#sectionTitleLabel {
            color: #111827;
            font-size: 15px;
            font-weight: 600;
        }

        QLabel#subtitleLabel,
        QLabel#secondaryTextLabel,
        QLabel#detailHintLabel,
        QLabel#metaLabel,
        QLabel#timeLabel,
        QLabel#fileMetaLabel {
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
            color: #111827;
        }

        QLineEdit:focus {
            background: #FFFFFF;
            border: 1px solid #3B82F6;
        }

        QLineEdit[mono="true"] {
            font-family: "DM Mono", monospace;
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
            color: #111827;
        }

        QPushButton#iconButton:hover {
            background: #EFF6FF;
            border-color: #3B82F6;
            color: #2563EB;
        }

        QPushButton#lightButton {
            background: #F0F2F5;
            color: #111827;
            border: 1px solid #E2E6EA;
        }

        QPushButton#lightButton:hover {
            background: #EFF6FF;
            border-color: #3B82F6;
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
            background: transparent;
            border: none;
            outline: none;
        }

        QListWidget#contactList::item {
            padding: 10px 12px;
            margin-bottom: 8px;
            border-radius: 10px;
            border: 1px solid transparent;
            background: transparent;
        }

        QListWidget#contactList::item:selected {
            background: #EFF6FF;
            border: 1px solid #BFDBFE;
            color: #111827;
        }

        QPushButton#bottomTabButton {
            min-height: 42px;
            border-radius: 10px;
            background: transparent;
            color: #6B7280;
            border: none;
            font-size: 14px;
            font-weight: 600;
        }

        QPushButton#bottomTabButton:hover {
            background: #F0F2F5;
            color: #111827;
        }

        QPushButton#bottomTabButton:checked {
            background: #EFF6FF;
            color: #3B82F6;
        }

        QLabel#avatarLabel {
            min-width: 40px;
            min-height: 40px;
            max-width: 40px;
            max-height: 40px;
            border-radius: 20px;
            background: #E0ECFF;
            color: #2563EB;
            font-size: 14px;
            font-weight: 700;
        }

        QLabel#onlineDotLabel {
            color: #22C55E;
            font-size: 11px;
        }

        QLabel#offlineDotLabel {
            color: #9CA3AF;
            font-size: 11px;
        }

        QLabel#incomingBubble {
            background: #FFFFFF;
            border: 1px solid #E2E6EA;
            border-radius: 12px;
            padding: 10px 12px;
            color: #111827;
            font-size: 14px;
        }

        QLabel#outgoingBubble {
            background: #3B82F6;
            border: none;
            border-radius: 12px;
            padding: 10px 12px;
            color: white;
            font-size: 14px;
        }

        QLabel#fileBubble {
            background: #FFFFFF;
            border: 1px solid #E2E6EA;
            border-radius: 12px;
            padding: 12px 14px;
            color: #111827;
            font-size: 14px;
        }

        QLabel#dateDividerLabel {
            color: #9CA3AF;
            font-size: 12px;
            padding: 6px 0;
        }

        QFrame#progressBarBg {
            background: #E5E7EB;
            border-radius: 6px;
            border: none;
        }

        QFrame#progressBarFill {
            background: #3B82F6;
            border-radius: 6px;
            border: none;
        }

        QTextEdit#messageInput {
            min-height: 44px;
            max-height: 90px;
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
    return button;
}

QPushButton* createBottomTabButton(const QString& text, QWidget* parent = nullptr) {
    auto* button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("bottomTabButton"));
    button->setCheckable(true);
    return button;
}

QLabel* createMetaLabel(const QString& text, QWidget* parent = nullptr) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("metaLabel"));
    return label;
}

QFrame* createFileChip(const QString& icon,
                       const QString& title,
                       const QString& meta,
                       QWidget* parent = nullptr) {
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("detailCard"));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(4);

    auto* title_label = new QLabel(QStringLiteral("%1 %2").arg(icon, title), card);
    title_label->setObjectName(QStringLiteral("sectionTitleLabel"));
    layout->addWidget(title_label);

    auto* meta_label = new QLabel(meta, card);
    meta_label->setObjectName(QStringLiteral("fileMetaLabel"));
    layout->addWidget(meta_label);
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

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    auto* bubble = new QLabel(text, wrap);
    bubble->setWordWrap(true);
    bubble->setMaximumWidth(320);
    bubble->setObjectName(outgoing ? QStringLiteral("outgoingBubble")
                                   : QStringLiteral("incomingBubble"));

    if (outgoing) {
        row->addStretch();
        row->addWidget(bubble);
    } else {
        row->addWidget(bubble);
        row->addStretch();
    }
    outer->addLayout(row);

    auto* time_row = new QHBoxLayout();
    time_row->setContentsMargins(0, 0, 0, 0);
    time_row->setSpacing(0);
    auto* time_label = new QLabel(time, wrap);
    time_label->setObjectName(QStringLiteral("timeLabel"));

    if (outgoing) {
        time_row->addStretch();
        time_row->addWidget(time_label);
    } else {
        time_row->addWidget(time_label);
        time_row->addStretch();
    }
    outer->addLayout(time_row);
    return wrap;
}

QWidget* createFileBubble(const QString& title,
                          const QString& meta,
                          const QString& time,
                          QWidget* parent = nullptr) {
    auto* wrap = new QWidget(parent);
    auto* outer = new QVBoxLayout(wrap);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(4);

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);

    auto* bubble = new QLabel(QStringLiteral("📄 %1\n%2 ↗").arg(title, meta), wrap);
    bubble->setObjectName(QStringLiteral("fileBubble"));
    bubble->setWordWrap(true);
    bubble->setMaximumWidth(340);
    row->addWidget(bubble);
    row->addStretch();
    outer->addLayout(row);

    auto* time_label = new QLabel(time, wrap);
    time_label->setObjectName(QStringLiteral("timeLabel"));
    outer->addWidget(time_label, 0, Qt::AlignLeft);
    return wrap;
}

QFrame* createDetailCard(const QString& title,
                         const QString& line1,
                         const QString& line2,
                         QWidget* parent = nullptr) {
    auto* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("detailCard"));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(4);

    auto* title_label = new QLabel(title, card);
    title_label->setObjectName(QStringLiteral("sectionTitleLabel"));
    layout->addWidget(title_label);

    auto* line1_label = new QLabel(line1, card);
    line1_label->setObjectName(QStringLiteral("secondaryTextLabel"));
    layout->addWidget(line1_label);

    auto* line2_label = new QLabel(line2, card);
    line2_label->setObjectName(QStringLiteral("secondaryTextLabel"));
    layout->addWidget(line2_label);
    return card;
}

} // namespace

MainWindow::MainWindow(const QString& username,
                       cloudvault::FriendService& friend_service,
                       QWidget* parent)
    : QMainWindow(parent),
      current_username_(username),
      friend_service_(friend_service) {
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
    resize(1180, 760);
    setMinimumSize(900, 580);
    setStyleSheet(mainWindowStyle());

    auto* shell = new QWidget(this);
    shell->setObjectName(QStringLiteral("windowShell"));
    auto* root = new QHBoxLayout(shell);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(16);

    auto* sidebar = new QFrame(shell);
    sidebar->setObjectName(QStringLiteral("sidebarPanel"));
    sidebar->setFixedWidth(260);
    auto* sidebar_layout = new QVBoxLayout(sidebar);
    sidebar_layout->setContentsMargins(18, 18, 18, 18);
    sidebar_layout->setSpacing(14);

    auto* sidebar_header = new QHBoxLayout();
    auto* sidebar_title = new QLabel(QStringLiteral("消息"), sidebar);
    sidebar_title->setObjectName(QStringLiteral("columnTitleLabel"));
    sidebar_header->addWidget(sidebar_title);
    sidebar_header->addStretch();
    sidebar_header->addWidget(createIconButton(QStringLiteral("✏"), sidebar));
    sidebar_layout->addLayout(sidebar_header);

    contact_search_edit_ = new QLineEdit(sidebar);
    contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
    sidebar_layout->addWidget(contact_search_edit_);

    contact_list_ = new QListWidget(sidebar);
    contact_list_->setObjectName(QStringLiteral("contactList"));
    sidebar_layout->addWidget(contact_list_, 1);

    auto* bottom_nav = new QHBoxLayout();
    bottom_nav->setSpacing(6);
    message_tab_btn_ = createBottomTabButton(QStringLiteral("💬 消息"), sidebar);
    file_tab_btn_ = createBottomTabButton(QStringLiteral("📁 文件"), sidebar);
    profile_tab_btn_ = createBottomTabButton(QStringLiteral("👤 我"), sidebar);
    bottom_nav->addWidget(message_tab_btn_);
    bottom_nav->addWidget(file_tab_btn_);
    bottom_nav->addWidget(profile_tab_btn_);
    sidebar_layout->addLayout(bottom_nav);

    root->addWidget(sidebar);

    auto* center_panel = new QFrame(shell);
    center_panel->setObjectName(QStringLiteral("centerPanel"));
    auto* center_layout = new QVBoxLayout(center_panel);
    center_layout->setContentsMargins(0, 0, 0, 0);
    center_layout->setSpacing(14);

    center_stack_ = new QStackedWidget(center_panel);

    {
        auto* message_page = new QWidget(center_stack_);
        auto* message_layout = new QVBoxLayout(message_page);
        message_layout->setContentsMargins(0, 0, 0, 0);
        message_layout->setSpacing(14);

        auto* chat_header = new QFrame(message_page);
        chat_header->setObjectName(QStringLiteral("chatHeaderCard"));
        auto* chat_header_layout = new QHBoxLayout(chat_header);
        chat_header_layout->setContentsMargins(18, 16, 18, 16);
        chat_header_layout->setSpacing(12);

        auto* chat_title_wrap = new QVBoxLayout();
        chat_title_wrap->setSpacing(3);
        chat_title_label_ = new QLabel(QStringLiteral("选择一个联系人"), chat_header);
        chat_title_label_->setObjectName(QStringLiteral("centerTitleLabel"));
        chat_title_wrap->addWidget(chat_title_label_);
        chat_status_label_ = new QLabel(QStringLiteral("在线"), chat_header);
        chat_status_label_->setObjectName(QStringLiteral("subtitleLabel"));
        chat_title_wrap->addWidget(chat_status_label_);
        chat_header_layout->addLayout(chat_title_wrap, 1);
        chat_header_layout->addWidget(createIconButton(QStringLiteral("📎"), chat_header));
        chat_header_layout->addWidget(createIconButton(QStringLiteral("⋯"), chat_header));
        message_layout->addWidget(chat_header);

        auto* chat_body = new QFrame(message_page);
        chat_body->setObjectName(QStringLiteral("centerPanel"));
        auto* chat_body_layout = new QVBoxLayout(chat_body);
        chat_body_layout->setContentsMargins(18, 8, 18, 8);
        chat_body_layout->setSpacing(12);
        auto* today_label = new QLabel(QStringLiteral("· 今天 ·"), chat_body);
        today_label->setObjectName(QStringLiteral("dateDividerLabel"));
        today_label->setAlignment(Qt::AlignCenter);
        chat_body_layout->addWidget(today_label);
        chat_body_layout->addWidget(createMessageBubble(
            QStringLiteral("嘿，项目文档准备好了吗？"), QStringLiteral("14:28"), false, chat_body));
        chat_body_layout->addWidget(createMessageBubble(
            QStringLiteral("快了，马上发给你。"), QStringLiteral("14:29"), true, chat_body));
        chat_body_layout->addWidget(createFileBubble(
            QStringLiteral("需求文档_v3.pdf"), QStringLiteral("2.4 MB · PDF"), QStringLiteral("14:30"), chat_body));
        chat_body_layout->addWidget(createMessageBubble(
            QStringLiteral("收到，谢谢！我看一下。"), QStringLiteral("14:31"), false, chat_body));
        chat_body_layout->addStretch();
        message_layout->addWidget(chat_body, 1);

        auto* composer = new QFrame(message_page);
        composer->setObjectName(QStringLiteral("messageComposerCard"));
        auto* composer_layout = new QHBoxLayout(composer);
        composer_layout->setContentsMargins(12, 10, 12, 10);
        composer_layout->setSpacing(8);
        composer_layout->addWidget(createIconButton(QStringLiteral("😊"), composer));
        composer_layout->addWidget(createIconButton(QStringLiteral("📎"), composer));
        auto* input = new QTextEdit(composer);
        input->setObjectName(QStringLiteral("messageInput"));
        input->setPlaceholderText(QStringLiteral("输入消息…"));
        composer_layout->addWidget(input, 1);
        auto* send_btn = new QPushButton(QStringLiteral("➤"), composer);
        send_btn->setObjectName(QStringLiteral("primaryButton"));
        send_btn->setFixedWidth(44);
        composer_layout->addWidget(send_btn);
        message_layout->addWidget(composer);

        center_stack_->addWidget(message_page);
    }

    {
        auto* file_page = new QWidget(center_stack_);
        auto* file_layout = new QVBoxLayout(file_page);
        file_layout->setContentsMargins(0, 0, 0, 0);
        file_layout->setSpacing(14);

        auto* path_header = new QFrame(file_page);
        path_header->setObjectName(QStringLiteral("chatHeaderCard"));
        auto* path_layout = new QHBoxLayout(path_header);
        path_layout->setContentsMargins(18, 16, 18, 16);
        path_layout->setSpacing(12);
        auto* file_title_wrap = new QVBoxLayout();
        file_title_wrap->setSpacing(3);
        auto* file_title = new QLabel(QStringLiteral("文件"), path_header);
        file_title->setObjectName(QStringLiteral("centerTitleLabel"));
        file_title_wrap->addWidget(file_title);
        file_path_label_ = new QLabel(QStringLiteral("📁 /未选择联系人/"), path_header);
        file_path_label_->setObjectName(QStringLiteral("subtitleLabel"));
        file_title_wrap->addWidget(file_path_label_);
        path_layout->addLayout(file_title_wrap, 1);
        file_layout->addWidget(path_header);

        auto* toolbar = new QFrame(file_page);
        toolbar->setObjectName(QStringLiteral("fileToolbarCard"));
        auto* toolbar_layout = new QHBoxLayout(toolbar);
        toolbar_layout->setContentsMargins(12, 10, 12, 10);
        toolbar_layout->setSpacing(8);
        auto* search = new QLineEdit(toolbar);
        search->setPlaceholderText(QStringLiteral("搜索文件…"));
        toolbar_layout->addWidget(search, 1);
        auto* clear_btn = createIconButton(QStringLiteral("✕"), toolbar);
        toolbar_layout->addWidget(clear_btn);
        file_layout->addWidget(toolbar);

        auto* file_list_card = new QFrame(file_page);
        file_list_card->setObjectName(QStringLiteral("centerPanel"));
        auto* file_list_layout = new QVBoxLayout(file_list_card);
        file_list_layout->setContentsMargins(18, 18, 18, 18);
        file_list_layout->setSpacing(12);
        file_list_layout->addWidget(createDetailCard(QStringLiteral("📁 documents"),
                                                     QStringLiteral("最近修改：今天"),
                                                     QStringLiteral("2 个文件"), file_list_card));
        file_list_layout->addWidget(createDetailCard(QStringLiteral("📁 images"),
                                                     QStringLiteral("最近修改：昨天"),
                                                     QStringLiteral("12 张图片"), file_list_card));
        file_list_layout->addWidget(createDetailCard(QStringLiteral("📄 report.pdf"),
                                                     QStringLiteral("2.4MB"),
                                                     QStringLiteral("今天上传"), file_list_card));
        file_list_layout->addWidget(createDetailCard(QStringLiteral("📄 data.xlsx"),
                                                     QStringLiteral("856KB"),
                                                     QStringLiteral("昨天上传"), file_list_card));
        file_list_layout->addStretch();
        file_layout->addWidget(file_list_card, 1);
        center_stack_->addWidget(file_page);
    }

    {
        auto* profile_page = new QWidget(center_stack_);
        auto* profile_layout = new QVBoxLayout(profile_page);
        profile_layout->setContentsMargins(0, 0, 0, 0);
        profile_layout->setSpacing(14);
        profile_layout->addStretch();

        auto* profile_card = new QFrame(profile_page);
        profile_card->setObjectName(QStringLiteral("profileCard"));
        profile_card->setMaximumWidth(420);
        auto* card_layout = new QVBoxLayout(profile_card);
        card_layout->setContentsMargins(28, 28, 28, 28);
        card_layout->setSpacing(14);

        auto* avatar = new QLabel(current_username_.left(1).toUpper(), profile_card);
        avatar->setObjectName(QStringLiteral("avatarLabel"));
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setFixedSize(72, 72);
        card_layout->addWidget(avatar, 0, Qt::AlignHCenter);

        profile_name_label_ = new QLabel(current_username_, profile_card);
        profile_name_label_->setObjectName(QStringLiteral("centerTitleLabel"));
        profile_name_label_->setAlignment(Qt::AlignCenter);
        card_layout->addWidget(profile_name_label_);

        profile_id_label_ = new QLabel(QStringLiteral("@%1").arg(current_username_), profile_card);
        profile_id_label_->setObjectName(QStringLiteral("subtitleLabel"));
        profile_id_label_->setAlignment(Qt::AlignCenter);
        card_layout->addWidget(profile_id_label_);

        auto* nickname_edit = new QLineEdit(profile_card);
        nickname_edit->setPlaceholderText(QStringLiteral("昵称"));
        nickname_edit->setText(current_username_);
        card_layout->addWidget(nickname_edit);

        auto* signature_edit = new QLineEdit(profile_card);
        signature_edit->setPlaceholderText(QStringLiteral("个性签名"));
        card_layout->addWidget(signature_edit);

        auto* button_row = new QHBoxLayout();
        auto* save_btn = new QPushButton(QStringLiteral("保存"), profile_card);
        save_btn->setObjectName(QStringLiteral("primaryButton"));
        auto* cancel_btn = new QPushButton(QStringLiteral("取消"), profile_card);
        cancel_btn->setObjectName(QStringLiteral("lightButton"));
        button_row->addWidget(save_btn);
        button_row->addWidget(cancel_btn);
        card_layout->addLayout(button_row);

        auto* logout_btn = new QPushButton(QStringLiteral("退出登录"), profile_card);
        logout_btn->setObjectName(QStringLiteral("dangerButton"));
        card_layout->addWidget(logout_btn);

        profile_layout->addWidget(profile_card, 0, Qt::AlignHCenter);
        profile_layout->addStretch();
        center_stack_->addWidget(profile_page);
    }

    center_layout->addWidget(center_stack_);
    root->addWidget(center_panel, 1);

    auto* detail_panel = new QFrame(shell);
    detail_panel->setObjectName(QStringLiteral("detailPanel"));
    detail_panel->setFixedWidth(220);
    auto* detail_layout = new QVBoxLayout(detail_panel);
    detail_layout->setContentsMargins(16, 18, 16, 18);
    detail_layout->setSpacing(14);

    auto* detail_title = new QLabel(QStringLiteral("详情"), detail_panel);
    detail_title->setObjectName(QStringLiteral("detailTitleLabel"));
    detail_layout->addWidget(detail_title);

    detail_stack_ = new QStackedWidget(detail_panel);

    {
        auto* message_detail = new QWidget(detail_stack_);
        auto* layout = new QVBoxLayout(message_detail);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);

        auto* contact_card = new QFrame(message_detail);
        contact_card->setObjectName(QStringLiteral("detailCard"));
        auto* card_layout = new QVBoxLayout(contact_card);
        card_layout->setContentsMargins(14, 14, 14, 14);
        card_layout->setSpacing(6);
        detail_contact_name_label_ = new QLabel(QStringLiteral("联系人"), contact_card);
        detail_contact_name_label_->setObjectName(QStringLiteral("sectionTitleLabel"));
        card_layout->addWidget(detail_contact_name_label_);
        detail_contact_status_label_ = new QLabel(QStringLiteral("未选择联系人"), contact_card);
        detail_contact_status_label_->setObjectName(QStringLiteral("secondaryTextLabel"));
        card_layout->addWidget(detail_contact_status_label_);
        layout->addWidget(contact_card);

        auto* shared_title = new QLabel(QStringLiteral("共享文件"), message_detail);
        shared_title->setObjectName(QStringLiteral("sectionTitleLabel"));
        layout->addWidget(shared_title);
        layout->addWidget(createFileChip(QStringLiteral("📄"), QStringLiteral("需求文档"),
                                         QStringLiteral("2.4MB 今天"), message_detail));
        layout->addWidget(createFileChip(QStringLiteral("📊"), QStringLiteral("数据统计"),
                                         QStringLiteral("856KB 昨天"), message_detail));
        layout->addWidget(createFileChip(QStringLiteral("🖼️"), QStringLiteral("UI截图"),
                                         QStringLiteral("1.1MB 周一"), message_detail));
        layout->addStretch();
        detail_stack_->addWidget(message_detail);
    }

    {
        auto* file_detail = new QWidget(detail_stack_);
        auto* layout = new QVBoxLayout(file_detail);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);

        auto* selected_title = new QLabel(QStringLiteral("选中文件"), file_detail);
        selected_title->setObjectName(QStringLiteral("sectionTitleLabel"));
        layout->addWidget(selected_title);

        auto* selection_card = new QFrame(file_detail);
        selection_card->setObjectName(QStringLiteral("fileSelectionCard"));
        auto* selection_layout = new QVBoxLayout(selection_card);
        selection_layout->setContentsMargins(14, 14, 14, 14);
        selection_layout->setSpacing(6);
        detail_file_target_label_ = new QLabel(QStringLiteral("report.pdf"), selection_card);
        detail_file_target_label_->setObjectName(QStringLiteral("sectionTitleLabel"));
        selection_layout->addWidget(detail_file_target_label_);
        selection_layout->addWidget(createMetaLabel(QStringLiteral("2.4MB · PDF")));
        selection_layout->addWidget(createMetaLabel(QStringLiteral("所有者：未选择联系人")));
        layout->addWidget(selection_card);

        auto* btn1 = new QPushButton(QStringLiteral("⬇ 下载"), file_detail);
        btn1->setObjectName(QStringLiteral("lightButton"));
        auto* btn2 = new QPushButton(QStringLiteral("✂ 移动"), file_detail);
        btn2->setObjectName(QStringLiteral("lightButton"));
        auto* btn3 = new QPushButton(QStringLiteral("✏ 重命名"), file_detail);
        btn3->setObjectName(QStringLiteral("lightButton"));
        auto* btn4 = new QPushButton(QStringLiteral("🗑 删除"), file_detail);
        btn4->setObjectName(QStringLiteral("dangerButton"));
        auto* btn5 = new QPushButton(QStringLiteral("↗ 分享"), file_detail);
        btn5->setObjectName(QStringLiteral("primaryButton"));
        layout->addWidget(btn1);
        layout->addWidget(btn2);
        layout->addWidget(btn3);
        layout->addWidget(btn4);
        layout->addWidget(btn5);

        auto* progress_title = new QLabel(QStringLiteral("上传进度"), file_detail);
        progress_title->setObjectName(QStringLiteral("sectionTitleLabel"));
        layout->addWidget(progress_title);

        auto* progress_bg = new QFrame(file_detail);
        progress_bg->setObjectName(QStringLiteral("progressBarBg"));
        progress_bg->setFixedHeight(12);
        auto* progress_layout = new QHBoxLayout(progress_bg);
        progress_layout->setContentsMargins(0, 0, 0, 0);
        auto* progress_fill = new QFrame(progress_bg);
        progress_fill->setObjectName(QStringLiteral("progressBarFill"));
        progress_fill->setFixedWidth(140);
        progress_layout->addWidget(progress_fill);
        progress_layout->addStretch();
        layout->addWidget(progress_bg);
        layout->addWidget(createMetaLabel(QStringLiteral("████████░░ 80%  3.2/4MB")));
        layout->addStretch();
        detail_stack_->addWidget(file_detail);
    }

    {
        auto* profile_detail = new QWidget(detail_stack_);
        auto* layout = new QVBoxLayout(profile_detail);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);

        auto* info_card = new QFrame(profile_detail);
        info_card->setObjectName(QStringLiteral("detailCard"));
        auto* card_layout = new QVBoxLayout(info_card);
        card_layout->setContentsMargins(14, 14, 14, 14);
        card_layout->setSpacing(6);
        detail_profile_name_label_ = new QLabel(current_username_, info_card);
        detail_profile_name_label_->setObjectName(QStringLiteral("sectionTitleLabel"));
        card_layout->addWidget(detail_profile_name_label_);
        detail_profile_status_label_ = new QLabel(QStringLiteral("主连接在线"), info_card);
        detail_profile_status_label_->setObjectName(QStringLiteral("secondaryTextLabel"));
        card_layout->addWidget(detail_profile_status_label_);
        card_layout->addWidget(createMetaLabel(QStringLiteral("账号：@%1").arg(current_username_)));
        layout->addWidget(info_card);

        layout->addWidget(createFileChip(QStringLiteral("📌"), QStringLiteral("当前状态"),
                                         QStringLiteral("可编辑昵称与个性签名"), profile_detail));
        layout->addWidget(createFileChip(QStringLiteral("🔐"), QStringLiteral("安全"),
                                         QStringLiteral("退出登录入口已展示"), profile_detail));
        layout->addStretch();
        detail_stack_->addWidget(profile_detail);
    }

    detail_layout->addWidget(detail_stack_);
    root->addWidget(detail_panel);

    setCentralWidget(shell);
}

void MainWindow::connectSignals() {
    message_tab_btn_->setChecked(true);

    connect(message_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(0); });
    connect(file_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(1); });
    connect(profile_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(2); });

    connect(contact_search_edit_, &QLineEdit::textChanged,
            this, &MainWindow::filterFriendList);
    connect(contact_list_, &QListWidget::itemSelectionChanged,
            this, &MainWindow::applySelectedFriend);

    connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
            this, &MainWindow::refreshFriendList);
    connect(&friend_service_, &cloudvault::FriendService::friendAdded,
            this, [this](const QString&) { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendDeleted,
            this, [this](const QString&) { friend_service_.flushFriends(); });
}

void MainWindow::refreshFriendList(const QList<QPair<QString, bool>>& friends) {
    friends_ = friends;
    filterFriendList(contact_search_edit_->text());
}

void MainWindow::filterFriendList(const QString& keyword) {
    const QString current = selectedFriend();
    contact_list_->clear();

    for (const auto& [username, online] : friends_) {
        if (!keyword.trimmed().isEmpty() &&
            !username.contains(keyword.trimmed(), Qt::CaseInsensitive)) {
            continue;
        }

        auto* item = new QListWidgetItem(contact_list_);
        item->setText(QStringLiteral("%1  %2\n    最近消息占位")
                          .arg(online ? QStringLiteral("🔵") : QStringLiteral("⚪"),
                               username));
        item->setData(Qt::UserRole, username);
        item->setData(Qt::UserRole + 1, online);
        item->setData(Qt::UserRole + 2, online ? QStringLiteral("14:32") : QStringLiteral("周一"));
        item->setData(Qt::UserRole + 3, online ? QStringLiteral("3") : QString());
        if (username == current) {
            item->setSelected(true);
        }
    }

    if (!current.isEmpty() && !selectedFriend().isEmpty()) {
        applySelectedFriend();
        return;
    }

    if (contact_list_->count() > 0) {
        contact_list_->setCurrentRow(0);
    } else {
        chat_title_label_->setText(QStringLiteral("选择一个联系人"));
        chat_status_label_->setText(QStringLiteral("在线"));
        file_path_label_->setText(QStringLiteral("📁 /未选择联系人/"));
        detail_contact_name_label_->setText(QStringLiteral("联系人"));
        detail_contact_status_label_->setText(QStringLiteral("未选择联系人"));
        detail_file_target_label_->setText(QStringLiteral("report.pdf"));
    }
}

void MainWindow::applySelectedFriend() {
    const QString username = selectedFriend();
    const bool online = selectedFriendOnline();

    if (username.isEmpty()) {
        return;
    }

    chat_title_label_->setText(QStringLiteral("● %1").arg(username));
    chat_status_label_->setText(online ? QStringLiteral("在线") : QStringLiteral("离线"));
    file_path_label_->setText(QStringLiteral("📁 /%1/").arg(username));
    detail_contact_name_label_->setText(username);
    detail_contact_status_label_->setText(
        online ? QStringLiteral("在线 · 可直接聊天与传输文件")
               : QStringLiteral("离线 · 等待对方再次上线"));
    detail_file_target_label_->setText(QStringLiteral("%1 的 report.pdf").arg(username));
}

void MainWindow::switchMainTab(int index) {
    center_stack_->setCurrentIndex(index);
    detail_stack_->setCurrentIndex(index);

    message_tab_btn_->setChecked(index == 0);
    file_tab_btn_->setChecked(index == 1);
    profile_tab_btn_->setChecked(index == 2);
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
