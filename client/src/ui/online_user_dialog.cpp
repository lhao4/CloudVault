// =============================================================
// client/src/ui/online_user_dialog.cpp
// 在线用户弹窗
// =============================================================

#include "online_user_dialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

QString dialogStyle() {
    return QStringLiteral(R"(
        QDialog {
            background: #F4F6F8;
            color: #111827;
            font-family: "DM Sans", "PingFang SC", "Microsoft YaHei", sans-serif;
        }

        QLabel#titleLabel {
            font-size: 18px;
            font-weight: 700;
            color: #111827;
        }

        QLabel#subtleLabel {
            font-size: 12px;
            color: #6B7280;
        }

        QLineEdit {
            min-height: 42px;
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

        QListWidget {
            background: #FFFFFF;
            border: 1px solid #E2E6EA;
            border-radius: 10px;
            outline: none;
        }

        QListWidget::item {
            padding: 10px 12px;
            border-bottom: 1px solid #F0F2F5;
        }

        QListWidget::item:selected {
            background: #EFF6FF;
            color: #111827;
        }

        QPushButton {
            min-height: 34px;
            border-radius: 8px;
            padding: 0 12px;
            font-size: 13px;
            font-weight: 600;
            border: none;
        }

        QPushButton#lightButton {
            background: #F8FAFC;
            color: #374151;
            border: 1px solid #E2E6EA;
        }

        QPushButton#lightButton:hover {
            background: #EFF6FF;
            border-color: #93C5FD;
        }

        QPushButton#primaryButton {
            background: #3B82F6;
            color: white;
        }

        QPushButton#primaryButton:hover {
            background: #2563EB;
        }
    )");
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
    setWindowTitle(QStringLiteral("在线用户"));
    setModal(true);
    resize(420, 480);
    setStyleSheet(dialogStyle());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    auto* title = new QLabel(
        QStringLiteral("在线用户（%1 人）").arg(std::count_if(
            users_.begin(), users_.end(), [](const auto& user) { return user.second; })),
        this);
    title->setObjectName(QStringLiteral("titleLabel"));
    root->addWidget(title);

    auto* hint = new QLabel(QStringLiteral("可直接输入用户名添加，也可从在线列表中选择。"), this);
    hint->setObjectName(QStringLiteral("subtleLabel"));
    root->addWidget(hint);

    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText(QStringLiteral("搜索…"));
    root->addWidget(search_edit_);

    user_list_ = new QListWidget(this);
    root->addWidget(user_list_, 1);

    auto* button_row = new QHBoxLayout();
    button_row->setSpacing(8);
    refresh_btn_ = new QPushButton(QStringLiteral("刷新"), this);
    refresh_btn_->setObjectName(QStringLiteral("lightButton"));
    add_btn_ = new QPushButton(QStringLiteral("+ 添加好友"), this);
    add_btn_->setObjectName(QStringLiteral("primaryButton"));
    add_btn_->setEnabled(false);
    button_row->addWidget(refresh_btn_);
    button_row->addStretch();
    button_row->addWidget(add_btn_);
    root->addLayout(button_row);
}

void OnlineUserDialog::connectSignals() {
    connect(search_edit_, &QLineEdit::textChanged,
            this, [this](const QString& text) {
                populateList(text);
                add_btn_->setEnabled(!text.trimmed().isEmpty() || user_list_->currentItem() != nullptr);
            });
    connect(user_list_, &QListWidget::itemSelectionChanged, this, [this] {
        add_btn_->setEnabled(!search_edit_->text().trimmed().isEmpty() ||
                             user_list_->currentItem() != nullptr);
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
    connect(user_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (!item) {
            return;
        }
        emit userChosen(item->data(Qt::UserRole).toString());
        accept();
    });
}

void OnlineUserDialog::populateList(const QString& keyword) {
    user_list_->clear();

    for (const auto& [username, online] : users_) {
        if (!online) {
            continue;
        }
        if (!keyword.trimmed().isEmpty() &&
            !username.contains(keyword.trimmed(), Qt::CaseInsensitive)) {
            continue;
        }

        auto* item = new QListWidgetItem(
            QStringLiteral("🔵 %1").arg(username), user_list_);
        item->setData(Qt::UserRole, username);
    }
}
