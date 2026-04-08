// =============================================================
// client/src/ui/profile_panel.cpp
// 个人信息面板
// =============================================================

#include "profile_panel.h"
#include "ui/widget_helpers.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QVBoxLayout>

using namespace cv_ui;

ProfilePanel::ProfilePanel(const QString& current_username, QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("profilePage"));

    auto* page_layout = new QVBoxLayout(this);
    page_layout->setContentsMargins(0, 0, 0, 0);
    page_layout->setSpacing(0);
    page_layout->addStretch();

    auto* card_row = new QHBoxLayout();
    card_row->setContentsMargins(0, 0, 0, 0);
    card_row->setSpacing(0);
    card_row->addStretch();

    profile_card_ = new QFrame(this);
    profile_card_->setObjectName(QStringLiteral("profileCard"));
    profile_card_->setFixedWidth(400);
    applyShadow(profile_card_, 24, 6, 23);

    auto* card_layout = new QVBoxLayout(profile_card_);
    card_layout->setContentsMargins(0, 0, 0, 0);
    card_layout->setSpacing(0);

    profile_banner_ = new QFrame(profile_card_);
    profile_banner_->setObjectName(QStringLiteral("profileBanner"));
    profile_banner_->setFixedHeight(120);
    card_layout->addWidget(profile_banner_);

    profile_avatar_label_ = new QLabel(profile_card_);
    profile_avatar_label_->setObjectName(QStringLiteral("profileAvatar"));
    prepareAvatarBadge(profile_avatar_label_, current_username, 72);
    profile_avatar_label_->raise();

    auto* content = new QWidget(profile_card_);
    auto* content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(32, 48, 32, 28);
    content_layout->setSpacing(12);

    profile_name_label_ = new QLabel(current_username, content);
    profile_name_label_->setObjectName(QStringLiteral("profileName"));
    content_layout->addWidget(profile_name_label_);

    profile_id_label_ = new QLabel(QStringLiteral("@%1").arg(current_username), content);
    profile_id_label_->setObjectName(QStringLiteral("profileId"));
    content_layout->addWidget(profile_id_label_);

    auto* divider_top = new QFrame(content);
    divider_top->setFrameShape(QFrame::HLine);
    content_layout->addWidget(divider_top);

    auto* nickname_label = new QLabel(QStringLiteral("昵称"), content);
    nickname_label->setObjectName(QStringLiteral("fieldLabel"));
    content_layout->addWidget(nickname_label);

    profile_nickname_edit_ = new QLineEdit(content);
    profile_nickname_edit_->setPlaceholderText(QStringLiteral("请输入昵称"));
    content_layout->addWidget(profile_nickname_edit_);

    auto* signature_label = new QLabel(QStringLiteral("个性签名"), content);
    signature_label->setObjectName(QStringLiteral("fieldLabel"));
    content_layout->addWidget(signature_label);

    profile_signature_edit_ = new QLineEdit(content);
    profile_signature_edit_->setPlaceholderText(QStringLiteral("写点什么介绍自己"));
    content_layout->addWidget(profile_signature_edit_);

    auto* action_row = new QHBoxLayout();
    action_row->setSpacing(10);
    auto* save_btn = new QPushButton(QStringLiteral("保存"), content);
    save_btn->setObjectName(QStringLiteral("primaryBtn"));
    auto* cancel_btn = new QPushButton(QStringLiteral("取消"), content);
    cancel_btn->setObjectName(QStringLiteral("secondaryBtn"));
    action_row->addWidget(save_btn);
    action_row->addWidget(cancel_btn);
    content_layout->addLayout(action_row);

    auto* divider_bottom = new QFrame(content);
    divider_bottom->setFrameShape(QFrame::HLine);
    content_layout->addWidget(divider_bottom);

    auto* logout_btn = new QPushButton(QStringLiteral("退出登录"), content);
    logout_btn->setObjectName(QStringLiteral("dangerBtn"));
    content_layout->addWidget(logout_btn);

    card_layout->addWidget(content);

    connect(save_btn, &QPushButton::clicked, this, &ProfilePanel::saveRequested);
    connect(cancel_btn, &QPushButton::clicked, this, &ProfilePanel::cancelRequested);
    connect(logout_btn, &QPushButton::clicked, this, &ProfilePanel::logoutRequested);

    card_row->addWidget(profile_card_);
    card_row->addStretch();
    page_layout->addLayout(card_row);
    page_layout->addStretch();
    updateAvatarGeometry();
}

void ProfilePanel::setDisplayName(const QString& name) {
    if (profile_name_label_) {
        profile_name_label_->setText(name);
    }
}

void ProfilePanel::setDraftValues(const QString& nickname, const QString& signature) {
    if (profile_nickname_edit_) {
        profile_nickname_edit_->setText(nickname);
    }
    if (profile_signature_edit_) {
        profile_signature_edit_->setText(signature);
    }
}

QString ProfilePanel::nicknameText() const {
    return profile_nickname_edit_ ? profile_nickname_edit_->text() : QString();
}

QString ProfilePanel::signatureText() const {
    return profile_signature_edit_ ? profile_signature_edit_->text() : QString();
}

void ProfilePanel::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateAvatarGeometry();
}

void ProfilePanel::updateAvatarGeometry() {
    if (!profile_card_ || !profile_banner_ || !profile_avatar_label_) {
        return;
    }
    const int avatar_size = 72;
    const int avatar_left = 32;
    const int avatar_top = profile_banner_->y() + profile_banner_->height() - avatar_size / 2;
    profile_avatar_label_->setGeometry(avatar_left, avatar_top, avatar_size, avatar_size);
}
