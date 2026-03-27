// =============================================================
// client/src/ui/online_user_dialog.cpp
// 在线用户弹窗
// =============================================================

#include "online_user_dialog.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

void applyShadow(QWidget* widget) {
    auto* effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(40);
    effect->setOffset(0, 8);
    effect->setColor(QColor(17, 24, 39, 30));
    widget->setGraphicsEffect(effect);
}

int avatarVariantForSeed(const QString& seed) {
    return static_cast<int>(qHash(seed)) % 6;
}

void prepareAvatarBadge(QLabel* label, const QString& seed, int size) {
    label->setObjectName(QStringLiteral("avatarBadge"));
    label->setProperty("variant", avatarVariantForSeed(seed));
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(size, size);
    label->setText(seed.left(1).toUpper());
    label->setStyleSheet(QStringLiteral("border-radius:%1px;").arg(size / 2));
    label->style()->unpolish(label);
    label->style()->polish(label);
}

QWidget* createUserRow(const QString& username, QWidget* parent = nullptr) {
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* avatar = new QLabel(row);
    prepareAvatarBadge(avatar, username, 32);
    layout->addWidget(avatar);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(1);

    auto* name_label = new QLabel(username, row);
    name_label->setObjectName(QStringLiteral("dialogItemTitle"));
    text_layout->addWidget(name_label);

    auto* meta_label = new QLabel(QStringLiteral("在线"), row);
    meta_label->setObjectName(QStringLiteral("dialogItemMeta"));
    text_layout->addWidget(meta_label);
    layout->addLayout(text_layout, 1);

    auto* dot = new QLabel(QStringLiteral("●"), row);
    dot->setObjectName(QStringLiteral("dialogOnlineDot"));
    layout->addWidget(dot, 0, Qt::AlignRight | Qt::AlignVCenter);

    return row;
}

} // namespace

OnlineUserDialog::OnlineUserDialog(const QList<QPair<QString, bool>>& users,
                                   QWidget* parent)
    : QDialog(parent), users_(users) {
    setupUi();
    connectSignals();
    populateList();
}

void OnlineUserDialog::setUsers(const QList<QPair<QString, bool>>& users) {
    users_ = users;
    populateList(search_edit_ ? search_edit_->text() : QString());
}

void OnlineUserDialog::setupUi() {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(380, 460);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(0);

    auto* surface = new QFrame(this);
    surface->setObjectName(QStringLiteral("dialogSurface"));
    applyShadow(surface);
    auto* surface_layout = new QVBoxLayout(surface);
    surface_layout->setContentsMargins(0, 0, 0, 0);
    surface_layout->setSpacing(0);

    auto* header = new QFrame(surface);
    header->setObjectName(QStringLiteral("dialogHeader"));
    header->setFixedHeight(52);
    auto* header_layout = new QHBoxLayout(header);
    header_layout->setContentsMargins(16, 0, 12, 0);
    header_layout->setSpacing(8);

    title_label_ = new QLabel(header);
    title_label_->setObjectName(QStringLiteral("dialogTitle"));
    header_layout->addWidget(title_label_);
    header_layout->addStretch();

    auto* close_btn = new QPushButton(QStringLiteral("×"), header);
    close_btn->setObjectName(QStringLiteral("iconBtn"));
    close_btn->setFixedSize(30, 30);
    connect(close_btn, &QPushButton::clicked, this, &OnlineUserDialog::reject);
    header_layout->addWidget(close_btn);
    surface_layout->addWidget(header);

    auto* content = new QWidget(surface);
    auto* content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(16, 16, 16, 16);
    content_layout->setSpacing(12);

    search_edit_ = new QLineEdit(content);
    search_edit_->setObjectName(QStringLiteral("dialogSearchEdit"));
    search_edit_->setPlaceholderText(QStringLiteral("搜索在线用户…"));
    content_layout->addWidget(search_edit_);

    user_list_ = new QListWidget(content);
    user_list_->setObjectName(QStringLiteral("dialogList"));
    user_list_->setSpacing(0);
    user_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    content_layout->addWidget(user_list_, 1);
    surface_layout->addWidget(content, 1);

    auto* footer = new QFrame(surface);
    footer->setObjectName(QStringLiteral("dialogFooter"));
    footer->setFixedHeight(56);
    auto* footer_layout = new QHBoxLayout(footer);
    footer_layout->setContentsMargins(16, 0, 16, 0);
    footer_layout->setSpacing(8);

    refresh_btn_ = new QPushButton(QStringLiteral("刷新"), footer);
    refresh_btn_->setObjectName(QStringLiteral("dialogSecondaryBtn"));
    footer_layout->addWidget(refresh_btn_);

    footer_layout->addStretch();

    add_btn_ = new QPushButton(QStringLiteral("+ 添加好友"), footer);
    add_btn_->setObjectName(QStringLiteral("dialogPrimaryBtn"));
    add_btn_->setEnabled(false);
    footer_layout->addWidget(add_btn_);
    surface_layout->addWidget(footer);

    root->addWidget(surface);
}

void OnlineUserDialog::connectSignals() {
    connect(search_edit_, &QLineEdit::textChanged,
            this, [this](const QString& text) {
                populateList(text);
                add_btn_->setEnabled(!text.trimmed().isEmpty() || user_list_->currentItem() != nullptr);
            });
    connect(user_list_, &QListWidget::itemSelectionChanged,
            this, [this] {
                add_btn_->setEnabled(!search_edit_->text().trimmed().isEmpty()
                                     || user_list_->currentItem() != nullptr);
            });
    connect(refresh_btn_, &QPushButton::clicked, this, &OnlineUserDialog::refreshRequested);
    connect(add_btn_, &QPushButton::clicked, this, [this] {
        QString username = search_edit_->text().trimmed();
        if (auto* item = user_list_->currentItem()) {
            username = item->data(Qt::UserRole).toString();
        }
        if (!username.isEmpty()) {
            emit userChosen(username);
            accept();
        }
    });
    connect(user_list_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                if (!item) {
                    return;
                }
                emit userChosen(item->data(Qt::UserRole).toString());
                accept();
            });
}

void OnlineUserDialog::populateList(const QString& keyword) {
    user_list_->clear();

    int online_count = 0;
    const QString trimmed = keyword.trimmed();
    for (const auto& [username, online] : users_) {
        if (!online) {
            continue;
        }
        ++online_count;
        if (!trimmed.isEmpty() && !username.contains(trimmed, Qt::CaseInsensitive)) {
            continue;
        }

        auto* item = new QListWidgetItem(user_list_);
        item->setData(Qt::UserRole, username);
        item->setSizeHint(QSize(0, 48));
        user_list_->setItemWidget(item, createUserRow(username, user_list_));
    }

    title_label_->setText(QStringLiteral("在线用户（%1）").arg(online_count));
}
