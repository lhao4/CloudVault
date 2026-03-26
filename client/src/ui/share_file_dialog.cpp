// =============================================================
// client/src/ui/share_file_dialog.cpp
// 文件分享弹窗
// =============================================================

#include "share_file_dialog.h"

#include <QFileInfo>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

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

        QLabel#fileHintLabel {
            background: #FFFFFF;
            border: 1px solid #E2E6EA;
            border-radius: 10px;
            color: #111827;
            font-size: 13px;
            padding: 10px 12px;
        }

        QLabel#selectionHintLabel {
            color: #6B7280;
            font-size: 12px;
        }

        QLineEdit {
            min-height: 40px;
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
            padding: 4px 8px;
            border-bottom: 1px solid #F0F2F5;
        }

        QCheckBox {
            spacing: 10px;
            font-size: 14px;
            color: #111827;
        }

        QCheckBox:disabled {
            color: #9CA3AF;
        }

        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 1px solid #CBD5E1;
            border-radius: 6px;
            background: white;
        }

        QCheckBox::indicator:checked {
            background: #3B82F6;
            border-color: #3B82F6;
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

        QLabel#emptyStateLabel {
            background: #F8FAFC;
            border: 1px dashed #CBD5E1;
            border-radius: 10px;
            color: #6B7280;
            font-size: 13px;
            padding: 14px 12px;
        }
    )");
}

} // namespace

ShareFileDialog::ShareFileDialog(const QString& file_name,
                                 const QList<QPair<QString, bool>>& friends,
                                 QWidget* parent)
    : QDialog(parent), file_name_(file_name), friends_(friends) {
    setupUi();
    connectSignals();
    populateList();
    updateConfirmButton();
}

void ShareFileDialog::setupUi() {
    setWindowTitle(QStringLiteral("分享文件"));
    setModal(true);
    resize(440, 500);
    setStyleSheet(dialogStyle());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* title = new QLabel(
        QStringLiteral("分享「%1」给好友").arg(file_name_), this);
    title->setObjectName(QStringLiteral("titleLabel"));
    root->addWidget(title);

    auto* hint = new QLabel(QStringLiteral("仅在线好友可立即接收，离线好友会保留展示但不可选。"), this);
    hint->setObjectName(QStringLiteral("subtleLabel"));
    root->addWidget(hint);

    file_hint_label_ = new QLabel(
        QStringLiteral("当前文件：%1").arg(QFileInfo(file_name_).fileName()), this);
    file_hint_label_->setObjectName(QStringLiteral("fileHintLabel"));
    root->addWidget(file_hint_label_);

    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText(QStringLiteral("搜索好友…"));
    root->addWidget(search_edit_);

    selection_hint_label_ = new QLabel(this);
    selection_hint_label_->setObjectName(QStringLiteral("selectionHintLabel"));
    root->addWidget(selection_hint_label_);

    friend_list_ = new QListWidget(this);
    root->addWidget(friend_list_, 1);

    empty_state_label_ = new QLabel(this);
    empty_state_label_->setObjectName(QStringLiteral("emptyStateLabel"));
    empty_state_label_->setAlignment(Qt::AlignCenter);
    empty_state_label_->hide();
    root->addWidget(empty_state_label_);

    auto* button_row = new QHBoxLayout();
    button_row->setSpacing(8);
    select_all_btn_ = new QPushButton(QStringLiteral("全选"), this);
    select_all_btn_->setObjectName(QStringLiteral("lightButton"));
    clear_btn_ = new QPushButton(QStringLiteral("取消全选"), this);
    clear_btn_->setObjectName(QStringLiteral("lightButton"));
    confirm_btn_ = new QPushButton(this);
    confirm_btn_->setObjectName(QStringLiteral("primaryButton"));
    button_row->addWidget(select_all_btn_);
    button_row->addWidget(clear_btn_);
    button_row->addStretch();
    button_row->addWidget(confirm_btn_);
    root->addLayout(button_row);
}

void ShareFileDialog::connectSignals() {
    connect(search_edit_, &QLineEdit::textChanged,
            this, [this](const QString& text) { populateList(text); });
    connect(select_all_btn_, &QPushButton::clicked, this, [this] {
        for (int i = 0; i < friend_list_->count(); ++i) {
            if (auto* item = friend_list_->item(i)) {
                auto* checkbox = qobject_cast<QCheckBox*>(friend_list_->itemWidget(item));
                if (checkbox && checkbox->isEnabled()) {
                    checkbox->setChecked(true);
                }
            }
        }
        updateConfirmButton();
    });
    connect(clear_btn_, &QPushButton::clicked, this, [this] {
        for (int i = 0; i < friend_list_->count(); ++i) {
            if (auto* item = friend_list_->item(i)) {
                auto* checkbox = qobject_cast<QCheckBox*>(friend_list_->itemWidget(item));
                if (checkbox) {
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

    for (const auto& [username, online] : friends_) {
        if (!trimmed.isEmpty() &&
            !username.contains(trimmed, Qt::CaseInsensitive)) {
            continue;
        }

        auto* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 36));

        auto* checkbox = new QCheckBox(friend_list_);
        checkbox->setText(
            online
                ? QStringLiteral("在线  %1").arg(username)
                : QStringLiteral("离线  %1  · 暂不可接收").arg(username));
        checkbox->setProperty("username", username);
        checkbox->setEnabled(online);
        friend_list_->addItem(item);
        friend_list_->setItemWidget(item, checkbox);
        connect(checkbox, &QCheckBox::toggled, this, &ShareFileDialog::updateConfirmButton);
        ++visible_count;
        if (online) {
            ++online_count;
        }
    }

    friend_list_->setVisible(visible_count > 0);
    empty_state_label_->setVisible(visible_count == 0);
    empty_state_label_->setText(
        trimmed.isEmpty()
            ? QStringLiteral("当前没有可展示的好友。先添加好友，再发起文件分享。")
            : QStringLiteral("没有匹配的好友，换个关键词再试。"));
    selection_hint_label_->setText(
        visible_count == 0
            ? QStringLiteral("0 位好友可见")
            : QStringLiteral("当前显示 %1 位好友，其中 %2 位在线可选")
                  .arg(visible_count)
                  .arg(online_count));
    updateConfirmButton();
}

void ShareFileDialog::updateConfirmButton() {
    const QStringList targets = selectedTargets();
    if (targets.isEmpty()) {
        confirm_btn_->setText(QStringLiteral("选择接收好友"));
    } else {
        confirm_btn_->setText(QStringLiteral("分享给 %1 位好友").arg(targets.size()));
    }
    confirm_btn_->setEnabled(!targets.isEmpty());
    clear_btn_->setEnabled(friend_list_->count() > 0);
    select_all_btn_->setEnabled(friend_list_->count() > 0);

    if (!targets.isEmpty()) {
        selection_hint_label_->setText(
            QStringLiteral("已选择：%1").arg(targets.join(QStringLiteral("、"))));
    }
}

QStringList ShareFileDialog::selectedTargets() const {
    QStringList result;
    for (int i = 0; i < friend_list_->count(); ++i) {
        if (auto* item = friend_list_->item(i)) {
            auto* checkbox = qobject_cast<QCheckBox*>(friend_list_->itemWidget(item));
            if (checkbox && checkbox->isChecked()) {
                result.append(checkbox->property("username").toString());
            }
        }
    }
    return result;
}
