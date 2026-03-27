// =============================================================
// client/src/ui/sidebar_panel.cpp
// 左侧联系人面板
// =============================================================

#include "sidebar_panel.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

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

void allowViewToHandleMouseEvents(QWidget* widget) {
    if (!widget) {
        return;
    }
    widget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    const auto children = widget->findChildren<QWidget*>();
    for (QWidget* child : children) {
        child->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }
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

QWidget* createAvatarWidget(const QString& seed,
                            int size,
                            bool online,
                            QWidget* parent = nullptr) {
    auto* wrap = new QWidget(parent);
    wrap->setFixedSize(size, size);

    auto* avatar = new QLabel(wrap);
    prepareAvatarBadge(avatar, seed, size);
    avatar->move(0, 0);

    if (size >= 34) {
        auto* dot = new QLabel(wrap);
        dot->setObjectName(QStringLiteral("onlineDot"));
        dot->setFixedSize(10, 10);
        dot->move(size - 12, size - 12);
        dot->setVisible(online);
    }

    return wrap;
}

QPushButton* createIconButton(const QString& text,
                              const QString& tooltip,
                              int size,
                              QWidget* parent = nullptr) {
    auto* button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("iconBtn"));
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(size, size);
    button->setToolTip(tooltip);
    return button;
}

QWidget* createContactItemWidget(const SidebarContactEntry& entry,
                                 bool selected,
                                 QWidget* parent = nullptr) {
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("contactCard"));
    frame->setProperty("selected", selected);

    auto* root = new QHBoxLayout(frame);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* accent = new QFrame(frame);
    accent->setObjectName(QStringLiteral("contactAccent"));
    accent->setProperty("selected", selected);
    accent->setFixedWidth(2);
    root->addWidget(accent);

    auto* body = new QWidget(frame);
    auto* body_layout = new QHBoxLayout(body);
    body_layout->setContentsMargins(10, 0, 12, 0);
    body_layout->setSpacing(10);

    body_layout->addWidget(createAvatarWidget(entry.username, 40, entry.online, body),
                           0,
                           Qt::AlignVCenter);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 10, 0, 10);
    text_layout->setSpacing(2);

    auto* name_label = new QLabel(entry.username, body);
    name_label->setObjectName(QStringLiteral("contactNameLabel"));
    text_layout->addWidget(name_label);

    auto* preview_label = new QLabel(entry.preview, body);
    preview_label->setObjectName(QStringLiteral("contactPreviewLabel"));
    preview_label->setWordWrap(false);
    text_layout->addWidget(preview_label);
    body_layout->addLayout(text_layout, 1);

    auto* meta_layout = new QVBoxLayout();
    meta_layout->setContentsMargins(0, 10, 0, 10);
    meta_layout->setSpacing(6);

    auto* time_label = new QLabel(entry.time, body);
    time_label->setObjectName(QStringLiteral("contactTimeLabel"));
    time_label->setAlignment(Qt::AlignRight);
    meta_layout->addWidget(time_label);

    if (entry.unread > 0) {
        auto* badge = new QLabel(QString::number(entry.unread), body);
        badge->setObjectName(QStringLiteral("contactBadgeLabel"));
        badge->setAlignment(Qt::AlignCenter);
        meta_layout->addWidget(badge, 0, Qt::AlignRight);
    } else {
        meta_layout->addStretch();
    }

    body_layout->addLayout(meta_layout);
    root->addWidget(body, 1);
    allowViewToHandleMouseEvents(frame);
    return frame;
}

} // namespace

