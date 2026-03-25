// =============================================================
// client/src/ui/friend_page.cpp
// 好友页
// =============================================================

#include "friend_page.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

FriendPage::FriendPage(const QString& current_username,
                       cloudvault::FriendService& friend_service,
                       QWidget* parent)
    : QWidget(parent),
      current_username_(current_username),
      friend_service_(friend_service) {
    setupUi();
    connectSignals();
    friend_service_.flushFriends();
}

void FriendPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(12);

    auto* header = new QLabel(QStringLiteral("当前用户：%1").arg(current_username_), this);
    header->setStyleSheet("font-size: 18px; font-weight: bold; color: #111827;");
    root->addWidget(header);

    auto* search_row = new QHBoxLayout();
    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText(QStringLiteral("输入用户名搜索好友"));
    search_btn_ = new QPushButton(QStringLiteral("搜索"), this);
    add_btn_ = new QPushButton(QStringLiteral("添加好友"), this);
    add_btn_->setEnabled(false);
    search_row->addWidget(search_edit_, 1);
    search_row->addWidget(search_btn_);
    search_row->addWidget(add_btn_);
    root->addLayout(search_row);

    search_result_label_ = new QLabel(QStringLiteral("搜索结果：无"), this);
    search_result_label_->setStyleSheet("color: #4B5563;");
    root->addWidget(search_result_label_);

    auto* list_header = new QHBoxLayout();
    auto* title = new QLabel(QStringLiteral("好友列表"), this);
    title->setStyleSheet("font-size: 16px; font-weight: bold;");
    refresh_btn_ = new QPushButton(QStringLiteral("刷新"), this);
    delete_btn_ = new QPushButton(QStringLiteral("删除好友"), this);
    delete_btn_->setEnabled(false);
    list_header->addWidget(title);
    list_header->addStretch();
    list_header->addWidget(refresh_btn_);
    list_header->addWidget(delete_btn_);
    root->addLayout(list_header);

    friends_list_ = new QListWidget(this);
    root->addWidget(friends_list_, 1);

    status_label_ = new QLabel(QStringLiteral("就绪"), this);
    status_label_->setStyleSheet("color: #2563EB;");
    root->addWidget(status_label_);
}

void FriendPage::connectSignals() {
    connect(search_btn_, &QPushButton::clicked, this, &FriendPage::onSearchClicked);
    connect(add_btn_, &QPushButton::clicked, this, &FriendPage::onAddClicked);
    connect(refresh_btn_, &QPushButton::clicked, this, &FriendPage::onRefreshClicked);
    connect(delete_btn_, &QPushButton::clicked, this, &FriendPage::onDeleteClicked);
    connect(friends_list_, &QListWidget::itemSelectionChanged,
            this, &FriendPage::onFriendSelectionChanged);

    connect(&friend_service_, &cloudvault::FriendService::userFound,
            this, [this](const QString& username, bool online) {
                searched_username_ = username;
                search_result_label_->setText(
                    QStringLiteral("搜索结果：%1（%2）")
                        .arg(username, online ? QStringLiteral("在线")
                                              : QStringLiteral("离线")));
                add_btn_->setEnabled(username != current_username_);
                setStatus(QStringLiteral("找到用户 %1").arg(username));
            });

    connect(&friend_service_, &cloudvault::FriendService::userNotFound,
            this, [this](const QString& username) {
                searched_username_.clear();
                add_btn_->setEnabled(false);
                search_result_label_->setText(QStringLiteral("搜索结果：未找到 %1").arg(username));
                setStatus(QStringLiteral("用户不存在"), true);
            });

    connect(&friend_service_, &cloudvault::FriendService::friendRequestSent,
            this, [this](const QString& target) {
                setStatus(QStringLiteral("好友请求已发送给 %1").arg(target));
            });

    connect(&friend_service_, &cloudvault::FriendService::incomingFriendRequest,
            this, [this](const QString& from) {
                const auto result = QMessageBox::question(
                    this,
                    QStringLiteral("好友请求"),
                    QStringLiteral("%1 想添加你为好友，是否同意？").arg(from));
                if (result == QMessageBox::Yes) {
                    friend_service_.agreeFriend(from);
                } else {
                    setStatus(QStringLiteral("已忽略来自 %1 的好友请求").arg(from));
                }
            });

    connect(&friend_service_, &cloudvault::FriendService::friendAdded,
            this, [this](const QString& username) {
                setStatus(QStringLiteral("已与 %1 成为好友").arg(username));
                friend_service_.flushFriends();
            });

    connect(&friend_service_, &cloudvault::FriendService::friendAddFailed,
            this, [this](const QString& reason) {
                setStatus(reason, true);
            });

    connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
            this, [this](const QList<QPair<QString, bool>>& friends) {
                friends_list_->clear();
                for (const auto& item : friends) {
                    auto* list_item = new QListWidgetItem(
                        QStringLiteral("%1  [%2]")
                            .arg(item.first, item.second ? QStringLiteral("在线")
                                                         : QStringLiteral("离线")),
                        friends_list_);
                    list_item->setData(Qt::UserRole, item.first);
                }
                delete_btn_->setEnabled(friends_list_->currentItem() != nullptr);
                setStatus(QStringLiteral("好友列表已刷新，共 %1 人").arg(friends.size()));
            });

    connect(&friend_service_, &cloudvault::FriendService::friendListRefreshFailed,
            this, [this](const QString& reason) {
                setStatus(reason, true);
            });

    connect(&friend_service_, &cloudvault::FriendService::friendDeleted,
            this, [this](const QString& username) {
                setStatus(QStringLiteral("已删除好友 %1").arg(username));
                friend_service_.flushFriends();
            });

    connect(&friend_service_, &cloudvault::FriendService::friendDeleteFailed,
            this, [this](const QString& reason) {
                setStatus(reason, true);
            });
}

void FriendPage::onSearchClicked() {
    const QString target = search_edit_->text().trimmed();
    if (target.isEmpty()) {
        setStatus(QStringLiteral("请输入要搜索的用户名"), true);
        return;
    }
    friend_service_.findUser(target);
}

void FriendPage::onAddClicked() {
    if (searched_username_.isEmpty()) {
        setStatus(QStringLiteral("请先搜索用户"), true);
        return;
    }
    friend_service_.addFriend(searched_username_);
}

void FriendPage::onRefreshClicked() {
    friend_service_.flushFriends();
}

void FriendPage::onDeleteClicked() {
    const QString target = selectedFriend();
    if (target.isEmpty()) {
        setStatus(QStringLiteral("请先选择一个好友"), true);
        return;
    }

    const auto result = QMessageBox::question(
        this,
        QStringLiteral("删除好友"),
        QStringLiteral("确定删除好友 %1 吗？").arg(target));
    if (result == QMessageBox::Yes) {
        friend_service_.deleteFriend(target);
    }
}

void FriendPage::onFriendSelectionChanged() {
    delete_btn_->setEnabled(!selectedFriend().isEmpty());
}

void FriendPage::setStatus(const QString& text, bool error) {
    status_label_->setText(text);
    status_label_->setStyleSheet(error ? "color: #DC2626;" : "color: #2563EB;");
}

QString FriendPage::selectedFriend() const {
    if (auto* item = friends_list_->currentItem()) {
        return item->data(Qt::UserRole).toString();
    }
    return {};
}
