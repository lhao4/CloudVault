// =============================================================
// client/src/ui/file_panel.cpp
// 文件中栏面板
// =============================================================

#include "file_panel.h"
#include "ui/widget_helpers.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

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

QLabel* createStandardIconLabel(QStyle::StandardPixmap icon_type,
                                int size,
                                const QString& object_name,
                                QWidget* parent = nullptr) {
    auto* label = new QLabel(parent);
    label->setObjectName(object_name);
    label->setFixedSize(size, size);
    label->setAlignment(Qt::AlignCenter);
    if (parent) {
        label->setPixmap(parent->style()->standardIcon(icon_type).pixmap(size, size));
    }
    return label;
}

QWidget* createFileItemWidget(const cloudvault::FileEntry& entry,
                              bool selected,
                              QWidget* parent = nullptr) {
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("fileRowCard"));
    frame->setProperty("selected", selected);

    auto* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* accent = new QFrame(frame);
    accent->setObjectName(QStringLiteral("fileRowAccent"));
    accent->setProperty("selected", selected);
    accent->setFixedWidth(2);
    layout->addWidget(accent);

    auto* body = new QWidget(frame);
    body->setMinimumHeight(52);
    auto* row = new QHBoxLayout(body);
    row->setContentsMargins(14, 0, 14, 0);
    row->setSpacing(12);

    auto* icon = createStandardIconLabel(entry.is_dir ? QStyle::SP_DirIcon
                                                      : QStyle::SP_FileIcon,
                                         20,
                                         QStringLiteral("fileIconLabel"),
                                         body);
    row->addWidget(icon);

    auto* name_label = new QLabel(entry.name, body);
    name_label->setObjectName(QStringLiteral("fileNameLabel"));
    name_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    row->addWidget(name_label, 1);

    auto* size_label = new QLabel(entry.is_dir ? QStringLiteral("—")
                                               : formatFileSize(entry.size),
                                  body);
    size_label->setObjectName(QStringLiteral("fileCellLabel"));
    size_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    size_label->setFixedWidth(120);
    row->addWidget(size_label);

    auto* time_label = new QLabel(entry.modified_at, body);
    time_label->setObjectName(QStringLiteral("fileCellLabel"));
    time_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    time_label->setFixedWidth(140);
    row->addWidget(time_label);

    auto* type_label = new QLabel(entry.is_dir ? QStringLiteral("文件夹")
                                               : QStringLiteral("文件"),
                                  body);
    type_label->setObjectName(QStringLiteral("fileTypeLabel"));
    type_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    type_label->setFixedWidth(80);
    row->addWidget(type_label);

    layout->addWidget(body, 1);
    frame->setToolTip(entry.path);
    allowViewToHandleMouseEvents(frame);
    return frame;
}

