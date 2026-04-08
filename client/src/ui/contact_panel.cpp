// =============================================================
// client/src/ui/contact_panel.cpp
// 联系人与群组主区面板
// =============================================================

#include "contact_panel.h"
#include "ui/widget_helpers.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

using namespace cv_ui;

namespace {

QPushButton* createActionButton(const QString& text,
                                const QString& object_name,
                                QWidget* parent = nullptr) {
    auto* button = new QPushButton(text, parent);
    button->setObjectName(object_name);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(34);
    return button;
}

QWidget* createMetaRow(const QString& label_text,
                       QLabel*& value_label,
                       QWidget* parent = nullptr) {
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* label = new QLabel(label_text, row);
    label->setObjectName(QStringLiteral("fieldLabel"));
    label->setMinimumWidth(56);
    layout->addWidget(label);

    value_label = new QLabel(QString(), row);
    value_label->setObjectName(QStringLiteral("contactProfileSummary"));
    value_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(value_label, 1);
    return row;
}

} // namespace

ContactPanel::ContactPanel(const QString& current_username, QWidget* parent)
    : QWidget(parent),
      current_username_(current_username) {
    setObjectName(QStringLiteral("contactPage"));

    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);

    stack_ = new QStackedWidget(this);
    root_layout->addWidget(stack_);

    auto* empty_page = new QWidget(stack_);
    auto* empty_layout = new QVBoxLayout(empty_page);
    empty_layout->setContentsMargins(32, 32, 32, 32);
    empty_layout->setSpacing(10);
    empty_layout->addStretch();

    auto* empty_icon = new QLabel(QStringLiteral("👥"), empty_page);
    empty_icon->setObjectName(QStringLiteral("chatEmptyIcon"));
    empty_icon->setAlignment(Qt::AlignCenter);
    empty_layout->addWidget(empty_icon, 0, Qt::AlignHCenter);

    auto* empty_title = new QLabel(QStringLiteral("选择好友或群组查看资料"), empty_page);
    empty_title->setObjectName(QStringLiteral("chatEmptyTitle"));
    empty_title->setAlignment(Qt::AlignCenter);
    empty_layout->addWidget(empty_title);

    auto* empty_hint = new QLabel(QStringLiteral("从左侧联系人视图中选择一个对象开始操作。"), empty_page);
    empty_hint->setObjectName(QStringLiteral("chatEmptyHint"));
    empty_hint->setAlignment(Qt::AlignCenter);
    empty_layout->addWidget(empty_hint);
    empty_layout->addStretch();
    stack_->addWidget(empty_page);

    auto* friend_page = new QWidget(stack_);
    auto* friend_layout = new QVBoxLayout(friend_page);
    friend_layout->setContentsMargins(24, 24, 24, 24);
    friend_layout->setSpacing(16);

    auto* friend_card = new QFrame(friend_page);
    friend_card->setObjectName(QStringLiteral("contactProfileCard"));
    auto* friend_card_layout = new QVBoxLayout(friend_card);
    friend_card_layout->setContentsMargins(28, 28, 28, 28);
    friend_card_layout->setSpacing(14);

    avatar_label_ = new QLabel(friend_card);
    prepareAvatarBadge(avatar_label_, current_username_, 56);
    friend_card_layout->addWidget(avatar_label_, 0, Qt::AlignLeft);

    title_label_ = new QLabel(QStringLiteral("好友"), friend_card);
    title_label_->setObjectName(QStringLiteral("contactProfileTitle"));
    friend_card_layout->addWidget(title_label_);

    subtitle_label_ = new QLabel(QStringLiteral("离线"), friend_card);
    subtitle_label_->setObjectName(QStringLiteral("contactProfileSubtitle"));
    friend_card_layout->addWidget(subtitle_label_);

    summary_label_ = new QLabel(QStringLiteral("选择好友后可在这里查看资料和常用操作。"), friend_card);
    summary_label_->setObjectName(QStringLiteral("contactProfileSummary"));
    summary_label_->setWordWrap(true);
    friend_card_layout->addWidget(summary_label_);

    friend_card_layout->addWidget(createMetaRow(QStringLiteral("账号"),
                                                friend_username_value_label_,
                                                friend_card));
    friend_card_layout->addWidget(createMetaRow(QStringLiteral("昵称"),
                                                friend_nickname_value_label_,
                                                friend_card));
    friend_card_layout->addWidget(createMetaRow(QStringLiteral("签名"),
                                                friend_signature_value_label_,
                                                friend_card));

    auto* friend_actions = new QHBoxLayout();
    friend_actions->setSpacing(10);
    send_message_btn_ = createActionButton(QStringLiteral("发送消息"),
                                           QStringLiteral("primaryActionBtn"),
                                           friend_card);
    share_file_btn_ = createActionButton(QStringLiteral("分享文件"),
                                         QStringLiteral("secondaryActionBtn"),
                                         friend_card);
    delete_friend_btn_ = createActionButton(QStringLiteral("删除好友"),
                                            QStringLiteral("dangerActionBtn"),
                                            friend_card);
    accept_friend_request_btn_ = createActionButton(QStringLiteral("通过申请"),
                                                    QStringLiteral("primaryActionBtn"),
                                                    friend_card);
    friend_actions->addWidget(send_message_btn_);
    friend_actions->addWidget(share_file_btn_);
    friend_actions->addWidget(accept_friend_request_btn_);
    friend_actions->addStretch();
    friend_actions->addWidget(delete_friend_btn_);
    friend_card_layout->addLayout(friend_actions);
    friend_layout->addWidget(friend_card);
    friend_layout->addStretch();
    stack_->addWidget(friend_page);

    auto* group_page = new QWidget(stack_);
    auto* group_layout = new QVBoxLayout(group_page);
    group_layout->setContentsMargins(24, 24, 24, 24);
    group_layout->setSpacing(16);

    auto* group_card = new QFrame(group_page);
    group_card->setObjectName(QStringLiteral("contactProfileCard"));
    auto* group_card_layout = new QVBoxLayout(group_card);
    group_card_layout->setContentsMargins(28, 28, 28, 28);
    group_card_layout->setSpacing(14);

    group_avatar_label_ = new QLabel(group_card);
    prepareAvatarBadge(group_avatar_label_, QStringLiteral("群"), 56);
    group_card_layout->addWidget(group_avatar_label_, 0, Qt::AlignLeft);

    group_title_label_ = new QLabel(QStringLiteral("群聊"), group_card);
    group_title_label_->setObjectName(QStringLiteral("contactProfileTitle"));
    group_card_layout->addWidget(group_title_label_);

    group_subtitle_label_ = new QLabel(QStringLiteral("0 人在线"), group_card);
    group_subtitle_label_->setObjectName(QStringLiteral("contactProfileSubtitle"));
    group_card_layout->addWidget(group_subtitle_label_);

    group_summary_label_ = new QLabel(QStringLiteral("可从这里进入群聊或执行基础群组操作。"), group_card);
    group_summary_label_->setObjectName(QStringLiteral("contactProfileSummary"));
    group_summary_label_->setWordWrap(true);
    group_card_layout->addWidget(group_summary_label_);

    group_card_layout->addWidget(createMetaRow(QStringLiteral("群 ID"),
                                               group_id_value_label_,
                                               group_card));
    group_card_layout->addWidget(createMetaRow(QStringLiteral("群主"),
                                               group_owner_value_label_,
                                               group_card));
    group_card_layout->addWidget(createMetaRow(QStringLiteral("在线"),
                                               group_online_value_label_,
                                               group_card));

    auto* group_actions = new QHBoxLayout();
    group_actions->setSpacing(10);
    open_group_chat_btn_ = createActionButton(QStringLiteral("进入聊天"),
                                              QStringLiteral("primaryActionBtn"),
                                              group_card);
    view_group_members_btn_ = createActionButton(QStringLiteral("查看成员"),
                                                 QStringLiteral("secondaryActionBtn"),
                                                 group_card);
    leave_group_btn_ = createActionButton(QStringLiteral("退出群组"),
                                          QStringLiteral("dangerActionBtn"),
                                          group_card);
    group_actions->addWidget(open_group_chat_btn_);
    group_actions->addWidget(view_group_members_btn_);
    group_actions->addStretch();
    group_actions->addWidget(leave_group_btn_);
    group_card_layout->addLayout(group_actions);
    group_layout->addWidget(group_card);
    group_layout->addStretch();
    stack_->addWidget(group_page);

    connect(send_message_btn_, &QPushButton::clicked, this, [this] {
        if (!current_friend_username_.isEmpty()) {
            emit sendMessageRequested(current_friend_username_);
        }
    });
    connect(share_file_btn_, &QPushButton::clicked, this, [this] {
        if (!current_friend_username_.isEmpty()) {
            emit shareFileRequested(current_friend_username_);
        }
    });
    connect(delete_friend_btn_, &QPushButton::clicked, this, [this] {
        if (!current_friend_username_.isEmpty()) {
            emit deleteFriendRequested(current_friend_username_);
        }
    });
    connect(accept_friend_request_btn_, &QPushButton::clicked, this, [this] {
        if (!current_friend_username_.isEmpty()) {
            emit acceptFriendRequestRequested(current_friend_username_);
        }
    });
    connect(open_group_chat_btn_, &QPushButton::clicked, this, [this] {
        if (current_group_id_ > 0) {
            emit openGroupChatRequested(current_group_id_);
        }
    });
    connect(view_group_members_btn_, &QPushButton::clicked, this, [this] {
        if (current_group_id_ > 0) {
            emit viewGroupMembersRequested(current_group_id_);
        }
    });
    connect(leave_group_btn_, &QPushButton::clicked, this, [this] {
        if (current_group_id_ > 0) {
            emit leaveGroupRequested(current_group_id_);
        }
    });
}

