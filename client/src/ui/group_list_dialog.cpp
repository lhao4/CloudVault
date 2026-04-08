// =============================================================
// client/src/ui/group_list_dialog.cpp
// 群组列表弹窗
// =============================================================

#include "group_list_dialog.h"
#include "ui/widget_helpers.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace cv_ui;

namespace {

QWidget* createGroupRow(const GroupListEntry& group, QWidget* parent = nullptr) {
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* avatar = new QLabel(group.name.left(1).toUpper(), row);
    avatar->setObjectName(QStringLiteral("avatarBadge"));
    avatar->setProperty("variant", static_cast<int>(qHash(group.name) % 6u));
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setFixedSize(28, 28);
    avatar->setStyleSheet(QStringLiteral("border-radius:14px;"));
    layout->addWidget(avatar);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(1);

    auto* title = new QLabel(group.name, row);
    title->setObjectName(QStringLiteral("dialogItemTitle"));
    text_layout->addWidget(title);

    auto* meta = new QLabel(QStringLiteral("%1 人在线").arg(group.online_count), row);
    meta->setObjectName(QStringLiteral("dialogItemMeta"));
    text_layout->addWidget(meta);
    layout->addLayout(text_layout, 1);

    if (group.unread_count > 0) {
        auto* badge = new QLabel(QString::number(group.unread_count), row);
        badge->setObjectName(QStringLiteral("contactBadgeLabel"));
        badge->setAlignment(Qt::AlignCenter);
        layout->addWidget(badge, 0, Qt::AlignRight | Qt::AlignVCenter);
    }

    return row;
}

} // namespace

GroupListDialog::GroupListDialog(const QList<GroupListEntry>& groups, QWidget* parent)
    : QDialog(parent), groups_(groups) {
    setupUi();
    connectSignals();
    populateList();
}

void GroupListDialog::setupUi() {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(380, 420);

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

    auto* title = new QLabel(QStringLiteral("我的群组"), header);
    title->setObjectName(QStringLiteral("dialogTitle"));
    header_layout->addWidget(title);
    header_layout->addStretch();

    auto* close_btn = new QPushButton(QStringLiteral("×"), header);
    close_btn->setObjectName(QStringLiteral("iconBtn"));
    close_btn->setFixedSize(30, 30);
    connect(close_btn, &QPushButton::clicked, this, &GroupListDialog::reject);
    header_layout->addWidget(close_btn);
    surface_layout->addWidget(header);

    auto* content = new QWidget(surface);
    auto* content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(16, 16, 16, 16);
    content_layout->setSpacing(12);

    group_list_ = new QListWidget(content);
    group_list_->setObjectName(QStringLiteral("dialogList"));
    group_list_->setSpacing(0);
    group_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    content_layout->addWidget(group_list_, 1);

    empty_state_label_ = new QLabel(QStringLiteral("当前暂无群组"), content);
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

    enter_btn_ = new QPushButton(QStringLiteral("进入群聊"), footer);
    enter_btn_->setObjectName(QStringLiteral("dialogSecondaryBtn"));
    enter_btn_->setEnabled(false);
    footer_layout->addWidget(enter_btn_);

    leave_btn_ = new QPushButton(QStringLiteral("退出群组"), footer);
    leave_btn_->setObjectName(QStringLiteral("dialogSecondaryBtn"));
    leave_btn_->setEnabled(false);
    footer_layout->addWidget(leave_btn_);

    footer_layout->addStretch();

    create_btn_ = new QPushButton(QStringLiteral("+ 创建群组"), footer);
    create_btn_->setObjectName(QStringLiteral("dialogPrimaryBtn"));
    create_btn_->setEnabled(false);
    footer_layout->addWidget(create_btn_);
    surface_layout->addWidget(footer);

    root->addWidget(surface);
}

void GroupListDialog::connectSignals() {
    connect(group_list_, &QListWidget::itemSelectionChanged, this, [this] {
        const bool has_selection = group_list_->currentItem() != nullptr;
        enter_btn_->setEnabled(has_selection);
        leave_btn_->setEnabled(has_selection);
    });

    connect(group_list_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                if (!item) {
                    return;
                }
                emit groupChosen(item->data(Qt::UserRole).toInt());
                accept();
            });

    connect(enter_btn_, &QPushButton::clicked, this, [this] {
        if (auto* item = group_list_->currentItem()) {
            emit groupChosen(item->data(Qt::UserRole).toInt());
            accept();
        }
    });

    connect(create_btn_, &QPushButton::clicked, this, [this] {
        const QString name = QInputDialog::getText(this,
                                                   QStringLiteral("创建群组"),
                                                   QStringLiteral("请输入群名称：")).trimmed();
        if (!name.isEmpty()) {
            emit createRequested(name);
        }
    });

    connect(leave_btn_, &QPushButton::clicked, this, [this] {
        auto* item = group_list_->currentItem();
        if (!item) {
            return;
        }
        if (QMessageBox::question(this,
                                  QStringLiteral("退出群组"),
                                  QStringLiteral("确认退出当前群组吗？"),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            emit leaveRequested(item->data(Qt::UserRole).toInt());
        }
    });
}

void GroupListDialog::populateList() {
    group_list_->clear();

    for (const auto& group : groups_) {
        auto* item = new QListWidgetItem(group_list_);
        item->setData(Qt::UserRole, group.group_id);
        item->setSizeHint(QSize(0, 48));
        group_list_->setItemWidget(item, createGroupRow(group, group_list_));
    }

    const bool has_groups = !groups_.isEmpty();
    group_list_->setVisible(has_groups);
    empty_state_label_->setVisible(!has_groups);
    create_btn_->setEnabled(true);
}

void GroupListDialog::setGroups(const QList<GroupListEntry>& groups) {
    groups_ = groups;
    populateList();
}
