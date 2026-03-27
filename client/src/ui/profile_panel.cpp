// =============================================================
// client/src/ui/profile_panel.cpp
// 个人信息面板
// =============================================================

#include "profile_panel.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace {

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

void applyShadow(QWidget* widget, int blur = 32, int offset_y = 6, int alpha = 20) {
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

} // namespace

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

    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("profileCard"));
    card->setFixedWidth(360);
    applyShadow(card);

    auto* card_layout = new QVBoxLayout(card);
    card_layout->setContentsMargins(32, 32, 32, 32);
    card_layout->setSpacing(12);

    auto* profile_avatar = new QLabel(card);
    prepareAvatarBadge(profile_avatar, current_username, 64);
    card_layout->addWidget(profile_avatar, 0, Qt::AlignHCenter);

    profile_name_label_ = new QLabel(current_username, card);
    profile_name_label_->setObjectName(QStringLiteral("profileName"));
    profile_name_label_->setAlignment(Qt::AlignCenter);
    card_layout->addWidget(profile_name_label_);

    profile_id_label_ = new QLabel(QStringLiteral("@%1").arg(current_username), card);
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

    auto* logout_btn = new QPushButton(QStringLiteral("退出登录"), card);
    logout_btn->setObjectName(QStringLiteral("logoutBtn"));
    card_layout->addWidget(logout_btn);

    connect(save_btn, &QPushButton::clicked, this, &ProfilePanel::saveRequested);
    connect(cancel_btn, &QPushButton::clicked, this, &ProfilePanel::cancelRequested);
    connect(logout_btn, &QPushButton::clicked, this, &ProfilePanel::logoutRequested);

    card_row->addWidget(card);
    card_row->addStretch();
    page_layout->addLayout(card_row);
    page_layout->addStretch();
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
