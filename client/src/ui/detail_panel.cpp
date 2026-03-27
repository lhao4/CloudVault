// =============================================================
// client/src/ui/detail_panel.cpp
// 右侧详情面板
// =============================================================

#include "detail_panel.h"

#include <QHBoxLayout>
#include <QLabel>
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

} // namespace

DetailPanel::DetailPanel(const QString& current_username, QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("detailPanel"));
    setMinimumWidth(220);
    setMaximumWidth(220);
    auto* detail_layout = new QVBoxLayout(this);
    detail_layout->setContentsMargins(0, 0, 0, 0);
    detail_layout->setSpacing(0);

    auto* detail_header = new QFrame(this);
    detail_header->setObjectName(QStringLiteral("detailHeader"));
    detail_header->setFixedHeight(56);
    auto* detail_header_layout = new QHBoxLayout(detail_header);
    detail_header_layout->setContentsMargins(16, 0, 16, 0);
    auto* detail_title = new QLabel(QStringLiteral("详情"), detail_header);
    detail_title->setObjectName(QStringLiteral("detailTitle"));
    detail_header_layout->addWidget(detail_title);
    detail_header_layout->addStretch();
    detail_layout->addWidget(detail_header);

    auto* detail_body = new QWidget(this);
    auto* detail_body_layout = new QVBoxLayout(detail_body);
    detail_body_layout->setContentsMargins(14, 14, 14, 14);
    detail_body_layout->setSpacing(14);

    auto* contact_section = new QFrame(detail_body);
    contact_section->setObjectName(QStringLiteral("detailSection"));
    auto* contact_section_layout = new QVBoxLayout(contact_section);
    contact_section_layout->setContentsMargins(14, 14, 14, 14);
    contact_section_layout->setSpacing(10);

    auto* contact_kicker = new QLabel(QStringLiteral("CONTACT"), contact_section);
    contact_kicker->setObjectName(QStringLiteral("sectionKicker"));
    contact_section_layout->addWidget(contact_kicker);

    auto* contact_info_row = new QHBoxLayout();
    contact_info_row->setSpacing(8);
    contact_avatar_label_ = new QLabel(contact_section);
    prepareAvatarBadge(contact_avatar_label_, current_username, 26);
    contact_info_row->addWidget(contact_avatar_label_, 0, Qt::AlignTop);

    auto* contact_text_layout = new QVBoxLayout();
    contact_text_layout->setContentsMargins(0, 0, 0, 0);
    contact_text_layout->setSpacing(1);
    contact_name_label_ = new QLabel(QStringLiteral("未选择联系人"), contact_section);
    contact_name_label_->setObjectName(QStringLiteral("detailContactName"));
    contact_text_layout->addWidget(contact_name_label_);

    contact_status_label_ = new QLabel(QStringLiteral("请选择左侧联系人"), contact_section);
    contact_status_label_->setObjectName(QStringLiteral("detailMeta"));
    contact_text_layout->addWidget(contact_status_label_);
    contact_info_row->addLayout(contact_text_layout, 1);
    contact_section_layout->addLayout(contact_info_row);
    detail_body_layout->addWidget(contact_section);

    auto* files_section = new QFrame(detail_body);
    files_section->setObjectName(QStringLiteral("detailSection"));
    auto* files_section_layout = new QVBoxLayout(files_section);
    files_section_layout->setContentsMargins(14, 14, 14, 14);
    files_section_layout->setSpacing(10);

    auto* files_kicker = new QLabel(QStringLiteral("SHARED FILES"), files_section);
    files_kicker->setObjectName(QStringLiteral("sectionKicker"));
    files_section_layout->addWidget(files_kicker);

    shared_empty_label_ = new QLabel(QStringLiteral("选择联系人后可查看共享文件"), files_section);
    shared_empty_label_->setObjectName(QStringLiteral("detailMeta"));
    shared_empty_label_->setWordWrap(true);
    files_section_layout->addWidget(shared_empty_label_);

    shared_files_layout_ = new QVBoxLayout();
    shared_files_layout_->setContentsMargins(0, 0, 0, 0);
    shared_files_layout_->setSpacing(10);
    files_section_layout->addLayout(shared_files_layout_);
    detail_body_layout->addWidget(files_section);
    detail_body_layout->addStretch();
    detail_layout->addWidget(detail_body, 1);
}

QLabel* DetailPanel::contactAvatarLabel() const { return contact_avatar_label_; }
QLabel* DetailPanel::contactNameLabel() const { return contact_name_label_; }
QLabel* DetailPanel::contactStatusLabel() const { return contact_status_label_; }
QVBoxLayout* DetailPanel::sharedFilesLayout() const { return shared_files_layout_; }
QLabel* DetailPanel::sharedEmptyLabel() const { return shared_empty_label_; }