SidebarPanel::SidebarPanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("sidebarPanel"));
    setMinimumWidth(260);
    setMaximumWidth(260);

    auto* sidebar_layout = new QVBoxLayout(this);
    sidebar_layout->setContentsMargins(0, 0, 0, 0);
    sidebar_layout->setSpacing(0);

    auto* sidebar_header = new QFrame(this);
    sidebar_header->setObjectName(QStringLiteral("sidebarHeader"));
    sidebar_header->setFixedHeight(56);
    auto* sidebar_header_layout = new QHBoxLayout(sidebar_header);
    sidebar_header_layout->setContentsMargins(16, 0, 12, 0);
    sidebar_header_layout->setSpacing(8);

    title_label_ = new QLabel(QStringLiteral("消息"), sidebar_header);
    title_label_->setObjectName(QStringLiteral("sidebarTitle"));
    sidebar_header_layout->addWidget(title_label_);
    sidebar_header_layout->addStretch();

    action_btn_ = createIconButton(QStringLiteral("群"),
                                   QStringLiteral("查看在线用户"),
                                   30,
                                   sidebar_header);
    sidebar_header_layout->addWidget(action_btn_);
    sidebar_layout->addWidget(sidebar_header);

    auto* sidebar_search_row = new QFrame(this);
    sidebar_search_row->setObjectName(QStringLiteral("sidebarSearchRow"));
    sidebar_search_row->setFixedHeight(48);
    auto* sidebar_search_layout = new QHBoxLayout(sidebar_search_row);
    sidebar_search_layout->setContentsMargins(12, 8, 12, 8);
    sidebar_search_layout->setSpacing(0);

    search_edit_ = new QLineEdit(sidebar_search_row);
    search_edit_->setObjectName(QStringLiteral("sidebarSearchEdit"));
    search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
    sidebar_search_layout->addWidget(search_edit_);
    sidebar_layout->addWidget(sidebar_search_row);

    contact_list_ = new QListWidget(this);
    contact_list_->setObjectName(QStringLiteral("contactList"));
    contact_list_->setSpacing(0);
    contact_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    sidebar_layout->addWidget(contact_list_, 1);

    connect(search_edit_, &QLineEdit::textChanged,
            this, &SidebarPanel::searchTextChanged);
    connect(contact_list_, &QListWidget::itemSelectionChanged,
            this, &SidebarPanel::contactSelectionChanged);
    connect(action_btn_, &QPushButton::clicked,
            this, &SidebarPanel::actionRequested);
}

void SidebarPanel::setHeader(const QString& title,
                             const QString& placeholder,
                             bool show_action) {
    if (title_label_) {
        title_label_->setText(title);
    }
    if (search_edit_) {
        search_edit_->setPlaceholderText(placeholder);
    }
    if (action_btn_) {
        action_btn_->setVisible(show_action);
    }
}

void SidebarPanel::populateContacts(const QList<SidebarContactEntry>& contacts,
                                    const QString& selected_username,
                                    bool auto_select_first) {
    if (!contact_list_) {
        return;
    }

    const QSignalBlocker blocker(contact_list_);
    contact_list_->clear();

    for (const auto& entry : contacts) {
        auto* item = new QListWidgetItem(contact_list_);
        item->setSizeHint(QSize(0, 64));
        item->setData(Qt::UserRole, entry.username);
        item->setData(Qt::UserRole + 1, entry.online);

        const bool is_selected = entry.username == selected_username;
        contact_list_->setItemWidget(item,
                                     createContactItemWidget(entry, is_selected, contact_list_));
        if (is_selected) {
            contact_list_->setCurrentItem(item);
            item->setSelected(true);
        }
    }

    if (auto_select_first && contact_list_->count() > 0 && selected_username.isEmpty()) {
        contact_list_->setCurrentRow(0);
    }
}

void SidebarPanel::refreshSelectionHighlights() {
    if (!contact_list_) {
        return;
    }
    for (int i = 0; i < contact_list_->count(); ++i) {
        auto* item = contact_list_->item(i);
        auto* widget = contact_list_->itemWidget(item);
        if (!widget) {
            continue;
        }
        const bool selected = item->isSelected();
        widget->setProperty("selected", selected);
        if (auto* accent = widget->findChild<QFrame*>(QStringLiteral("contactAccent"))) {
            accent->setProperty("selected", selected);
            repolish(accent);
        }
        repolish(widget);
    }
}

QString SidebarPanel::searchText() const {
    return search_edit_ ? search_edit_->text() : QString();
}

void SidebarPanel::clearSearch() {
    if (search_edit_) {
        search_edit_->clear();
    }
}

void SidebarPanel::focusSearchSelectAll() {
    if (search_edit_) {
        search_edit_->setFocus();
        search_edit_->selectAll();
    }
}

int SidebarPanel::contactCount() const {
    return contact_list_ ? contact_list_->count() : 0;
}

QString SidebarPanel::currentUsername() const {
    if (contact_list_) {
        if (auto* item = contact_list_->currentItem()) {
            return item->data(Qt::UserRole).toString();
        }
    }
    return {};
}

bool SidebarPanel::currentOnline() const {
    if (contact_list_) {
        if (auto* item = contact_list_->currentItem()) {
            return item->data(Qt::UserRole + 1).toBool();
        }
    }
    return false;
}
