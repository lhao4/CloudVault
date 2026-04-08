// =============================================================
// client/src/ui/share_file_dialog.cpp
// 文件分享弹窗
// =============================================================

#include "share_file_dialog.h"
#include "ui/widget_helpers.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

using namespace cv_ui;

namespace {

QWidget* createFriendRow(const cloudvault::FriendProfile& profile, QWidget* parent = nullptr) {
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* checkbox = new QCheckBox(row);
    checkbox->setProperty("username", profile.username);
    checkbox->setEnabled(profile.online);
    layout->addWidget(checkbox);

    auto* avatar = new QLabel(row);
    prepareAvatarBadge(avatar, profile.username, 28);
    layout->addWidget(avatar);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(1);

    const QString display_name = profile.nickname.trimmed().isEmpty()
        ? profile.username
        : profile.nickname.trimmed();
    auto* name_label = new QLabel(display_name, row);
    name_label->setObjectName(QStringLiteral("dialogItemTitle"));
    text_layout->addWidget(name_label);

    auto* meta_label = new QLabel(profile.online
                                      ? QStringLiteral("在线，可立即接收")
                                      : QStringLiteral("离线，暂不可接收"),
                                  row);
    meta_label->setObjectName(QStringLiteral("dialogItemMeta"));
    text_layout->addWidget(meta_label);
    layout->addLayout(text_layout, 1);

    return row;
}

QCheckBox* rowCheckBox(QWidget* row) {
    return row ? row->findChild<QCheckBox*>() : nullptr;
}

} // namespace

ShareFileDialog::ShareFileDialog(const QString& file_name,
                                 const QList<cloudvault::FriendProfile>& friends,
                                 QWidget* parent)
    : QDialog(parent), file_name_(file_name), friends_(friends) {
    setupUi();
    connectSignals();
    populateList();
    updateConfirmButton();
}

void ShareFileDialog::setupUi() {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(380, 500);

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

    auto* title = new QLabel(
        QStringLiteral("分享「%1」给好友").arg(QFileInfo(file_name_).fileName()),
        header);
    title->setObjectName(QStringLiteral("dialogTitle"));
    header_layout->addWidget(title);
    header_layout->addStretch();

    auto* close_btn = new QPushButton(QStringLiteral("×"), header);
    close_btn->setObjectName(QStringLiteral("iconBtn"));
    close_btn->setFixedSize(30, 30);
    connect(close_btn, &QPushButton::clicked, this, &ShareFileDialog::reject);
    header_layout->addWidget(close_btn);
    surface_layout->addWidget(header);

    auto* content = new QWidget(surface);
    auto* content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(16, 16, 16, 16);
    content_layout->setSpacing(12);

    file_hint_label_ = new QLabel(
        QStringLiteral("选择接收者后，文件会以分享通知的形式发送。"), content);
    file_hint_label_->setObjectName(QStringLiteral("dialogMetaCard"));
    content_layout->addWidget(file_hint_label_);

    search_edit_ = new QLineEdit(content);
    search_edit_->setObjectName(QStringLiteral("dialogSearchEdit"));
    search_edit_->setPlaceholderText(QStringLiteral("搜索好友…"));
    content_layout->addWidget(search_edit_);

    selection_hint_label_ = new QLabel(content);
    selection_hint_label_->setObjectName(QStringLiteral("dialogMeta"));
    content_layout->addWidget(selection_hint_label_);

    friend_list_ = new QListWidget(content);
    friend_list_->setObjectName(QStringLiteral("dialogList"));
    friend_list_->setSpacing(0);
    friend_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    content_layout->addWidget(friend_list_, 1);

    empty_state_label_ = new QLabel(QStringLiteral("当前没有可分享的好友"), content);
    empty_state_label_->setObjectName(QStringLiteral("dialogEmptyLabel"));
    empty_state_label_->setAlignment(Qt::AlignCenter);
    empty_state_label_->hide();
    content_layout->addWidget(empty_state_label_);
    surface_layout->addWidget(content, 1);

    auto* footer = new QFrame(surface);
    footer->setObjectName(QStringLiteral("dialogFooter"));
    footer->setFixedHeight(56);
    auto* footer_layout = new QHBoxLayout(footer);
    footer_layout->setContentsMargins(16, 0, 16, 0);
    footer_layout->setSpacing(8);

    select_all_btn_ = new QPushButton(QStringLiteral("全选"), footer);
    select_all_btn_->setObjectName(QStringLiteral("dialogSecondaryBtn"));
    footer_layout->addWidget(select_all_btn_);

    clear_btn_ = new QPushButton(QStringLiteral("取消全选"), footer);
    clear_btn_->setObjectName(QStringLiteral("dialogSecondaryBtn"));
    footer_layout->addWidget(clear_btn_);

    footer_layout->addStretch();

    confirm_btn_ = new QPushButton(QStringLiteral("选择接收好友"), footer);
    confirm_btn_->setObjectName(QStringLiteral("dialogPrimaryBtn"));
    footer_layout->addWidget(confirm_btn_);
    surface_layout->addWidget(footer);

    root->addWidget(surface);
}