void clearLayout(QLayout* layout) {
    if (!layout) {
        return;
    }
    while (auto* item = layout->takeAt(0)) {
        if (auto* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

} // namespace

FilePanel::FilePanel(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("filePage"));
    auto* page_layout = new QVBoxLayout(this);
    page_layout->setContentsMargins(0, 0, 0, 0);
    page_layout->setSpacing(0);

    auto* top_bar = new QFrame(this);
    top_bar->setObjectName(QStringLiteral("fileTopBar"));
    auto* top_layout = new QVBoxLayout(top_bar);
    top_layout->setContentsMargins(24, 12, 24, 12);
    top_layout->setSpacing(10);

    auto* summary_row = new QWidget(top_bar);
    auto* top_summary_layout = new QHBoxLayout(summary_row);
    top_summary_layout->setContentsMargins(0, 0, 0, 0);
    top_summary_layout->setSpacing(8);

    path_label_ = new QLabel(QStringLiteral("/"), summary_row);
    path_label_->setObjectName(QStringLiteral("filePathLabel"));
    path_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    top_summary_layout->addWidget(path_label_, 1);

    path_meta_label_ = new QLabel(summary_row);
    path_meta_label_->setObjectName(QStringLiteral("filePathMetaLabel"));
    path_meta_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    top_summary_layout->addWidget(path_meta_label_);

    back_btn_ = new QPushButton(QStringLiteral("返回"), summary_row);
    back_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    top_summary_layout->addWidget(back_btn_);

    upload_btn_ = new QPushButton(QStringLiteral("上传"), summary_row);
    upload_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    top_summary_layout->addWidget(upload_btn_);

    refresh_btn_ = new QPushButton(QStringLiteral("刷新"), summary_row);
    refresh_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    top_summary_layout->addWidget(refresh_btn_);

    create_btn_ = new QPushButton(QStringLiteral("+ 新建"), summary_row);
    create_btn_->setObjectName(QStringLiteral("primaryFileBtn"));
    top_summary_layout->addWidget(create_btn_);
    top_layout->addWidget(summary_row);

    breadcrumb_row_ = new QWidget(top_bar);
    breadcrumb_row_->setObjectName(QStringLiteral("fileBreadcrumbRow"));
    breadcrumb_layout_ = new QHBoxLayout(breadcrumb_row_);
    breadcrumb_layout_->setContentsMargins(0, 0, 0, 0);
    breadcrumb_layout_->setSpacing(6);
    top_layout->addWidget(breadcrumb_row_);
    page_layout->addWidget(top_bar);

    auto* search_row = new QFrame(this);
    search_row->setObjectName(QStringLiteral("fileSearchRow"));
    auto* search_layout = new QHBoxLayout(search_row);
    search_layout->setContentsMargins(20, 12, 20, 12);
    search_layout->setSpacing(0);

    search_edit_ = new QLineEdit(search_row);
    search_edit_->setObjectName(QStringLiteral("fileSearchInput"));
    search_edit_->setPlaceholderText(QStringLiteral("搜索文件、回车确认…"));
    search_layout->addWidget(search_edit_);
    page_layout->addWidget(search_row);

    auto* table_header = new QFrame(this);
    table_header->setObjectName(QStringLiteral("fileTableHeader"));
    table_header->setFixedHeight(44);
    auto* table_header_layout = new QHBoxLayout(table_header);
    table_header_layout->setContentsMargins(16, 0, 16, 0);
    table_header_layout->setSpacing(12);

    auto* name_header = new QLabel(QStringLiteral("名称"), table_header);
    name_header->setObjectName(QStringLiteral("fileColumnLabel"));
    table_header_layout->addSpacing(22);
    table_header_layout->addWidget(name_header, 1);

    auto* size_header = new QLabel(QStringLiteral("大小"), table_header);
    size_header->setObjectName(QStringLiteral("fileColumnLabel"));
    size_header->setFixedWidth(120);
    size_header->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table_header_layout->addWidget(size_header);

    auto* time_header = new QLabel(QStringLiteral("修改时间"), table_header);
    time_header->setObjectName(QStringLiteral("fileColumnLabel"));
    time_header->setFixedWidth(140);
    time_header->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table_header_layout->addWidget(time_header);

    auto* type_header = new QLabel(QStringLiteral("类型"), table_header);
    type_header->setObjectName(QStringLiteral("fileColumnLabel"));
    type_header->setFixedWidth(80);
    type_header->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table_header_layout->addWidget(type_header);
    page_layout->addWidget(table_header);

    file_list_ = new QListWidget(this);
    file_list_->setObjectName(QStringLiteral("fileList"));
    file_list_->setSpacing(0);
    file_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    page_layout->addWidget(file_list_, 1);

    empty_state_label_ = new QLabel(
        QStringLiteral("当前目录为空\n点击“+ 新建”或使用“上传”开始管理文件"),
        this);
    empty_state_label_->setObjectName(QStringLiteral("emptyStateLabel"));
    empty_state_label_->setAlignment(Qt::AlignCenter);
    empty_state_label_->hide();
    page_layout->addWidget(empty_state_label_);

    transfer_row_ = new QFrame(this);
    transfer_row_->setObjectName(QStringLiteral("fileTransferRow"));
    transfer_row_->hide();
    auto* transfer_layout = new QHBoxLayout(transfer_row_);
    transfer_layout->setContentsMargins(16, 0, 16, 0);
    transfer_layout->setSpacing(10);

    transfer_label_ = new QLabel(QStringLiteral("文件传输"), transfer_row_);
    transfer_label_->setObjectName(QStringLiteral("transferLabel"));
    transfer_layout->addWidget(transfer_label_);

    transfer_bar_ = new QProgressBar(transfer_row_);
    transfer_bar_->setTextVisible(false);
    transfer_bar_->setRange(0, 100);
    transfer_layout->addWidget(transfer_bar_, 1);

    transfer_percent_label_ = new QLabel(QStringLiteral("0%"), transfer_row_);
    transfer_percent_label_->setObjectName(QStringLiteral("transferPercent"));
    transfer_layout->addWidget(transfer_percent_label_);

    transfer_cancel_btn_ = new QPushButton(QStringLiteral("取消"), transfer_row_);
    transfer_cancel_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    transfer_cancel_btn_->setFixedHeight(28);
    transfer_layout->addWidget(transfer_cancel_btn_);
    page_layout->addWidget(transfer_row_);

    auto* footer = new QFrame(this);
    footer->setObjectName(QStringLiteral("fileActionBar"));
    footer->setFixedHeight(64);
    auto* footer_layout = new QHBoxLayout(footer);
    footer_layout->setContentsMargins(20, 0, 20, 0);
    footer_layout->setSpacing(8);

    auto* summary_layout = new QVBoxLayout();
    summary_layout->setContentsMargins(0, 8, 0, 8);
    summary_layout->setSpacing(1);
    selection_label_ = new QLabel(QStringLiteral("选中：未选择文件"), footer);
    selection_label_->setObjectName(QStringLiteral("fileSelectionLabel"));
    summary_layout->addWidget(selection_label_);

    meta_label_ = new QLabel(QStringLiteral("未选择文件"), footer);
    meta_label_->setObjectName(QStringLiteral("fileStatusLabel"));
    summary_layout->addWidget(meta_label_);
    status_label_ = meta_label_;
    footer_layout->addLayout(summary_layout, 1);

    download_btn_ = new QPushButton(QStringLiteral("下载"), footer);
    download_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    footer_layout->addWidget(download_btn_);

    share_btn_ = new QPushButton(QStringLiteral("分享"), footer);
    share_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    footer_layout->addWidget(share_btn_);

    rename_btn_ = new QPushButton(QStringLiteral("重命名"), footer);
    rename_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    footer_layout->addWidget(rename_btn_);

    move_btn_ = new QPushButton(QStringLiteral("移动"), footer);
    move_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    footer_layout->addWidget(move_btn_);

    delete_btn_ = new QPushButton(QStringLiteral("删除"), footer);
    delete_btn_->setObjectName(QStringLiteral("deleteBtnFile"));
    footer_layout->addWidget(delete_btn_);
    page_layout->addWidget(footer);

    connect(back_btn_, &QPushButton::clicked, this, &FilePanel::backRequested);
    connect(upload_btn_, &QPushButton::clicked, this, &FilePanel::uploadRequested);
    connect(refresh_btn_, &QPushButton::clicked, this, &FilePanel::refreshRequested);
    connect(create_btn_, &QPushButton::clicked, this, &FilePanel::createRequested);
    connect(search_edit_, &QLineEdit::returnPressed, this, &FilePanel::searchRequested);
    connect(search_edit_, &QLineEdit::textChanged, this, &FilePanel::searchTextChanged);
    connect(file_list_, &QListWidget::itemSelectionChanged, this, &FilePanel::selectionChanged);
    connect(file_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        emit itemActivated();
    });
    connect(rename_btn_, &QPushButton::clicked, this, &FilePanel::renameRequested);
    connect(move_btn_, &QPushButton::clicked, this, &FilePanel::moveRequested);
    connect(delete_btn_, &QPushButton::clicked, this, &FilePanel::deleteRequested);
    connect(download_btn_, &QPushButton::clicked, this, &FilePanel::downloadRequested);
    connect(share_btn_, &QPushButton::clicked, this, &FilePanel::shareRequested);
    connect(transfer_cancel_btn_, &QPushButton::clicked, this, &FilePanel::transferCancelRequested);
}

