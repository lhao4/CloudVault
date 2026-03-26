// =============================================================
// client/src/ui/share_file_dialog.cpp
// 文件分享弹窗
// =============================================================

#include "share_file_dialog.h"

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
            padding: 6px 10px;
            border-bottom: 1px solid #F0F2F5;
        }

        QCheckBox {
            spacing: 10px;
            font-size: 14px;
            color: #111827;
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
    resize(460, 520);
    setStyleSheet(dialogStyle());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    auto* title = new QLabel(
        QStringLiteral("分享「%1」给好友").arg(file_name_), this);
    title->setObjectName(QStringLiteral("titleLabel"));
    root->addWidget(title);

    auto* hint = new QLabel(QStringLiteral("当前仅展示已拉取到的好友列表。"), this);
    hint->setObjectName(QStringLiteral("subtleLabel"));
    root->addWidget(hint);

    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText(QStringLiteral("搜索好友…"));
    root->addWidget(search_edit_);

    friend_list_ = new QListWidget(this);
    root->addWidget(friend_list_, 1);

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
                if (checkbox) {
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

    for (const auto& [username, online] : friends_) {
        if (!keyword.trimmed().isEmpty() &&
            !username.contains(keyword.trimmed(), Qt::CaseInsensitive)) {
            continue;
        }

        auto* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 36));

        auto* checkbox = new QCheckBox(
            QStringLiteral("%1  %2")
                .arg(online ? QStringLiteral("🔵") : QStringLiteral("⚪"), username),
            friend_list_);
        checkbox->setProperty("username", username);
        friend_list_->addItem(item);
        friend_list_->setItemWidget(item, checkbox);
        connect(checkbox, &QCheckBox::toggled, this, &ShareFileDialog::updateConfirmButton);
    }
}

void ShareFileDialog::updateConfirmButton() {
    const QStringList targets = selectedTargets();
    confirm_btn_->setText(QStringLiteral("确认分享 (%1)").arg(targets.size()));
    confirm_btn_->setEnabled(!targets.isEmpty());
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
