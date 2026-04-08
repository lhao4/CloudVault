// =============================================================
// client/src/ui/sidebar_panel.cpp
// 左侧联系人面板
// =============================================================

#include "sidebar_panel.h"
#include "ui/widget_helpers.h"

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

using namespace cv_ui;

namespace {

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

QWidget* createContactItemWidget(const SidebarEntry& entry,
                                 bool selected,
                                 QWidget* parent = nullptr) {
    if (entry.type == SidebarItemType::Section) {
        auto* section = new QFrame(parent);
        section->setObjectName(QStringLiteral("sidebarSectionRow"));
        auto* layout = new QHBoxLayout(section);
        layout->setContentsMargins(12, 12, 12, 6);
        layout->setSpacing(8);

        auto* label = new QLabel(entry.title, section);
        label->setObjectName(QStringLiteral("sidebarSectionLabel"));
        layout->addWidget(label);
        layout->addStretch();

        if (!entry.tag.isEmpty()) {
            auto* tag = new QLabel(entry.tag, section);
            tag->setObjectName(QStringLiteral("sidebarSectionTag"));
            layout->addWidget(tag);
        }
        allowViewToHandleMouseEvents(section);
        return section;
    }

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

    const QString avatar_seed = (entry.type == SidebarItemType::Group)
        ? QStringLiteral("群")
        : (entry.type == SidebarItemType::Action ? QStringLiteral("＋") : entry.title);
    body_layout->addWidget(createAvatarWidget(avatar_seed, 40, entry.online, body),
                           0,
                           Qt::AlignVCenter);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 10, 0, 10);
    text_layout->setSpacing(2);

    auto* name_label = new QLabel(entry.title, body);
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

    if (!entry.tag.isEmpty()) {
        auto* badge = new QLabel(entry.tag, body);
        badge->setObjectName(QStringLiteral("contactTagLabel"));
        badge->setAlignment(Qt::AlignCenter);
        meta_layout->addWidget(badge, 0, Qt::AlignRight);
    } else if (entry.unread > 0) {
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
    sidebar_header->setFixedHeight(64);
    auto* sidebar_header_layout = new QHBoxLayout(sidebar_header);
    sidebar_header_layout->setContentsMargins(18, 0, 14, 0);
    sidebar_header_layout->setSpacing(8);

    title_label_ = new QLabel(QStringLiteral("消息"), sidebar_header);
    title_label_->setObjectName(QStringLiteral("sidebarTitle"));
    sidebar_header_layout->addWidget(title_label_);
    sidebar_header_layout->addStretch();

    action_btn_ = createIconButton(QStringLiteral("+"),
                                   QStringLiteral("快捷操作"),
                                   30,
                                   sidebar_header);
    sidebar_header_layout->addWidget(action_btn_);
    sidebar_layout->addWidget(sidebar_header);

    auto* sidebar_search_row = new QFrame(this);
    sidebar_search_row->setObjectName(QStringLiteral("sidebarSearchRow"));
    sidebar_search_row->setFixedHeight(54);
    auto* sidebar_search_layout = new QHBoxLayout(sidebar_search_row);
    sidebar_search_layout->setContentsMargins(16, 10, 16, 10);
    sidebar_search_layout->setSpacing(0);

    search_edit_ = new QLineEdit(sidebar_search_row);
    search_edit_->setObjectName(QStringLiteral("searchInput"));
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
    connect(contact_list_, &QListWidget::itemClicked,
            this, [this](QListWidgetItem*) { emit contactSelectionChanged(); });
    connect(action_btn_, &QPushButton::clicked,
            this, &SidebarPanel::actionRequested);
}

void SidebarPanel::setMode(SidebarMode mode) {
    mode_ = mode;
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

void SidebarPanel::populateEntries(const QList<SidebarEntry>& entries,
                                   const QString& selected_key,
                                   bool auto_select_first) {
    if (!contact_list_) {
        return;
    }

    const QSignalBlocker blocker(contact_list_);
    contact_list_->clear();

    for (const auto& entry : entries) {
        auto* item = new QListWidgetItem(contact_list_);
        item->setSizeHint(QSize(0, entry.type == SidebarItemType::Section ? 34 : 64));
        item->setData(Qt::UserRole, entry.key);
        item->setData(Qt::UserRole + 1, entry.online);
        item->setData(Qt::UserRole + 2, entry.title);
        item->setData(Qt::UserRole + 3, entry.id);
        item->setData(Qt::UserRole + 4, static_cast<int>(entry.type));

        const bool is_selected = entry.key == selected_key;
        contact_list_->setItemWidget(item,
                                     createContactItemWidget(entry, is_selected, contact_list_));
        if (entry.type == SidebarItemType::Section) {
            item->setFlags(Qt::NoItemFlags);
            continue;
        }
        if (is_selected) {
            contact_list_->setCurrentItem(item);
            item->setSelected(true);
        }
    }

    if (auto_select_first && selected_key.isEmpty()) {
        for (int i = 0; i < contact_list_->count(); ++i) {
            auto* item = contact_list_->item(i);
            if (item && item->flags() != Qt::NoItemFlags) {
                contact_list_->setCurrentItem(item);
                break;
            }
        }
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
        if (item->flags() == Qt::NoItemFlags) {
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

int SidebarPanel::itemCount() const {
    return contact_list_ ? contact_list_->count() : 0;
}

QString SidebarPanel::currentKey() const {
    if (contact_list_) {
        if (auto* item = contact_list_->currentItem()) {
            return item->data(Qt::UserRole).toString();
        }
    }
    return {};
}

QString SidebarPanel::currentTitle() const {
    if (contact_list_) {
        if (auto* item = contact_list_->currentItem()) {
            return item->data(Qt::UserRole + 2).toString();
        }
    }
    return {};
}

SidebarItemType SidebarPanel::currentItemType() const {
    if (contact_list_) {
        if (auto* item = contact_list_->currentItem()) {
            return static_cast<SidebarItemType>(item->data(Qt::UserRole + 4).toInt());
        }
    }
    return SidebarItemType::Friend;
}

int SidebarPanel::currentItemId() const {
    if (contact_list_) {
        if (auto* item = contact_list_->currentItem()) {
            return item->data(Qt::UserRole + 3).toInt();
        }
    }
    return 0;
}

bool SidebarPanel::currentOnline() const {
    if (contact_list_) {
        if (auto* item = contact_list_->currentItem()) {
            return item->data(Qt::UserRole + 1).toBool();
        }
    }
    return false;
}

SidebarMode SidebarPanel::mode() const {
    return mode_;
}