void ShareFileDialog::connectSignals() {
    connect(search_edit_, &QLineEdit::textChanged,
            this, [this](const QString& text) { populateList(text); });
    connect(select_all_btn_, &QPushButton::clicked, this, [this] {
        for (int i = 0; i < friend_list_->count(); ++i) {
            if (auto* item = friend_list_->item(i)) {
                if (auto* checkbox = rowCheckBox(friend_list_->itemWidget(item))) {
                    if (checkbox->isEnabled()) {
                        checkbox->setChecked(true);
                    }
                }
            }
        }
        updateConfirmButton();
    });
    connect(clear_btn_, &QPushButton::clicked, this, [this] {
        for (int i = 0; i < friend_list_->count(); ++i) {
            if (auto* item = friend_list_->item(i)) {
                if (auto* checkbox = rowCheckBox(friend_list_->itemWidget(item))) {
                    checkbox->setChecked(false);
                }
            }
        }
        updateConfirmButton();
    });
    connect(confirm_btn_, &QPushButton::clicked, this, [this] {
        const QStringList targets = selectedTargets();
        if (targets.isEmpty()) {
            return;
        }
        emit shareConfirmed(file_name_, targets);
        accept();
    });
}

void ShareFileDialog::populateList(const QString& keyword) {
    friend_list_->clear();
    const QString trimmed = keyword.trimmed();
    int visible_count = 0;
    int online_count = 0;

    for (const auto& profile : friends_) {
        const QString display_name = profile.nickname.trimmed().isEmpty()
            ? profile.username
            : profile.nickname.trimmed();
        if (!trimmed.isEmpty()
            && !profile.username.contains(trimmed, Qt::CaseInsensitive)
            && !display_name.contains(trimmed, Qt::CaseInsensitive)) {
            continue;
        }

        auto* item = new QListWidgetItem(friend_list_);
        item->setSizeHint(QSize(0, 48));
        auto* row = createFriendRow(profile, friend_list_);
        if (auto* checkbox = rowCheckBox(row)) {
            connect(checkbox, &QCheckBox::toggled, this, &ShareFileDialog::updateConfirmButton);
        }
        friend_list_->setItemWidget(item, row);
        ++visible_count;
        if (profile.online) {
            ++online_count;
        }
    }

    friend_list_->setVisible(visible_count > 0);
    empty_state_label_->setVisible(visible_count == 0);
    empty_state_label_->setText(trimmed.isEmpty()
        ? QStringLiteral("当前没有可展示的好友")
        : QStringLiteral("没有匹配的好友"));
    selection_hint_label_->setText(visible_count == 0
        ? QStringLiteral("0 位好友可见")
        : QStringLiteral("当前显示 %1 位好友，其中 %2 位在线可选")
              .arg(visible_count)
              .arg(online_count));
    updateConfirmButton();
}

void ShareFileDialog::updateConfirmButton() {
    const QStringList targets = selectedTargets();
    if (targets.isEmpty()) {
        confirm_btn_->setText(QStringLiteral("确认分享"));
    } else {
        confirm_btn_->setText(QStringLiteral("确认分享 (%1)").arg(targets.size()));
    }
    confirm_btn_->setEnabled(!targets.isEmpty());
    clear_btn_->setEnabled(friend_list_->count() > 0);
    select_all_btn_->setEnabled(friend_list_->count() > 0);

    if (targets.isEmpty()) {
        return;
    }
    selection_hint_label_->setText(QStringLiteral("已选择：%1").arg(targets.join(QStringLiteral("、"))));
}

QStringList ShareFileDialog::selectedTargets() const {
    QStringList result;
    for (int i = 0; i < friend_list_->count(); ++i) {
        if (auto* item = friend_list_->item(i)) {
            if (auto* checkbox = rowCheckBox(friend_list_->itemWidget(item))) {
                if (checkbox->isChecked()) {
                    result.append(checkbox->property("username").toString());
                }
            }
        }
    }
    return result;
}