void FilePanel::setStatusMessage(const QString& message, bool error) {
    if (!status_label_) {
        return;
    }
    status_label_->setProperty("error", error);
    status_label_->setText(message);
    repolish(status_label_);
}

void FilePanel::resetSelectionSummary() {
    setSelectionSummary(QStringLiteral("未选择文件"),
                        QStringLiteral("未选择文件"));
}

void FilePanel::setSelectionSummary(const QString& name,
                                    const QString& meta,
                                    const QString& tooltip) {
    if (selection_label_) {
        selection_label_->setText(QStringLiteral("选中：%1").arg(name));
    }
    if (meta_label_) {
        meta_label_->setText(meta);
        meta_label_->setToolTip(tooltip);
    }
}

void FilePanel::setTransferState(const QString& title, int percent, bool cancellable) {
    if (!transfer_row_) {
        return;
    }
    const int clamped = std::clamp(percent, 0, 100);
    transfer_row_->show();
    if (transfer_label_) {
        transfer_label_->setText(title);
    }
    if (transfer_bar_) {
        transfer_bar_->setValue(clamped);
    }
    if (transfer_percent_label_) {
        transfer_percent_label_->setText(QStringLiteral("%1%").arg(clamped));
    }
    if (transfer_cancel_btn_) {
        transfer_cancel_btn_->setEnabled(cancellable);
    }
}