void ContactPanel::showEmptyState() {
    current_friend_username_.clear();
    current_group_id_ = 0;
    state_ = State::Empty;
    if (stack_) {
        stack_->setCurrentIndex(0);
    }
}

void ContactPanel::showFriendProfile(const QString& username,
                                     const QString& display_name,
                                     const QString& signature,
                                     bool online) {
    current_group_id_ = 0;
    current_friend_username_ = username;
    state_ = State::Friend;
    if (stack_) {
        stack_->setCurrentIndex(1);
    }
    updateAvatar(username, 56);
    if (title_label_) {
        title_label_->setText(display_name.isEmpty() ? username : display_name);
    }
    if (subtitle_label_) {
        subtitle_label_->setText(online ? QStringLiteral("在线") : QStringLiteral("离线"));
    }
    if (summary_label_) {
        summary_label_->setText(QString());
    }
    if (friend_username_value_label_) {
        friend_username_value_label_->setText(username);
    }
    if (friend_nickname_value_label_) {
        friend_nickname_value_label_->setText(display_name.isEmpty() ? QString() : display_name);
    }
    if (friend_signature_value_label_) {
        friend_signature_value_label_->setText(signature);
    }
    send_message_btn_->setVisible(true);
    share_file_btn_->setVisible(true);
    delete_friend_btn_->setVisible(true);
    accept_friend_request_btn_->setVisible(false);
}

