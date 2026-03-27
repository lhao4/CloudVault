// =============================================================
// client/src/ui/file_panel.cpp
// 文件中栏面板
// =============================================================

#include "file_panel.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

FilePanel::FilePanel(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("filePage"));
    auto* page_layout = new QVBoxLayout(this);
    page_layout->setContentsMargins(0, 0, 0, 0);
    page_layout->setSpacing(0);

    auto* top_bar = new QFrame(this);
    top_bar->setObjectName(QStringLiteral("fileTopBar"));
    top_bar->setFixedHeight(56);
    auto* top_layout = new QHBoxLayout(top_bar);
    top_layout->setContentsMargins(20, 0, 20, 0);
    top_layout->setSpacing(8);

    path_label_ = new QLabel(QStringLiteral("/"), top_bar);
    path_label_->setObjectName(QStringLiteral("filePathLabel"));
    path_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    top_layout->addWidget(path_label_, 1);

    back_btn_ = new QPushButton(QStringLiteral("返回"), top_bar);
    back_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    top_layout->addWidget(back_btn_);

    upload_btn_ = new QPushButton(QStringLiteral("上传"), top_bar);
    upload_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    top_layout->addWidget(upload_btn_);

    refresh_btn_ = new QPushButton(QStringLiteral("刷新"), top_bar);
    refresh_btn_->setObjectName(QStringLiteral("fileActionBtn"));
    top_layout->addWidget(refresh_btn_);

    create_btn_ = new QPushButton(QStringLiteral("+ 新建"), top_bar);
    create_btn_->setObjectName(QStringLiteral("primaryFileBtn"));
    top_layout->addWidget(create_btn_);
    page_layout->addWidget(top_bar);

    auto* search_row = new QFrame(this);
    search_row->setObjectName(QStringLiteral("fileSearchRow"));
    auto* search_layout = new QHBoxLayout(search_row);
    search_layout->setContentsMargins(16, 10, 16, 10);
    search_layout->setSpacing(0);

    search_edit_ = new QLineEdit(search_row);
    search_edit_->setPlaceholderText(QStringLiteral("搜索文件、回车确认…"));
    search_layout->addWidget(search_edit_);
    page_layout->addWidget(search_row);

    auto* table_header = new QFrame(this);
    table_header->setObjectName(QStringLiteral("fileTableHeader"));
    table_header->setFixedHeight(40);
    auto* table_header_layout = new QHBoxLayout(table_header);
    table_header_layout->setContentsMargins(14, 0, 14, 0);
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
    footer->setFixedHeight(56);
    auto* footer_layout = new QHBoxLayout(footer);
    footer_layout->setContentsMargins(16, 0, 16, 0);
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
    delete_btn_->setObjectName(QStringLiteral("deleteBtn"));
    footer_layout->addWidget(delete_btn_);
    page_layout->addWidget(footer);
}

QLabel* FilePanel::pathLabel() const { return path_label_; }
QLabel* FilePanel::statusLabel() const { return status_label_; }
QLineEdit* FilePanel::searchEdit() const { return search_edit_; }
QListWidget* FilePanel::fileList() const { return file_list_; }
QLabel* FilePanel::emptyStateLabel() const { return empty_state_label_; }
QPushButton* FilePanel::backButton() const { return back_btn_; }
QPushButton* FilePanel::uploadButton() const { return upload_btn_; }
QPushButton* FilePanel::refreshButton() const { return refresh_btn_; }
QPushButton* FilePanel::createButton() const { return create_btn_; }
QFrame* FilePanel::transferRow() const { return transfer_row_; }
QLabel* FilePanel::transferLabel() const { return transfer_label_; }
QLabel* FilePanel::transferPercentLabel() const { return transfer_percent_label_; }
QProgressBar* FilePanel::transferBar() const { return transfer_bar_; }
QPushButton* FilePanel::transferCancelButton() const { return transfer_cancel_btn_; }
QLabel* FilePanel::selectionLabel() const { return selection_label_; }
QLabel* FilePanel::metaLabel() const { return meta_label_; }
QPushButton* FilePanel::downloadButton() const { return download_btn_; }
QPushButton* FilePanel::shareButton() const { return share_btn_; }
QPushButton* FilePanel::renameButton() const { return rename_btn_; }
QPushButton* FilePanel::moveButton() const { return move_btn_; }
QPushButton* FilePanel::deleteButton() const { return delete_btn_; }