void FilePanel::clearTransferState() {
    if (transfer_row_) {
        transfer_row_->hide();
    }
    if (transfer_bar_) {
        transfer_bar_->setValue(0);
    }
    if (transfer_percent_label_) {
        transfer_percent_label_->setText(QStringLiteral("0%"));
    }
}

void FilePanel::populateEntries(const cloudvault::FileEntries& entries) {
    if (!file_list_) {
        return;
    }
    file_list_->clear();
    for (const auto& entry : entries) {
        auto* item = new QListWidgetItem(file_list_);
        item->setSizeHint(QSize(0, 52));
        item->setData(Qt::UserRole, entry.path);
        item->setData(Qt::UserRole + 1, entry.is_dir);
        item->setData(Qt::UserRole + 2, entry.name);
        file_list_->setItemWidget(item, createFileItemWidget(entry, false, file_list_));
    }
}

void FilePanel::setPathState(const QString& text, const QString& tooltip, bool can_go_back) {
    if (path_label_) {
        path_label_->setText(text);
        path_label_->setToolTip(tooltip);
    }
    if (back_btn_) {
        back_btn_->setEnabled(can_go_back);
    }
}

void FilePanel::setBreadcrumbs(const QList<QPair<QString, QString>>& crumbs,
                               const QString& active_path) {
    if (!breadcrumb_layout_ || !breadcrumb_row_) {
        return;
    }

    clearLayout(breadcrumb_layout_);
    const bool has_crumbs = !crumbs.isEmpty();
    breadcrumb_row_->setVisible(has_crumbs);
    if (!has_crumbs) {
        return;
    }

    for (int i = 0; i < crumbs.size(); ++i) {
        const auto& crumb = crumbs.at(i);
        auto* button = new QPushButton(crumb.first, breadcrumb_row_);
        button->setObjectName(QStringLiteral("fileBreadcrumbBtn"));
        button->setProperty("active", crumb.second == active_path);
        button->setFlat(true);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, [this, path = crumb.second] {
            emit breadcrumbRequested(path);
        });
        breadcrumb_layout_->addWidget(button, 0);
        repolish(button);

        if (i < crumbs.size() - 1) {
            auto* separator = new QLabel(QStringLiteral("/"), breadcrumb_row_);
            separator->setObjectName(QStringLiteral("fileBreadcrumbSep"));
            breadcrumb_layout_->addWidget(separator, 0);
        }
    }
    breadcrumb_layout_->addStretch(1);
}