void ContactPanel::showFriendRequest(const QString& username, bool incoming) {
    current_group_id_ = 0;
    current_friend_username_ = username;
    state_ = incoming ? State::FriendRequestIncoming : State::FriendRequestOutgoing;
    if (stack_) {
        stack_->setCurrentIndex(1);
    }
    updateAvatar(username, 56);
    if (title_label_) {
        title_label_->setText(username);
    }
    if (subtitle_label_) {
        subtitle_label_->setText(incoming ? QStringLiteral("收到的好友申请")
                                          : QStringLiteral("已发送的好友申请"));
    }
    if (summary_label_) {
        summary_label_->setText(QString());
    }
    if (friend_username_value_label_) {
        friend_username_value_label_->setText(username);
    }
    if (friend_nickname_value_label_) {
        friend_nickname_value_label_->setText(QString());
    }
    if (friend_signature_value_label_) {
        friend_signature_value_label_->setText(QString());
    }
    send_message_btn_->setVisible(false);
    share_file_btn_->setVisible(false);
    delete_friend_btn_->setVisible(false);
    accept_friend_request_btn_->setVisible(incoming);
}

void ContactPanel::showGroupProfile(int group_id,
                                    const QString& group_name,
                                    int owner_id,
                                    int online_count) {
    current_friend_username_.clear();
    current_group_id_ = group_id;
    state_ = State::Group;
    if (stack_) {
        stack_->setCurrentIndex(2);
    }
    prepareAvatarBadge(group_avatar_label_, QStringLiteral("群"), 56);
    if (group_title_label_) {
        group_title_label_->setText(group_name);
    }
    if (group_subtitle_label_) {
        group_subtitle_label_->setText(QStringLiteral("%1 人在线").arg(online_count));
    }
    if (group_summary_label_) {
        group_summary_label_->setText(QString());
    }
    if (group_id_value_label_) {
        group_id_value_label_->setText(QString::number(group_id));
    }
    if (group_owner_value_label_) {
        group_owner_value_label_->setText(QString::number(owner_id));
    }
    if (group_online_value_label_) {
        group_online_value_label_->setText(QString::number(online_count));
    }
}

void ContactPanel::updateAvatar(const QString& seed, int size) {
    prepareAvatarBadge(avatar_label_, seed, size);
}

ContactPanel::State ContactPanel::state() const {
    return state_;
}