void FilePanel::setContextSummary(const QString& title, const QString& meta) {
    if (path_label_) {
        path_label_->setText(title);
    }
    if (path_meta_label_) {
        path_meta_label_->setText(meta);
        path_meta_label_->setVisible(!meta.trimmed().isEmpty());
    }
}

void FilePanel::setEmptyState(const QString& text, bool empty) {
    if (empty_state_label_) {
        empty_state_label_->setText(text);
        empty_state_label_->setVisible(empty);
    }
    if (file_list_) {
        file_list_->setVisible(!empty);
    }
}

void FilePanel::selectFirstEntry() {
    if (file_list_ && file_list_->count() > 0) {
        file_list_->setCurrentRow(0);
    }
}

void FilePanel::refreshSelectionHighlights() {
    if (!file_list_) {
        return;
    }
    for (int i = 0; i < file_list_->count(); ++i) {
        auto* item = file_list_->item(i);
        auto* widget = file_list_->itemWidget(item);
        if (!widget) {
            continue;
        }
        widget->setProperty("selected", item->isSelected());
        if (auto* accent = widget->findChild<QFrame*>(QStringLiteral("fileRowAccent"))) {
            accent->setProperty("selected", item->isSelected());
            repolish(accent);
        }
        repolish(widget);
    }
}

void FilePanel::clearCurrentSelection() {
    if (!file_list_) {
        return;
    }
    file_list_->clearSelection();
    file_list_->setCurrentItem(nullptr);
}

QString FilePanel::searchText() const {
    return search_edit_ ? search_edit_->text() : QString();
}

void FilePanel::clearSearch() {
    if (search_edit_) {
        search_edit_->clear();
    }
}

void FilePanel::focusSearchSelectAll() {
    if (search_edit_) {
        search_edit_->setFocus();
        search_edit_->selectAll();
    }
}

int FilePanel::itemCount() const {
    return file_list_ ? file_list_->count() : 0;
}

bool FilePanel::hasSelection() const {
    return file_list_ && file_list_->currentItem();
}

QString FilePanel::currentPath() const {
    if (file_list_) {
        if (auto* item = file_list_->currentItem()) {
            return item->data(Qt::UserRole).toString();
        }
    }
    return {};
}

QString FilePanel::currentName() const {
    if (file_list_) {
        if (auto* item = file_list_->currentItem()) {
            return item->data(Qt::UserRole + 2).toString();
        }
    }
    return {};
}

bool FilePanel::currentIsDir() const {
    if (file_list_) {
        if (auto* item = file_list_->currentItem()) {
            return item->data(Qt::UserRole + 1).toBool();
        }
    }
    return false;
}

void FilePanel::setActionButtonsEnabled(bool has_selection, bool is_file) {
    if (download_btn_) {
        download_btn_->setEnabled(is_file);
    }
    if (share_btn_) {
        share_btn_->setEnabled(is_file);
    }
    if (rename_btn_) {
        rename_btn_->setEnabled(has_selection);
    }
    if (move_btn_) {
        move_btn_->setEnabled(has_selection);
    }
    if (delete_btn_) {
        delete_btn_->setEnabled(has_selection);
    }
}

void FilePanel::setTransferCancelEnabled(bool enabled) {
    if (transfer_cancel_btn_) {
        transfer_cancel_btn_->setEnabled(enabled);
    }
}

QListWidget* FilePanel::fileList() const { return file_list_; }
