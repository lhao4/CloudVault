// =============================================================
// client/src/ui/main_window.cpp
// 主窗口（消息 / 文件 / 我）
// =============================================================

#include "main_window.h"

#include "ui/group_list_dialog.h"
#include "ui/online_user_dialog.h"
#include "ui/share_file_dialog.h"

#include <algorithm>

#include <QAbstractItemView>
#include <QApplication>
#include <QCloseEvent>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr int kMessageKindRole = Qt::UserRole + 40;
constexpr int kMessageContentRole = Qt::UserRole + 41;
constexpr int kMessageTimestampRole = Qt::UserRole + 42;
constexpr int kMessageOutgoingRole = Qt::UserRole + 43;
constexpr int kMessageAvatarSeedRole = Qt::UserRole + 44;
constexpr int kMessageDateRole = Qt::UserRole + 45;

QString formatPresence(bool online) {
    return online ? QStringLiteral("在线") : QStringLiteral("离线");
}

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

void applyShadow(QWidget* widget, int blur = 40, int offset_y = 8, int alpha = 30) {
    auto* effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(blur);
    effect->setOffset(0, offset_y);
    effect->setColor(QColor(17, 24, 39, alpha));
    widget->setGraphicsEffect(effect);
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

QPushButton* createNavButton(const QString& text, QWidget* parent = nullptr) {
    auto* button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("navBtn"));
    button->setCursor(Qt::PointingHandCursor);
    button->setProperty("active", false);
    return button;
}

QString formatConversationPreview(const QString& content, bool outgoing) {
    QString simplified = content.simplified();
    if (simplified.isEmpty()) {
        simplified = QStringLiteral("[空消息]");
    }

    const QString prefix = outgoing ? QStringLiteral("我: ")
                                    : QStringLiteral("对方: ");
    const int max_length = 24 - prefix.size();
    if (simplified.size() > max_length) {
        simplified = simplified.left(std::max(8, max_length)) + QStringLiteral("...");
    }
    return prefix + simplified;
}

QDateTime parseConversationTimestamp(const QString& timestamp) {
    QString normalized = timestamp.trimmed();
    const int dot_index = normalized.indexOf('.');
    if (dot_index >= 0) {
        int fraction_end = dot_index + 1;
        while (fraction_end < normalized.size() && normalized.at(fraction_end).isDigit()) {
            ++fraction_end;
        }
        QString fraction = normalized.mid(dot_index + 1, fraction_end - dot_index - 1);
        fraction = fraction.left(3).leftJustified(3, QLatin1Char('0'));
        normalized = normalized.left(dot_index) + QStringLiteral(".") + fraction;
    }

    QDateTime dt = QDateTime::fromString(normalized, QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    if (!dt.isValid()) {
        dt = QDateTime::fromString(normalized.section('.', 0, 0),
                                   QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }
    return dt;
}

QString dateKeyForTimestamp(const QString& timestamp) {
    const QDateTime dt = parseConversationTimestamp(timestamp);
    if (dt.isValid()) {
        return dt.date().toString(QStringLiteral("yyyy-MM-dd"));
    }
    return timestamp.section(' ', 0, 0);
}

QString formatConversationTime(const QString& timestamp) {
    const QDateTime dt = parseConversationTimestamp(timestamp);
    if (!dt.isValid()) {
        return {};
    }

    const QDate today = QDate::currentDate();
    if (dt.date() == today) {
        return dt.time().toString(QStringLiteral("HH:mm"));
    }
    if (dt.date().year() == today.year()) {
        return dt.date().toString(QStringLiteral("MM-dd"));
    }
    return dt.date().toString(QStringLiteral("yyyy-MM-dd"));
}

QString formatDividerDate(const QString& key) {
    const QDate date = QDate::fromString(key, QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid()) {
        return key;
    }
    return date.toString(QStringLiteral("yyyy-MM-dd"));
}

QString formatFileSize(quint64 size) {
    static const char* units[] = {"B", "KB", "MB", "GB"};
    double value = static_cast<double>(size);
    int unit_index = 0;
    while (value >= 1024.0 && unit_index < 3) {
        value /= 1024.0;
        ++unit_index;
    }
    if (unit_index == 0) {
        return QStringLiteral("%1 %2").arg(static_cast<qulonglong>(size)).arg(units[unit_index]);
    }
    return QStringLiteral("%1 %2")
        .arg(QString::number(value, 'f', value >= 10.0 ? 1 : 2))
        .arg(units[unit_index]);
}

QPainterPath bubblePath(const QRectF& rect, bool outgoing) {
    const qreal radius = 16.0;
    const qreal tail = 4.0;

    QPainterPath path;
    path.moveTo(rect.left() + radius, rect.top());
    path.lineTo(rect.right() - radius, rect.top());
    path.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + radius);

    if (outgoing) {
        path.lineTo(rect.right(), rect.bottom() - tail);
        path.quadTo(rect.right(), rect.bottom(), rect.right() - tail, rect.bottom());
        path.lineTo(rect.left() + radius, rect.bottom());
        path.quadTo(rect.left(), rect.bottom(), rect.left(), rect.bottom() - radius);
    } else {
        path.lineTo(rect.right(), rect.bottom() - radius);
        path.quadTo(rect.right(), rect.bottom(), rect.right() - radius, rect.bottom());
        path.lineTo(rect.left() + tail, rect.bottom());
        path.quadTo(rect.left(), rect.bottom(), rect.left(), rect.bottom() - tail);
    }

    path.lineTo(rect.left(), rect.top() + radius);
    path.quadTo(rect.left(), rect.top(), rect.left() + radius, rect.top());
    path.closeSubpath();
    return path;
}

class MessageBubbleDelegate final : public QStyledItemDelegate {
public:
    explicit MessageBubbleDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override {
        const QString kind = index.data(kMessageKindRole).toString();
        if (kind == QStringLiteral("divider")) {
            return QSize(option.rect.width(), 28);
        }

        QFont bubble_font = option.font;
        bubble_font.setPixelSize(13);
        QFontMetrics bubble_metrics(bubble_font);
        const QString text = index.data(kMessageContentRole).toString();
        const QRect text_rect = bubble_metrics.boundingRect(
            QRect(0, 0, 328, 10000),
            Qt::TextWordWrap | Qt::AlignLeft,
            text);
        const int bubble_height = text_rect.height() + 22;
        const int content_height = std::max(28, bubble_height);
        return QSize(option.rect.width(), content_height + 14);
    }

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);

        const QRect rect = option.rect.adjusted(0, 2, 0, -2);
        const QString kind = index.data(kMessageKindRole).toString();
        if (kind == QStringLiteral("divider")) {
            drawDivider(painter, rect, index.data(kMessageDateRole).toString(), option);
            painter->restore();
            return;
        }

        const bool outgoing = index.data(kMessageOutgoingRole).toBool();
        const QString text = index.data(kMessageContentRole).toString();
        const QString time = formatConversationTime(index.data(kMessageTimestampRole).toString());
        const QString seed = index.data(kMessageAvatarSeedRole).toString();

        QFont bubble_font = option.font;
        bubble_font.setPixelSize(13);
        bubble_font.setWeight(QFont::Medium);
        QFontMetrics bubble_metrics(bubble_font);

        QFont time_font = option.font;
        time_font.setPixelSize(10);
        QFontMetrics time_metrics(time_font);

        const QRect text_rect = bubble_metrics.boundingRect(
            QRect(0, 0, 328, 10000),
            Qt::TextWordWrap | Qt::AlignLeft,
            text);

        const int bubble_width = std::min(360, text_rect.width() + 28);
        const int bubble_height = text_rect.height() + 22;
        const int avatar_size = 28;
        const int side_margin = 14;
        const int avatar_gap = 8;
        const int time_gap = 10;
        const int time_width = time_metrics.horizontalAdvance(time);
        const int time_height = time_metrics.height();
        const int top = rect.top() + 6;

        QRect bubble_rect;
        QRect avatar_rect;
        QRect time_rect;

        if (outgoing) {
            const int bubble_right = rect.right() - side_margin;
            bubble_rect = QRect(bubble_right - bubble_width + 1, top, bubble_width, bubble_height);
            time_rect = QRect(bubble_rect.left() - time_gap - time_width,
                              bubble_rect.bottom() - time_height,
                              time_width,
                              time_height);
        } else {
            avatar_rect = QRect(rect.left() + side_margin,
                                top + std::max(0, bubble_height - avatar_size),
                                avatar_size,
                                avatar_size);
            bubble_rect = QRect(avatar_rect.right() + avatar_gap,
                                top,
                                bubble_width,
                                bubble_height);
            time_rect = QRect(bubble_rect.right() + time_gap,
                              bubble_rect.bottom() - time_height,
                              time_width,
                              time_height);
        }

        if (!outgoing) {
            drawAvatar(painter, avatar_rect, seed);
        }

        const QColor bubble_color = outgoing ? QColor(QStringLiteral("#3B82F6"))
                                             : QColor(QStringLiteral("#FFFFFF"));
        const QColor border_color = outgoing ? QColor(QStringLiteral("#3B82F6"))
                                             : QColor(QStringLiteral("#E2E6EA"));
        const QColor text_color = outgoing ? QColor(Qt::white)
                                           : QColor(QStringLiteral("#111827"));

        painter->setPen(QPen(border_color, outgoing ? 0.0 : 1.0));
        painter->setBrush(bubble_color);
        painter->drawPath(bubblePath(QRectF(bubble_rect), outgoing));

        painter->setFont(bubble_font);
        painter->setPen(text_color);
        painter->drawText(bubble_rect.adjusted(14, 11, -14, -11),
                          Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignVCenter,
                          text);

        painter->setFont(time_font);
        painter->setPen(QColor(QStringLiteral("#9CA3AF")));
        painter->drawText(time_rect, Qt::AlignVCenter | Qt::AlignLeft, time);

        painter->restore();
    }

private:
    static void drawDivider(QPainter* painter,
                            const QRect& rect,
                            const QString& label,
                            const QStyleOptionViewItem& option) {
        QFont font = option.font;
        font.setPixelSize(12);
        painter->setFont(font);
        painter->setPen(QColor(QStringLiteral("#9CA3AF")));

        const QString text = formatDividerDate(label);
        const QFontMetrics metrics(font);
        const int text_width = metrics.horizontalAdvance(text) + 20;
        const int center_y = rect.center().y();
        const int text_left = rect.center().x() - text_width / 2;
        const int text_right = rect.center().x() + text_width / 2;

        painter->setPen(QPen(QColor(QStringLiteral("#D1D5DB")), 1.0));
        painter->drawLine(rect.left() + 20, center_y, text_left, center_y);
        painter->drawLine(text_right, center_y, rect.right() - 20, center_y);

        painter->setPen(QColor(QStringLiteral("#9CA3AF")));
        painter->drawText(QRect(text_left, rect.top(), text_width, rect.height()),
                          Qt::AlignCenter,
                          text);
    }

    static void drawAvatar(QPainter* painter, const QRect& rect, const QString& seed) {
        static const QColor backgrounds[] = {
            QColor(QStringLiteral("#3B82F6")),
            QColor(QStringLiteral("#2563EB")),
            QColor(QStringLiteral("#22C55E")),
            QColor(QStringLiteral("#6B7280")),
            QColor(QStringLiteral("#9CA3AF")),
            QColor(QStringLiteral("#111827")),
        };

        const QColor background = backgrounds[avatarVariantForSeed(seed)];
        painter->setPen(Qt::NoPen);
        painter->setBrush(background);
        painter->drawEllipse(rect);

        QFont font = painter->font();
        font.setPixelSize(12);
        font.setBold(true);
        painter->setFont(font);
        painter->setPen(Qt::white);
        painter->drawText(rect, Qt::AlignCenter, seed.left(1).toUpper());
    }
};

class SendTextEdit final : public QTextEdit {
    Q_OBJECT

public:
    using QTextEdit::QTextEdit;

signals:
    void submitRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override {
        if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            && !(event->modifiers() & Qt::ShiftModifier)) {
            emit submitRequested();
            event->accept();
            return;
        }
        QTextEdit::keyPressEvent(event);
    }
};

QWidget* createContactItemWidget(const QString& username,
                                 const QString& preview,
                                 const QString& time,
                                 int unread,
                                 bool online,
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

    body_layout->addWidget(createAvatarWidget(username, 40, online, body), 0, Qt::AlignVCenter);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 10, 0, 10);
    text_layout->setSpacing(2);

    auto* name_label = new QLabel(username, body);
    name_label->setObjectName(QStringLiteral("contactNameLabel"));
    text_layout->addWidget(name_label);

    auto* preview_label = new QLabel(preview, body);
    preview_label->setObjectName(QStringLiteral("contactPreviewLabel"));
    preview_label->setWordWrap(false);
    text_layout->addWidget(preview_label);
    body_layout->addLayout(text_layout, 1);

    auto* meta_layout = new QVBoxLayout();
    meta_layout->setContentsMargins(0, 10, 0, 10);
    meta_layout->setSpacing(6);

    auto* time_label = new QLabel(time, body);
    time_label->setObjectName(QStringLiteral("contactTimeLabel"));
    time_label->setAlignment(Qt::AlignRight);
    meta_layout->addWidget(time_label);

    if (unread > 0) {
        auto* badge = new QLabel(QString::number(unread), body);
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
    body->setMinimumHeight(44);
    auto* row = new QHBoxLayout(body);
    row->setContentsMargins(12, 0, 12, 0);
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

QWidget* createDetailFileRow(const QString& title,
                             const QString& meta,
                             QWidget* parent = nullptr) {
    auto* row = new QFrame(parent);
    row->setObjectName(QStringLiteral("detailFileRow"));
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* icon_wrap = createStandardIconLabel(QStyle::SP_FileIcon,
                                              18,
                                              QStringLiteral("detailFileIcon"),
                                              row);
    icon_wrap->setFixedSize(28, 28);
    layout->addWidget(icon_wrap, 0, Qt::AlignTop);

    auto* text_layout = new QVBoxLayout();
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(1);

    auto* name_label = new QLabel(title, row);
    name_label->setObjectName(QStringLiteral("detailFileName"));
    text_layout->addWidget(name_label);

    auto* meta_label = new QLabel(meta, row);
    meta_label->setObjectName(QStringLiteral("detailFileMeta"));
    meta_label->setWordWrap(true);
    text_layout->addWidget(meta_label);

    layout->addLayout(text_layout, 1);
    return row;
}

} // namespace

MainWindow::MainWindow(const QString& username,
                       cloudvault::ChatService& chat_service,
                       cloudvault::FriendService& friend_service,
                       cloudvault::FileService& file_service,
                       cloudvault::ShareService& share_service,
                       QWidget* parent)
    : QMainWindow(parent),
      current_username_(username),
      chat_service_(chat_service),
      friend_service_(friend_service),
      file_service_(file_service),
      share_service_(share_service) {
    setupUi();
    connectSignals();
    loadProfileDraft();
    friend_service_.flushFriends();
    file_service_.listFiles(current_file_path_);
    switchMainTab(0);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    emit windowClosed();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("CloudVault - %1").arg(current_username_));
    setWindowIcon(QApplication::windowIcon());
    resize(900, 580);
    setMinimumSize(900, 580);

    auto* shell = new QWidget(this);
    shell->setObjectName(QStringLiteral("mainShell"));
    auto* shell_layout = new QVBoxLayout(shell);
    shell_layout->setContentsMargins(0, 0, 0, 0);
    shell_layout->setSpacing(0);

    content_root_ = new QWidget(shell);
    auto* content_root_layout = new QVBoxLayout(content_root_);
    content_root_layout->setContentsMargins(0, 0, 0, 0);
    content_root_layout->setSpacing(0);

    connection_banner_ = new QFrame(content_root_);
    connection_banner_->setObjectName(QStringLiteral("connectionBanner"));
    connection_banner_->hide();
    auto* connection_banner_layout = new QHBoxLayout(connection_banner_);
    connection_banner_layout->setContentsMargins(16, 0, 16, 0);
    connection_banner_layout->setSpacing(8);
    connection_banner_label_ = new QLabel(QStringLiteral("已断开连接，正在重连…"), connection_banner_);
    connection_banner_label_->setObjectName(QStringLiteral("connectionBannerLabel"));
    connection_banner_layout->addWidget(connection_banner_label_);
    connection_banner_layout->addStretch();
    content_root_layout->addWidget(connection_banner_);

    content_splitter_ = new QSplitter(Qt::Horizontal, content_root_);
    content_splitter_->setChildrenCollapsible(false);
    content_splitter_->setHandleWidth(1);
    content_root_layout->addWidget(content_splitter_, 1);

    sidebar_panel_ = new QFrame(content_splitter_);
    sidebar_panel_->setObjectName(QStringLiteral("sidebarPanel"));
    sidebar_panel_->setMinimumWidth(260);
    sidebar_panel_->setMaximumWidth(260);
    auto* sidebar_layout = new QVBoxLayout(sidebar_panel_);
    sidebar_layout->setContentsMargins(0, 0, 0, 0);
    sidebar_layout->setSpacing(0);

    auto* sidebar_header = new QFrame(sidebar_panel_);
    sidebar_header->setObjectName(QStringLiteral("sidebarHeader"));
    sidebar_header->setFixedHeight(56);
    auto* sidebar_header_layout = new QHBoxLayout(sidebar_header);
    sidebar_header_layout->setContentsMargins(16, 0, 12, 0);
    sidebar_header_layout->setSpacing(8);

    sidebar_title_label_ = new QLabel(QStringLiteral("消息"), sidebar_header);
    sidebar_title_label_->setObjectName(QStringLiteral("sidebarTitle"));
    sidebar_header_layout->addWidget(sidebar_title_label_);
    sidebar_header_layout->addStretch();
    sidebar_action_btn_ = createIconButton(QStringLiteral("群"),
                                           QStringLiteral("查看在线用户"),
                                           30,
                                           sidebar_header);
    sidebar_header_layout->addWidget(sidebar_action_btn_);
    sidebar_layout->addWidget(sidebar_header);

    auto* sidebar_search_row = new QFrame(sidebar_panel_);
    sidebar_search_row->setObjectName(QStringLiteral("sidebarSearchRow"));
    sidebar_search_row->setFixedHeight(48);
    auto* sidebar_search_layout = new QHBoxLayout(sidebar_search_row);
    sidebar_search_layout->setContentsMargins(12, 8, 12, 8);
    sidebar_search_layout->setSpacing(0);

    contact_search_edit_ = new QLineEdit(sidebar_search_row);
    contact_search_edit_->setObjectName(QStringLiteral("sidebarSearchEdit"));
    contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
    sidebar_search_layout->addWidget(contact_search_edit_);
    sidebar_layout->addWidget(sidebar_search_row);

    contact_list_ = new QListWidget(sidebar_panel_);
    contact_list_->setObjectName(QStringLiteral("contactList"));
    contact_list_->setSpacing(0);
    contact_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    sidebar_layout->addWidget(contact_list_, 1);

    auto* center_panel = new QWidget(content_splitter_);
    center_panel->setObjectName(QStringLiteral("centerPanel"));
    auto* center_layout = new QVBoxLayout(center_panel);
    center_layout->setContentsMargins(0, 0, 0, 0);
    center_layout->setSpacing(0);

    center_stack_ = new QStackedWidget(center_panel);
    center_layout->addWidget(center_stack_, 1);

    {
        auto* page = new QWidget(center_stack_);
        page->setObjectName(QStringLiteral("chatPage"));
        auto* page_layout = new QVBoxLayout(page);
        page_layout->setContentsMargins(0, 0, 0, 0);
        page_layout->setSpacing(0);

        chat_stack_ = new QStackedWidget(page);
        page_layout->addWidget(chat_stack_);

        auto* empty_page = new QWidget(chat_stack_);
        auto* empty_layout = new QVBoxLayout(empty_page);
        empty_layout->setContentsMargins(0, 0, 0, 0);
        empty_layout->setSpacing(8);
        empty_layout->addStretch();

        auto* empty_icon = new QLabel(QStringLiteral("💬"), empty_page);
        empty_icon->setObjectName(QStringLiteral("chatEmptyIcon"));
        empty_icon->setAlignment(Qt::AlignCenter);
        empty_layout->addWidget(empty_icon, 0, Qt::AlignHCenter);

        auto* empty_title = new QLabel(QStringLiteral("选择一位联系人开始聊天"), empty_page);
        empty_title->setObjectName(QStringLiteral("chatEmptyTitle"));
        empty_title->setAlignment(Qt::AlignCenter);
        empty_layout->addWidget(empty_title);

        auto* empty_hint = new QLabel(QStringLiteral("从左侧联系人列表中选择一个会话。"), empty_page);
        empty_hint->setObjectName(QStringLiteral("chatEmptyHint"));
        empty_hint->setAlignment(Qt::AlignCenter);
        empty_layout->addWidget(empty_hint);
        empty_layout->addStretch();
        chat_stack_->addWidget(empty_page);

        auto* conversation_page = new QWidget(chat_stack_);
        auto* conversation_layout = new QVBoxLayout(conversation_page);
        conversation_layout->setContentsMargins(0, 0, 0, 0);
        conversation_layout->setSpacing(0);

        auto* chat_top_bar = new QFrame(conversation_page);
        chat_top_bar->setObjectName(QStringLiteral("chatTopBar"));
        chat_top_bar->setFixedHeight(56);
        auto* chat_top_layout = new QHBoxLayout(chat_top_bar);
        chat_top_layout->setContentsMargins(16, 0, 16, 0);
        chat_top_layout->setSpacing(10);

        chat_top_layout->addWidget(createAvatarWidget(current_username_, 34, false, chat_top_bar));
        chat_avatar_label_ = qobject_cast<QLabel*>(
            chat_top_bar->findChild<QLabel*>(QStringLiteral("avatarBadge")));
        if (chat_avatar_label_) {
            prepareAvatarBadge(chat_avatar_label_, current_username_, 34);
        }

        auto* chat_text_layout = new QVBoxLayout();
        chat_text_layout->setContentsMargins(0, 7, 0, 7);
        chat_text_layout->setSpacing(1);

        chat_title_label_ = new QLabel(QStringLiteral("联系人"), chat_top_bar);
        chat_title_label_->setObjectName(QStringLiteral("chatTitle"));
        chat_text_layout->addWidget(chat_title_label_);

        chat_status_label_ = new QLabel(QStringLiteral("请选择联系人开始聊天"), chat_top_bar);
        chat_status_label_->setObjectName(QStringLiteral("chatStatus"));
        chat_text_layout->addWidget(chat_status_label_);
        chat_top_layout->addLayout(chat_text_layout, 1);

        group_list_btn_ = createIconButton(QStringLiteral("组"),
                                           QStringLiteral("群组列表"),
                                           30,
                                           chat_top_bar);
        chat_top_layout->addWidget(group_list_btn_);
        chat_top_layout->addWidget(createIconButton(QStringLiteral("附"),
                                                    QStringLiteral("附件"),
                                                    30,
                                                    chat_top_bar));
        chat_top_layout->addWidget(createIconButton(QStringLiteral("…"),
                                                    QStringLiteral("更多"),
                                                    30,
                                                    chat_top_bar));
        conversation_layout->addWidget(chat_top_bar);

        message_list_ = new QListWidget(conversation_page);
        message_list_->setObjectName(QStringLiteral("messageList"));
        message_list_->setSelectionMode(QAbstractItemView::NoSelection);
        message_list_->setFocusPolicy(Qt::NoFocus);
        message_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        message_list_->setItemDelegate(new MessageBubbleDelegate(message_list_));
        conversation_layout->addWidget(message_list_, 1);

        auto* compose_bar = new QFrame(conversation_page);
        compose_bar->setObjectName(QStringLiteral("composeBar"));
        auto* compose_layout = new QHBoxLayout(compose_bar);
        compose_layout->setContentsMargins(12, 10, 12, 10);
        compose_layout->setSpacing(8);
        compose_layout->addWidget(createIconButton(QStringLiteral("表"),
                                                   QStringLiteral("表情"),
                                                   34,
                                                   compose_bar));
        compose_layout->addWidget(createIconButton(QStringLiteral("附"),
                                                   QStringLiteral("发送文件"),
                                                   34,
                                                   compose_bar));

        message_input_ = new SendTextEdit(compose_bar);
        message_input_->setObjectName(QStringLiteral("messageInput"));
        message_input_->setPlaceholderText(QStringLiteral("输入消息…"));
        compose_layout->addWidget(message_input_, 1);

        send_btn_ = new QPushButton(QStringLiteral("➤"), compose_bar);
        send_btn_->setObjectName(QStringLiteral("primaryBtn"));
        send_btn_->setCursor(Qt::PointingHandCursor);
        send_btn_->setFixedSize(44, 44);
        compose_layout->addWidget(send_btn_);
        conversation_layout->addWidget(compose_bar);

        chat_stack_->addWidget(conversation_page);

        auto* group_page = new QWidget(chat_stack_);
        auto* group_layout = new QVBoxLayout(group_page);
        group_layout->setContentsMargins(0, 0, 0, 0);
        group_layout->setSpacing(0);

        auto* group_top_bar = new QFrame(group_page);
        group_top_bar->setObjectName(QStringLiteral("chatTopBar"));
        group_top_bar->setFixedHeight(56);
        auto* group_top_layout = new QHBoxLayout(group_top_bar);
        group_top_layout->setContentsMargins(16, 0, 16, 0);
        group_top_layout->setSpacing(10);

        group_top_layout->addWidget(createAvatarWidget(QStringLiteral("群"), 34, false, group_top_bar));

        auto* group_text_layout = new QVBoxLayout();
        group_text_layout->setContentsMargins(0, 7, 0, 7);
        group_text_layout->setSpacing(1);

        group_chat_title_label_ = new QLabel(QStringLiteral("群聊"), group_top_bar);
        group_chat_title_label_->setObjectName(QStringLiteral("chatTitle"));
        group_text_layout->addWidget(group_chat_title_label_);

        group_chat_status_label_ = new QLabel(QStringLiteral("当前版本尚未接入群聊后端"), group_top_bar);
        group_chat_status_label_->setObjectName(QStringLiteral("chatStatus"));
        group_text_layout->addWidget(group_chat_status_label_);
        group_top_layout->addLayout(group_text_layout, 1);
        group_layout->addWidget(group_top_bar);

        auto* group_empty = new QWidget(group_page);
        auto* group_empty_layout = new QVBoxLayout(group_empty);
        group_empty_layout->setContentsMargins(0, 0, 0, 0);
        group_empty_layout->setSpacing(8);
        group_empty_layout->addStretch();

        auto* group_icon = new QLabel(QStringLiteral("👥"), group_empty);
        group_icon->setObjectName(QStringLiteral("chatEmptyIcon"));
        group_icon->setAlignment(Qt::AlignCenter);
        group_empty_layout->addWidget(group_icon, 0, Qt::AlignHCenter);

        auto* group_title = new QLabel(QStringLiteral("群聊界面已就绪"), group_empty);
        group_title->setObjectName(QStringLiteral("chatEmptyTitle"));
        group_title->setAlignment(Qt::AlignCenter);
        group_empty_layout->addWidget(group_title);

        auto* group_hint = new QLabel(QStringLiteral("当前版本仅提供群组 UI 流程，消息后端待接入。"), group_empty);
        group_hint->setObjectName(QStringLiteral("chatEmptyHint"));
        group_hint->setAlignment(Qt::AlignCenter);
        group_empty_layout->addWidget(group_hint);
        group_empty_layout->addStretch();
        group_layout->addWidget(group_empty, 1);

        chat_stack_->addWidget(group_page);
        center_stack_->addWidget(page);
    }

    {
        auto* page = new QWidget(center_stack_);
        page->setObjectName(QStringLiteral("filePage"));
        auto* page_layout = new QVBoxLayout(page);
        page_layout->setContentsMargins(0, 0, 0, 0);
        page_layout->setSpacing(0);

        auto* top_bar = new QFrame(page);
        top_bar->setObjectName(QStringLiteral("fileTopBar"));
        top_bar->setFixedHeight(56);
        auto* top_layout = new QHBoxLayout(top_bar);
        top_layout->setContentsMargins(20, 0, 20, 0);
        top_layout->setSpacing(8);

        file_path_label_ = new QLabel(QStringLiteral("/"), top_bar);
        file_path_label_->setObjectName(QStringLiteral("filePathLabel"));
        file_path_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        top_layout->addWidget(file_path_label_, 1);

        file_back_btn_ = new QPushButton(QStringLiteral("返回"), top_bar);
        file_back_btn_->setObjectName(QStringLiteral("fileActionBtn"));
        top_layout->addWidget(file_back_btn_);

        file_upload_btn_ = new QPushButton(QStringLiteral("上传"), top_bar);
        file_upload_btn_->setObjectName(QStringLiteral("fileActionBtn"));
        top_layout->addWidget(file_upload_btn_);

        file_refresh_btn_ = new QPushButton(QStringLiteral("刷新"), top_bar);
        file_refresh_btn_->setObjectName(QStringLiteral("fileActionBtn"));
        top_layout->addWidget(file_refresh_btn_);

        file_create_btn_ = new QPushButton(QStringLiteral("+ 新建"), top_bar);
        file_create_btn_->setObjectName(QStringLiteral("primaryFileBtn"));
        top_layout->addWidget(file_create_btn_);
        page_layout->addWidget(top_bar);

        auto* search_row = new QFrame(page);
        search_row->setObjectName(QStringLiteral("fileSearchRow"));
        auto* search_layout = new QHBoxLayout(search_row);
        search_layout->setContentsMargins(16, 10, 16, 10);
        search_layout->setSpacing(0);

        file_search_edit_ = new QLineEdit(search_row);
        file_search_edit_->setPlaceholderText(QStringLiteral("搜索文件、回车确认…"));
        search_layout->addWidget(file_search_edit_);
        page_layout->addWidget(search_row);

        auto* table_header = new QFrame(page);
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

        file_list_ = new QListWidget(page);
        file_list_->setObjectName(QStringLiteral("fileList"));
        file_list_->setSpacing(0);
        file_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        page_layout->addWidget(file_list_, 1);

        file_empty_state_label_ = new QLabel(
            QStringLiteral("当前目录为空\n点击“+ 新建”或使用“上传”开始管理文件"),
            page);
        file_empty_state_label_->setObjectName(QStringLiteral("emptyStateLabel"));
        file_empty_state_label_->setAlignment(Qt::AlignCenter);
        file_empty_state_label_->hide();
        page_layout->addWidget(file_empty_state_label_);

        file_transfer_row_ = new QFrame(page);
        file_transfer_row_->setObjectName(QStringLiteral("fileTransferRow"));
        file_transfer_row_->hide();
        auto* transfer_layout = new QHBoxLayout(file_transfer_row_);
        transfer_layout->setContentsMargins(16, 0, 16, 0);
        transfer_layout->setSpacing(10);

        file_transfer_label_ = new QLabel(QStringLiteral("文件传输"), file_transfer_row_);
        file_transfer_label_->setObjectName(QStringLiteral("transferLabel"));
        transfer_layout->addWidget(file_transfer_label_);

        file_transfer_bar_ = new QProgressBar(file_transfer_row_);
        file_transfer_bar_->setTextVisible(false);
        file_transfer_bar_->setRange(0, 100);
        transfer_layout->addWidget(file_transfer_bar_, 1);

        file_transfer_percent_label_ = new QLabel(QStringLiteral("0%"), file_transfer_row_);
        file_transfer_percent_label_->setObjectName(QStringLiteral("transferPercent"));
        transfer_layout->addWidget(file_transfer_percent_label_);

        file_transfer_cancel_btn_ = new QPushButton(QStringLiteral("取消"), file_transfer_row_);
        file_transfer_cancel_btn_->setObjectName(QStringLiteral("fileActionBtn"));
        file_transfer_cancel_btn_->setFixedHeight(28);
        transfer_layout->addWidget(file_transfer_cancel_btn_);
        page_layout->addWidget(file_transfer_row_);

        auto* footer = new QFrame(page);
        footer->setObjectName(QStringLiteral("fileActionBar"));
        footer->setFixedHeight(56);
        auto* footer_layout = new QHBoxLayout(footer);
        footer_layout->setContentsMargins(16, 0, 16, 0);
        footer_layout->setSpacing(8);

        auto* summary_layout = new QVBoxLayout();
        summary_layout->setContentsMargins(0, 8, 0, 8);
        summary_layout->setSpacing(1);
        detail_file_target_label_ = new QLabel(QStringLiteral("选中：未选择文件"), footer);
        detail_file_target_label_->setObjectName(QStringLiteral("fileSelectionLabel"));
        summary_layout->addWidget(detail_file_target_label_);

        detail_file_meta_label_ = new QLabel(QStringLiteral("未选择文件"), footer);
        detail_file_meta_label_->setObjectName(QStringLiteral("fileStatusLabel"));
        summary_layout->addWidget(detail_file_meta_label_);
        file_status_label_ = detail_file_meta_label_;
        footer_layout->addLayout(summary_layout, 1);

        file_download_btn_ = new QPushButton(QStringLiteral("下载"), footer);
        file_download_btn_->setObjectName(QStringLiteral("fileActionBtn"));
        footer_layout->addWidget(file_download_btn_);

        file_share_btn_ = new QPushButton(QStringLiteral("分享"), footer);
        file_share_btn_->setObjectName(QStringLiteral("fileActionBtn"));
        footer_layout->addWidget(file_share_btn_);

        file_rename_btn_ = new QPushButton(QStringLiteral("重命名"), footer);
        file_rename_btn_->setObjectName(QStringLiteral("fileActionBtn"));
        footer_layout->addWidget(file_rename_btn_);

        file_move_btn_ = new QPushButton(QStringLiteral("移动"), footer);
        file_move_btn_->setObjectName(QStringLiteral("fileActionBtn"));
        footer_layout->addWidget(file_move_btn_);

        file_delete_btn_ = new QPushButton(QStringLiteral("删除"), footer);
        file_delete_btn_->setObjectName(QStringLiteral("deleteBtn"));
        footer_layout->addWidget(file_delete_btn_);
        page_layout->addWidget(footer);

        center_stack_->addWidget(page);
    }

    {
        auto* page = new QWidget(center_stack_);
        page->setObjectName(QStringLiteral("profilePage"));
        auto* page_layout = new QVBoxLayout(page);
        page_layout->setContentsMargins(0, 0, 0, 0);
        page_layout->setSpacing(0);
        page_layout->addStretch();

        auto* card_row = new QHBoxLayout();
        card_row->setContentsMargins(0, 0, 0, 0);
        card_row->setSpacing(0);
        card_row->addStretch();

        auto* card = new QFrame(page);
        card->setObjectName(QStringLiteral("profileCard"));
        card->setFixedWidth(360);
        applyShadow(card, 32, 6, 20);
        auto* card_layout = new QVBoxLayout(card);
        card_layout->setContentsMargins(32, 32, 32, 32);
        card_layout->setSpacing(12);

        auto* profile_avatar = new QLabel(card);
        prepareAvatarBadge(profile_avatar, current_username_, 64);
        card_layout->addWidget(profile_avatar, 0, Qt::AlignHCenter);

        profile_name_label_ = new QLabel(current_username_, card);
        profile_name_label_->setObjectName(QStringLiteral("profileName"));
        profile_name_label_->setAlignment(Qt::AlignCenter);
        card_layout->addWidget(profile_name_label_);

        profile_id_label_ = new QLabel(QStringLiteral("@%1").arg(current_username_), card);
        profile_id_label_->setObjectName(QStringLiteral("profileId"));
        profile_id_label_->setAlignment(Qt::AlignCenter);
        card_layout->addWidget(profile_id_label_);

        auto* divider_top = new QFrame(card);
        divider_top->setFrameShape(QFrame::HLine);
        card_layout->addWidget(divider_top);

        auto* nickname_label = new QLabel(QStringLiteral("昵称"), card);
        nickname_label->setObjectName(QStringLiteral("fieldLabel"));
        card_layout->addWidget(nickname_label);

        profile_nickname_edit_ = new QLineEdit(card);
        profile_nickname_edit_->setPlaceholderText(QStringLiteral("请输入昵称"));
        card_layout->addWidget(profile_nickname_edit_);

        auto* signature_label = new QLabel(QStringLiteral("个性签名"), card);
        signature_label->setObjectName(QStringLiteral("fieldLabel"));
        card_layout->addWidget(signature_label);

        profile_signature_edit_ = new QLineEdit(card);
        profile_signature_edit_->setPlaceholderText(QStringLiteral("写点什么介绍自己"));
        card_layout->addWidget(profile_signature_edit_);

        auto* action_row = new QHBoxLayout();
        action_row->setSpacing(10);
        auto* save_btn = new QPushButton(QStringLiteral("保存"), card);
        save_btn->setObjectName(QStringLiteral("primaryBtn"));
        auto* cancel_btn = new QPushButton(QStringLiteral("取消"), card);
        cancel_btn->setObjectName(QStringLiteral("secondaryBtn"));
        action_row->addWidget(save_btn);
        action_row->addWidget(cancel_btn);
        card_layout->addLayout(action_row);

        auto* divider_bottom = new QFrame(card);
        divider_bottom->setFrameShape(QFrame::HLine);
        card_layout->addWidget(divider_bottom);

        logout_btn_ = new QPushButton(QStringLiteral("退出登录"), card);
        logout_btn_->setObjectName(QStringLiteral("logoutBtn"));
        card_layout->addWidget(logout_btn_);

        connect(save_btn, &QPushButton::clicked, this, &MainWindow::saveProfileDraft);
        connect(cancel_btn, &QPushButton::clicked, this, &MainWindow::loadProfileDraft);

        card_row->addWidget(card);
        card_row->addStretch();
        page_layout->addLayout(card_row);
        page_layout->addStretch();

        center_stack_->addWidget(page);
    }

    detail_panel_ = new QFrame(content_splitter_);
    detail_panel_->setObjectName(QStringLiteral("detailPanel"));
    qobject_cast<QFrame*>(detail_panel_)->setMinimumWidth(220);
    qobject_cast<QFrame*>(detail_panel_)->setMaximumWidth(220);
    auto* detail_layout = new QVBoxLayout(detail_panel_);
    detail_layout->setContentsMargins(0, 0, 0, 0);
    detail_layout->setSpacing(0);

    auto* detail_header = new QFrame(detail_panel_);
    detail_header->setObjectName(QStringLiteral("detailHeader"));
    detail_header->setFixedHeight(56);
    auto* detail_header_layout = new QHBoxLayout(detail_header);
    detail_header_layout->setContentsMargins(16, 0, 16, 0);
    auto* detail_title = new QLabel(QStringLiteral("详情"), detail_header);
    detail_title->setObjectName(QStringLiteral("detailTitle"));
    detail_header_layout->addWidget(detail_title);
    detail_header_layout->addStretch();
    detail_layout->addWidget(detail_header);

    auto* detail_body = new QWidget(detail_panel_);
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
    detail_contact_avatar_label_ = new QLabel(contact_section);
    prepareAvatarBadge(detail_contact_avatar_label_, current_username_, 26);
    contact_info_row->addWidget(detail_contact_avatar_label_, 0, Qt::AlignTop);

    auto* contact_text_layout = new QVBoxLayout();
    contact_text_layout->setContentsMargins(0, 0, 0, 0);
    contact_text_layout->setSpacing(1);
    detail_contact_name_label_ = new QLabel(QStringLiteral("未选择联系人"), contact_section);
    detail_contact_name_label_->setObjectName(QStringLiteral("detailContactName"));
    contact_text_layout->addWidget(detail_contact_name_label_);

    detail_contact_status_label_ = new QLabel(QStringLiteral("请选择左侧联系人"), contact_section);
    detail_contact_status_label_->setObjectName(QStringLiteral("detailMeta"));
    contact_text_layout->addWidget(detail_contact_status_label_);
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

    detail_shared_empty_label_ = new QLabel(QStringLiteral("选择联系人后可查看共享文件"), files_section);
    detail_shared_empty_label_->setObjectName(QStringLiteral("detailMeta"));
    detail_shared_empty_label_->setWordWrap(true);
    files_section_layout->addWidget(detail_shared_empty_label_);

    detail_shared_files_layout_ = new QVBoxLayout();
    detail_shared_files_layout_->setContentsMargins(0, 0, 0, 0);
    detail_shared_files_layout_->setSpacing(10);
    files_section_layout->addLayout(detail_shared_files_layout_);
    detail_body_layout->addWidget(files_section);
    detail_body_layout->addStretch();
    detail_layout->addWidget(detail_body, 1);

    content_splitter_->addWidget(sidebar_panel_);
    content_splitter_->addWidget(center_panel);
    content_splitter_->addWidget(detail_panel_);
    content_splitter_->setStretchFactor(1, 1);
    content_splitter_->setSizes({260, 420, 220});

    shell_layout->addWidget(content_root_, 1);

    event_log_panel_ = new QFrame(shell);
    event_log_panel_->setObjectName(QStringLiteral("eventLogPanel"));
    auto* event_log_layout = new QVBoxLayout(event_log_panel_);
    event_log_layout->setContentsMargins(0, 0, 0, 0);
    event_log_layout->setSpacing(0);

    auto* event_log_header = new QFrame(event_log_panel_);
    event_log_header->setObjectName(QStringLiteral("eventLogHeader"));
    event_log_header->setFixedHeight(36);
    auto* event_log_header_layout = new QHBoxLayout(event_log_header);
    event_log_header_layout->setContentsMargins(16, 0, 12, 0);
    event_log_header_layout->setSpacing(8);

    auto* event_log_title = new QLabel(QStringLiteral("● 事件日志"), event_log_header);
    event_log_title->setObjectName(QStringLiteral("eventLogTitle"));
    event_log_header_layout->addWidget(event_log_title);
    event_log_header_layout->addStretch();

    event_log_toggle_btn_ = new QToolButton(event_log_header);
    event_log_toggle_btn_->setObjectName(QStringLiteral("eventLogToggleBtn"));
    event_log_toggle_btn_->setText(QStringLiteral("折叠 ▲"));
    event_log_toggle_btn_->setCursor(Qt::PointingHandCursor);
    event_log_header_layout->addWidget(event_log_toggle_btn_);
    event_log_layout->addWidget(event_log_header);

    event_log_list_ = new QListWidget(event_log_panel_);
    event_log_list_->setObjectName(QStringLiteral("eventLogList"));
    event_log_list_->setSelectionMode(QAbstractItemView::NoSelection);
    event_log_list_->setFocusPolicy(Qt::NoFocus);
    event_log_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    event_log_list_->setUniformItemSizes(true);
    event_log_list_->setFixedHeight(108);
    event_log_layout->addWidget(event_log_list_);
    shell_layout->addWidget(event_log_panel_);

    auto* nav_bar = new QFrame(shell);
    nav_bar->setObjectName(QStringLiteral("navBar"));
    nav_bar->setFixedHeight(52);
    auto* nav_layout = new QHBoxLayout(nav_bar);
    nav_layout->setContentsMargins(0, 0, 0, 0);
    nav_layout->setSpacing(0);
    message_tab_btn_ = createNavButton(QStringLiteral("消息"), nav_bar);
    file_tab_btn_ = createNavButton(QStringLiteral("文件"), nav_bar);
    profile_tab_btn_ = createNavButton(QStringLiteral("我"), nav_bar);
    nav_layout->addWidget(message_tab_btn_);
    nav_layout->addWidget(file_tab_btn_);
    nav_layout->addWidget(profile_tab_btn_);
    shell_layout->addWidget(nav_bar);

    setCentralWidget(shell);
    showChatEmptyState();
    hideTransferRow();
    resetFileActionSummary();
    appendEventLog(QStringLiteral("已打开客户端，等待连接服务器"), QStringLiteral("●"));
}

void MainWindow::connectSignals() {
    connect(message_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(0); });
    connect(file_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(1); });
    connect(profile_tab_btn_, &QPushButton::clicked, this, [this] { switchMainTab(2); });
    connect(event_log_toggle_btn_, &QToolButton::clicked, this, [this] {
        event_log_expanded_ = !event_log_expanded_;
        event_log_list_->setVisible(event_log_expanded_);
        event_log_toggle_btn_->setText(event_log_expanded_
            ? QStringLiteral("折叠 ▲")
            : QStringLiteral("展开 ▼"));
    });

    connect(send_btn_, &QPushButton::clicked, this, &MainWindow::sendCurrentMessage);
    connect(qobject_cast<SendTextEdit*>(message_input_), &SendTextEdit::submitRequested,
            this, &MainWindow::sendCurrentMessage);
    connect(group_list_btn_, &QPushButton::clicked, this, &MainWindow::openGroupListDialog);
    connect(contact_search_edit_, &QLineEdit::textChanged,
            this, &MainWindow::filterFriendList);
    connect(contact_list_, &QListWidget::itemSelectionChanged,
            this, &MainWindow::applySelectedFriend);
    connect(sidebar_action_btn_, &QPushButton::clicked,
            this, &MainWindow::openOnlineUserDialog);

    connect(logout_btn_, &QPushButton::clicked, this, &MainWindow::logoutRequested);
    connect(logout_btn_, &QPushButton::clicked, this, [this] {
        appendEventLog(QStringLiteral("用户请求退出登录"), QStringLiteral("→"));
    });

    connect(file_back_btn_, &QPushButton::clicked, this, &MainWindow::openCurrentParentDirectory);
    connect(file_upload_btn_, &QPushButton::clicked, this, &MainWindow::uploadFile);
    connect(file_refresh_btn_, &QPushButton::clicked,
            this, [this] {
                if (file_search_mode_ && !current_file_query_.isEmpty()) {
                    setFileStatus(QStringLiteral("正在刷新搜索结果…"));
                    appendEventLog(QStringLiteral("刷新当前搜索结果"), QStringLiteral("→"));
                    file_service_.search(current_file_query_);
                } else {
                    setFileStatus(QStringLiteral("正在刷新当前目录…"));
                    appendEventLog(QStringLiteral("刷新文件列表 %1").arg(current_file_path_), QStringLiteral("→"));
                    file_service_.listFiles(current_file_path_);
                }
            });
    connect(file_create_btn_, &QPushButton::clicked, this, &MainWindow::createDirectory);
    connect(file_search_edit_, &QLineEdit::returnPressed, this, &MainWindow::runFileSearch);
    connect(file_search_edit_, &QLineEdit::textChanged, this, &MainWindow::clearFileSearchIfNeeded);
    connect(file_list_, &QListWidget::itemSelectionChanged, this, &MainWindow::applySelectedFile);
    connect(file_list_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                if (!item) {
                    return;
                }
                if (!item->data(Qt::UserRole + 1).toBool()) {
                    return;
                }
                file_search_mode_ = false;
                current_file_query_.clear();
                file_search_edit_->clear();
                navigateToFilePath(item->data(Qt::UserRole).toString());
            });
    connect(file_rename_btn_, &QPushButton::clicked, this, &MainWindow::renameSelectedFile);
    connect(file_move_btn_, &QPushButton::clicked, this, &MainWindow::moveSelectedFile);
    connect(file_delete_btn_, &QPushButton::clicked, this, &MainWindow::deleteSelectedFile);
    connect(file_download_btn_, &QPushButton::clicked, this, &MainWindow::downloadSelectedFile);
    connect(file_share_btn_, &QPushButton::clicked, this, &MainWindow::openShareFileDialog);
    connect(file_transfer_cancel_btn_, &QPushButton::clicked,
            this, [this] {
                if (!pending_upload_paths_.isEmpty()) {
                    pending_upload_paths_.clear();
                    setFileStatus(QStringLiteral("已清空等待中的上传队列，当前正在进行的传输不会中断。"));
                    file_transfer_cancel_btn_->setEnabled(false);
                    return;
                }
                QMessageBox::information(this,
                                         QStringLiteral("当前不可取消"),
                                         QStringLiteral("当前版本暂不支持中途取消已开始的传输任务。"));
            });

    connect(&chat_service_, &cloudvault::ChatService::messageReceived,
            this, [this](const cloudvault::ChatMessage& message) {
                const QString peer = (message.from == current_username_) ? message.to : message.from;
                applyConversationMessage(
                    message,
                    message.from != current_username_ && peer != active_chat_peer_);
                appendEventLog(QStringLiteral("%1 %2: %3")
                                   .arg(message.from == current_username_ ? QStringLiteral("发送给")
                                                                          : QStringLiteral("收到来自"))
                                   .arg(peer)
                                   .arg(message.content.left(24)),
                               message.from == current_username_ ? QStringLiteral("→")
                                                                 : QStringLiteral("●"));
                if (peer == active_chat_peer_) {
                    appendChatMessage(message);
                    showChatConversation(peer, selectedFriendOnline());
                }
            });
    connect(&chat_service_, &cloudvault::ChatService::historyLoaded,
            this, [this](const QString& peer, const QList<cloudvault::ChatMessage>& messages) {
                pending_history_requests_.remove(peer);
                updateConversationSummary(peer, messages);
                if (peer != active_chat_peer_) {
                    return;
                }
                rebuildMessageList(messages);

                QDateTime latest_incoming;
                for (const auto& message : messages) {
                    if (message.from != peer) {
                        continue;
                    }
                    const QDateTime message_time = parseConversationTimestamp(message.timestamp);
                    if (message_time.isValid() &&
                        (!latest_incoming.isValid() || message_time > latest_incoming)) {
                        latest_incoming = message_time;
                    }
                }
                saveConversationReadAt(peer, latest_incoming);
                showChatConversation(peer, selectedFriendOnline());
            });
    connect(&chat_service_, &cloudvault::ChatService::historyLoadFailed,
            this, [this](const QString& peer, const QString& reason) {
                pending_history_requests_.remove(peer);
                if (peer != active_chat_peer_) {
                    return;
                }
                showChatConversation(peer, selectedFriendOnline());
                chat_status_label_->setText(reason);
            });

    connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
            this, &MainWindow::refreshFriendList);
    connect(&friend_service_, &cloudvault::FriendService::friendAdded,
            this, [this](const QString&) { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendDeleted,
            this, [this](const QString&) { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendRequestSent,
            this, [this](const QString& target) {
                QMessageBox::information(this,
                                         QStringLiteral("好友申请"),
                                         QStringLiteral("已向 %1 发送好友申请。").arg(target));
            });
    connect(&friend_service_, &cloudvault::FriendService::friendAddFailed,
            this, [this](const QString& reason) {
                QMessageBox::warning(this, QStringLiteral("添加失败"), reason);
            });
    connect(&friend_service_, &cloudvault::FriendService::incomingFriendRequest,
            this, [this](const QString& from) {
                const auto reply = QMessageBox::question(
                    this,
                    QStringLiteral("好友申请"),
                    QStringLiteral("%1 想添加你为好友，是否同意？").arg(from),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    friend_service_.agreeFriend(from);
                }
            });

    connect(&file_service_, &cloudvault::FileService::filesListed,
            this, &MainWindow::refreshFileList);
    connect(&file_service_, &cloudvault::FileService::fileListFailed,
            this, [this](const QString& reason) { setFileStatus(reason, true); });
    connect(&file_service_, &cloudvault::FileService::searchCompleted,
            this, [this](const QString& keyword, const cloudvault::FileEntries& entries) {
                file_search_mode_ = true;
                current_file_query_ = keyword;
                refreshFileList(QStringLiteral("搜索：%1").arg(keyword), entries);
            });
    connect(&file_service_, &cloudvault::FileService::searchFailed,
            this, [this](const QString& reason) { setFileStatus(reason, true); });
    connect(&file_service_, &cloudvault::FileService::fileOperationSucceeded,
            this, [this](const QString& message) {
                setFileStatus(message);
                appendEventLog(message, QStringLiteral("✓"));
                file_search_mode_ = false;
                current_file_query_.clear();
                file_search_edit_->clear();
                file_service_.listFiles(current_file_path_);
            });
    connect(&file_service_, &cloudvault::FileService::fileOperationFailed,
            this, [this](const QString& message) { setFileStatus(message, true); });
    connect(&file_service_, &cloudvault::FileService::uploadProgress,
            this, [this](const QString& filename, quint64 sent, quint64 total) {
                const int percent = (total == 0)
                    ? 0
                    : static_cast<int>((static_cast<double>(sent) * 100.0) / static_cast<double>(total));
                showTransferRow(QStringLiteral("上传 %1").arg(filename), percent, true);
                setFileStatus(QStringLiteral("正在上传 %1 · %2 / %3")
                                  .arg(filename, formatFileSize(sent), formatFileSize(total)));
                if (percent == 0) {
                    appendEventLog(QStringLiteral("开始上传文件 %1").arg(filename), QStringLiteral("↑"));
                }
            });
    connect(&file_service_, &cloudvault::FileService::uploadFinished,
            this, [this](const QString& remote_path, const QString& message) {
                showTransferRow(QStringLiteral("上传完成"), 100, !pending_upload_paths_.isEmpty());
                setFileStatus(QStringLiteral("%1 · %2").arg(message, remote_path));
                appendEventLog(QStringLiteral("文件上传完成 %1").arg(QFileInfo(remote_path).fileName()),
                               QStringLiteral("✓"));
                file_search_mode_ = false;
                current_file_query_.clear();
                file_search_edit_->clear();
                file_service_.listFiles(current_file_path_);
                QTimer::singleShot(0, this, &MainWindow::startNextQueuedUpload);
                QTimer::singleShot(1500, this, [this] {
                    if (!file_service_.hasActiveTransfer() && pending_upload_paths_.isEmpty()) {
                        hideTransferRow();
                    }
                });
            });
    connect(&file_service_, &cloudvault::FileService::uploadFailed,
            this, [this](const QString& message) {
                showTransferRow(QStringLiteral("上传失败"), 0, !pending_upload_paths_.isEmpty());
                setFileStatus(message, true);
                QTimer::singleShot(0, this, &MainWindow::startNextQueuedUpload);
                QTimer::singleShot(1500, this, [this] {
                    if (!file_service_.hasActiveTransfer() && pending_upload_paths_.isEmpty()) {
                        hideTransferRow();
                    }
                });
            });
    connect(&file_service_, &cloudvault::FileService::downloadProgress,
            this, [this](const QString& filename, quint64 received, quint64 total) {
                const int percent = (total == 0)
                    ? 0
                    : static_cast<int>((static_cast<double>(received) * 100.0) / static_cast<double>(total));
                showTransferRow(QStringLiteral("下载 %1").arg(filename), percent, true);
                setFileStatus(QStringLiteral("正在下载 %1 · %2 / %3")
                                  .arg(filename, formatFileSize(received), formatFileSize(total)));
            });
    connect(&file_service_, &cloudvault::FileService::downloadFinished,
            this, [this](const QString& local_path, const QString& message) {
                showTransferRow(QStringLiteral("下载完成"), 100, false);
                setFileStatus(QStringLiteral("%1 · %2").arg(message, local_path));
                appendEventLog(QStringLiteral("文件下载完成 %1").arg(QFileInfo(local_path).fileName()),
                               QStringLiteral("✓"));
                QTimer::singleShot(1500, this, &MainWindow::hideTransferRow);
                QMessageBox::information(this, QStringLiteral("下载完成"), local_path);
            });
    connect(&file_service_, &cloudvault::FileService::downloadFailed,
            this, [this](const QString& message) {
                showTransferRow(QStringLiteral("下载失败"), 0, false);
                setFileStatus(message, true);
                QTimer::singleShot(1500, this, &MainWindow::hideTransferRow);
            });

    connect(&share_service_, &cloudvault::ShareService::shareRequestSent,
            this, [this](const QString& message) { setFileStatus(message); });
    connect(&share_service_, &cloudvault::ShareService::shareRequestFailed,
            this, [this](const QString& reason) { setFileStatus(reason, true); });
    connect(&share_service_, &cloudvault::ShareService::incomingShareRequest,
            this, [this](const QString& from, const QString& file_path) {
                QMessageBox box(this);
                box.setIcon(QMessageBox::Question);
                box.setWindowTitle(QStringLiteral("收到文件分享"));
                box.setText(QStringLiteral("%1 想与你共享文件").arg(from));
                box.setInformativeText(
                    QStringLiteral("文件：%1\n来源路径：%2\n接收后会复制到你的根目录。")
                        .arg(QFileInfo(file_path).fileName(), file_path));
                auto* accept_btn = box.addButton(QStringLiteral("接收"), QMessageBox::AcceptRole);
                box.addButton(QStringLiteral("稍后处理"), QMessageBox::RejectRole);
                box.exec();
                if (box.clickedButton() == accept_btn) {
                    setFileStatus(QStringLiteral("正在接收来自 %1 的分享…").arg(from));
                    share_service_.acceptShare(from, file_path);
                }
            });
    connect(&share_service_, &cloudvault::ShareService::shareAccepted,
            this, [this](const QString& message) {
                setFileStatus(message);
                file_search_mode_ = false;
                current_file_query_.clear();
                file_search_edit_->clear();
                file_service_.listFiles(QStringLiteral("/"));
            });
    connect(&share_service_, &cloudvault::ShareService::shareAcceptFailed,
            this, [this](const QString& reason) {
                setFileStatus(reason, true);
                QMessageBox::warning(this, QStringLiteral("接收失败"), reason);
            });

    auto* refresh_shortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(refresh_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 0) {
            friend_service_.flushFriends();
            if (!active_chat_peer_.isEmpty()) {
                requestConversationSnapshot(active_chat_peer_);
            }
            appendEventLog(QStringLiteral("快捷键刷新消息面板"), QStringLiteral("→"));
        } else if (center_stack_->currentIndex() == 1) {
            if (file_search_mode_ && !current_file_query_.isEmpty()) {
                file_service_.search(current_file_query_);
            } else {
                file_service_.listFiles(current_file_path_);
            }
            appendEventLog(QStringLiteral("快捷键刷新文件面板"), QStringLiteral("→"));
        }
    });

    auto* delete_shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(delete_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1 && file_list_->currentItem()) {
            deleteSelectedFile();
        }
    });

    auto* rename_shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    connect(rename_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1 && file_list_->currentItem()) {
            renameSelectedFile();
        }
    });

    auto* back_shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
    connect(back_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1) {
            openCurrentParentDirectory();
        }
    });

    auto* focus_search_shortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+F")), this);
    connect(focus_search_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1) {
            file_search_edit_->setFocus();
            file_search_edit_->selectAll();
        }
    });

    auto* upload_shortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+U")), this);
    connect(upload_shortcut, &QShortcut::activated, this, [this] {
        if (center_stack_->currentIndex() == 1) {
            uploadFile();
        }
    });
}

void MainWindow::appendChatMessage(const cloudvault::ChatMessage& message) {
    appendDateDividerIfNeeded(message.timestamp);

    auto* item = new QListWidgetItem(message_list_);
    item->setData(kMessageKindRole, QStringLiteral("message"));
    item->setData(kMessageContentRole, message.content);
    item->setData(kMessageTimestampRole, message.timestamp);
    item->setData(kMessageOutgoingRole, message.from == current_username_);
    item->setData(kMessageAvatarSeedRole, (message.from == current_username_) ? current_username_
                                                                              : message.from);
    message_list_->scrollToBottom();
}

void MainWindow::rebuildMessageList(const QList<cloudvault::ChatMessage>& messages) {
    message_list_->clear();
    for (const auto& message : messages) {
        appendChatMessage(message);
    }
    message_list_->scrollToBottom();
}

void MainWindow::showChatEmptyState() {
    active_chat_peer_.clear();
    active_group_name_.clear();
    if (chat_stack_) {
        chat_stack_->setCurrentIndex(0);
    }
    if (chat_title_label_) {
        chat_title_label_->setText(QStringLiteral("CloudVault"));
    }
    if (chat_status_label_) {
        chat_status_label_->setText(QStringLiteral("请选择联系人开始聊天"));
    }
    if (message_list_) {
        message_list_->clear();
    }
    if (detail_contact_name_label_) {
        detail_contact_name_label_->setText(QStringLiteral("未选择联系人"));
    }
    if (detail_contact_status_label_) {
        detail_contact_status_label_->setText(QStringLiteral("请选择左侧联系人"));
    }
    if (detail_shared_empty_label_) {
        detail_shared_empty_label_->setText(QStringLiteral("选择联系人后可查看共享文件"));
    }
    if (detail_contact_avatar_label_) {
        prepareAvatarBadge(detail_contact_avatar_label_, current_username_, 26);
    }
}

void MainWindow::showChatConversation(const QString& username, bool online) {
    if (chat_stack_) {
        chat_stack_->setCurrentIndex(1);
    }
    active_group_name_.clear();
    chat_title_label_->setText(username);
    chat_status_label_->setText(formatPresence(online));
    if (chat_avatar_label_) {
        prepareAvatarBadge(chat_avatar_label_, username, 34);
    }
    detail_contact_name_label_->setText(username);
    detail_contact_status_label_->setText(formatPresence(online));
    if (detail_contact_avatar_label_) {
        prepareAvatarBadge(detail_contact_avatar_label_, username, 26);
    }

    while (detail_shared_files_layout_->count() > 0) {
        if (auto* item = detail_shared_files_layout_->takeAt(0)) {
            delete item->widget();
            delete item;
        }
    }

    const auto summary_it = conversation_summaries_.constFind(username);
    if (summary_it != conversation_summaries_.constEnd() && summary_it->has_message) {
        detail_shared_empty_label_->setVisible(false);
        detail_shared_files_layout_->addWidget(
            createDetailFileRow(QStringLiteral("最近会话"),
                                summary_it->preview,
                                detail_panel_));
    } else {
        detail_shared_empty_label_->setText(QStringLiteral("暂无与 %1 的共享文件").arg(username));
        detail_shared_empty_label_->setVisible(true);
    }
}

void MainWindow::requestConversationSnapshot(const QString& peer) {
    const QString trimmed = peer.trimmed();
    if (trimmed.isEmpty() || pending_history_requests_.contains(trimmed)) {
        return;
    }
    pending_history_requests_.insert(trimmed);
    chat_service_.loadHistory(trimmed);
}

void MainWindow::updateConversationSummary(const QString& peer,
                                           const QList<cloudvault::ChatMessage>& messages) {
    auto& summary = conversation_summaries_[peer];
    if (messages.isEmpty()) {
        summary.preview.clear();
        summary.timestamp.clear();
        summary.unread_count = 0;
        summary.has_message = false;
        summary.last_message_outgoing = false;
    } else {
        const auto& latest = messages.constLast();
        summary.last_message_outgoing = latest.from == current_username_;
        summary.preview = formatConversationPreview(latest.content, summary.last_message_outgoing);
        summary.timestamp = latest.timestamp;
        summary.has_message = true;
        summary.unread_count = (peer == active_chat_peer_) ? 0 : calculateUnreadCount(peer, messages);
    }
    filterFriendList(contact_search_edit_->text());
}

void MainWindow::applyConversationMessage(const cloudvault::ChatMessage& message, bool increment_unread) {
    const QString peer = (message.from == current_username_) ? message.to : message.from;
    if (peer.isEmpty()) {
        return;
    }

    auto& summary = conversation_summaries_[peer];
    summary.last_message_outgoing = message.from == current_username_;
    summary.preview = formatConversationPreview(message.content, summary.last_message_outgoing);
    summary.timestamp = message.timestamp;
    summary.has_message = true;
    if (peer == active_chat_peer_) {
        summary.unread_count = 0;
        if (message.from == peer) {
            saveConversationReadAt(peer, parseConversationTimestamp(message.timestamp));
        }
    } else if (increment_unread) {
        ++summary.unread_count;
    }
    filterFriendList(contact_search_edit_->text());
}

void MainWindow::clearConversationUnread(const QString& peer) {
    auto it = conversation_summaries_.find(peer);
    if (it == conversation_summaries_.end() || it->unread_count == 0) {
        return;
    }
    it->unread_count = 0;
    filterFriendList(contact_search_edit_->text());
}

int MainWindow::calculateUnreadCount(const QString& peer,
                                     const QList<cloudvault::ChatMessage>& messages) const {
    const QDateTime last_read_at = loadConversationReadAt(peer);
    if (!last_read_at.isValid()) {
        int unread_count = 0;
        for (const auto& message : messages) {
            if (message.from == peer) {
                ++unread_count;
            }
        }
        return unread_count;
    }

    int unread_count = 0;
    for (const auto& message : messages) {
        if (message.from != peer) {
            continue;
        }
        const QDateTime message_time = parseConversationTimestamp(message.timestamp);
        if (!message_time.isValid() || message_time > last_read_at) {
            ++unread_count;
        }
    }
    return unread_count;
}

QDateTime MainWindow::loadConversationReadAt(const QString& peer) const {
    if (peer.trimmed().isEmpty()) {
        return {};
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("chat/read-state/%1").arg(current_username_));
    const QDateTime value = settings.value(peer.trimmed()).toDateTime();
    settings.endGroup();
    return value;
}

void MainWindow::saveConversationReadAt(const QString& peer, const QDateTime& timestamp) const {
    const QString trimmed = peer.trimmed();
    if (trimmed.isEmpty() || !timestamp.isValid()) {
        return;
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("chat/read-state/%1").arg(current_username_));
    const QDateTime current = settings.value(trimmed).toDateTime();
    if (!current.isValid() || timestamp > current) {
        settings.setValue(trimmed, timestamp);
    }
    settings.endGroup();
}

void MainWindow::refreshFriendList(const QList<QPair<QString, bool>>& friends) {
    friends_ = friends;
    QSet<QString> current_peers;
    for (const auto& [username, online] : friends_) {
        Q_UNUSED(online);
        current_peers.insert(username);
        requestConversationSnapshot(username);
    }

    const auto known_peers = conversation_summaries_.keys();
    for (const auto& peer : known_peers) {
        if (!current_peers.contains(peer)) {
            conversation_summaries_.remove(peer);
            pending_history_requests_.remove(peer);
        }
    }
    filterFriendList(contact_search_edit_->text());
}

void MainWindow::filterFriendList(const QString& keyword) {
    const QString current = !active_chat_peer_.isEmpty()
        ? active_chat_peer_
        : selectedFriend();
    const QString trimmed = keyword.trimmed();
    QList<QPair<QString, bool>> ordered_friends = friends_;
    std::sort(ordered_friends.begin(), ordered_friends.end(),
              [this](const QPair<QString, bool>& lhs, const QPair<QString, bool>& rhs) {
                  const auto lhs_summary = conversation_summaries_.constFind(lhs.first);
                  const auto rhs_summary = conversation_summaries_.constFind(rhs.first);
                  const bool lhs_has_message = lhs_summary != conversation_summaries_.constEnd()
                                            && lhs_summary->has_message;
                  const bool rhs_has_message = rhs_summary != conversation_summaries_.constEnd()
                                            && rhs_summary->has_message;
                  if (lhs_has_message != rhs_has_message) {
                      return lhs_has_message;
                  }

                  if (lhs_has_message && rhs_has_message) {
                      const QDateTime lhs_time = parseConversationTimestamp(lhs_summary->timestamp);
                      const QDateTime rhs_time = parseConversationTimestamp(rhs_summary->timestamp);
                      if (lhs_time.isValid() && rhs_time.isValid() && lhs_time != rhs_time) {
                          return lhs_time > rhs_time;
                      }
                      if (lhs_summary->unread_count != rhs_summary->unread_count) {
                          return lhs_summary->unread_count > rhs_summary->unread_count;
                      }
                  }

                  if (lhs.second != rhs.second) {
                      return lhs.second;
                  }
                  return QString::localeAwareCompare(lhs.first, rhs.first) < 0;
              });

    {
        const QSignalBlocker blocker(contact_list_);
        contact_list_->clear();

        for (const auto& [username, online] : ordered_friends) {
            if (!trimmed.isEmpty() && !username.contains(trimmed, Qt::CaseInsensitive)) {
                continue;
            }

            const auto summary_it = conversation_summaries_.constFind(username);
            const bool has_message = summary_it != conversation_summaries_.constEnd()
                                  && summary_it->has_message;
            const QString preview = has_message
                ? summary_it->preview
                : QStringLiteral("暂无消息");
            const QString time = has_message ? formatConversationTime(summary_it->timestamp) : QString();
            const int unread = has_message ? summary_it->unread_count : 0;

            auto* item = new QListWidgetItem(contact_list_);
            item->setSizeHint(QSize(0, 64));
            item->setData(Qt::UserRole, username);
            item->setData(Qt::UserRole + 1, online);

            const bool is_selected = username == current;
            auto* widget = createContactItemWidget(username,
                                                   preview,
                                                   time,
                                                   unread,
                                                   online,
                                                   is_selected,
                                                   contact_list_);
            contact_list_->setItemWidget(item, widget);
            if (is_selected) {
                contact_list_->setCurrentItem(item);
                item->setSelected(true);
            }
        }

        if (contact_list_->count() > 0 && current.isEmpty()) {
            contact_list_->setCurrentRow(0);
        }
    }

    if (contact_list_->count() == 0) {
        showChatEmptyState();
    }

    updateContactSelectionState();

    if (selectedFriend() != current) {
        applySelectedFriend();
    }
}

void MainWindow::updateContactSelectionState() {
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

void MainWindow::applySelectedFriend() {
    updateContactSelectionState();

    const QString username = selectedFriend();
    if (username.isEmpty()) {
        showChatEmptyState();
        return;
    }

    const bool online = selectedFriendOnline();
    active_chat_peer_ = username;
    clearConversationUnread(username);
    showChatConversation(username, online);
    chat_status_label_->setText(QStringLiteral("正在加载历史消息…"));
    requestConversationSnapshot(username);
}

void MainWindow::sendCurrentMessage() {
    if (active_chat_peer_.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法发送"),
                             QStringLiteral("请先选择一个联系人。"));
        return;
    }

    const QString content = message_input_->toPlainText().trimmed();
    if (content.isEmpty()) {
        return;
    }

    chat_service_.sendMessage(active_chat_peer_, content);
    const cloudvault::ChatMessage local_message{
        current_username_,
        active_chat_peer_,
        content,
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
    };
    appendChatMessage(local_message);
    applyConversationMessage(local_message, false);
    message_input_->clear();
    showChatConversation(active_chat_peer_, selectedFriendOnline());
    QTimer::singleShot(120, this, [this, peer = active_chat_peer_] {
        if (!peer.isEmpty() && peer == active_chat_peer_) {
            requestConversationSnapshot(peer);
        }
    });
}

void MainWindow::setFileStatus(const QString& message, bool error) {
    file_status_label_->setProperty("error", error);
    file_status_label_->setText(message);
    repolish(file_status_label_);
}

void MainWindow::resetFileActionSummary() {
    detail_file_target_label_->setText(QStringLiteral("选中：未选择文件"));
    detail_file_meta_label_->setText(QStringLiteral("未选择文件"));
}

void MainWindow::refreshFileList(const QString& path,
                                 const cloudvault::FileEntries& entries) {
    if (!file_search_mode_) {
        current_file_path_ = path.isEmpty() ? QStringLiteral("/") : path;
    }
    current_file_entries_ = entries;
    file_list_->clear();

    for (const auto& entry : entries) {
        auto* item = new QListWidgetItem(file_list_);
        item->setSizeHint(QSize(0, 44));
        item->setData(Qt::UserRole, entry.path);
        item->setData(Qt::UserRole + 1, entry.is_dir);
        item->setData(Qt::UserRole + 2, entry.name);
        auto* widget = createFileItemWidget(entry, false, file_list_);
        file_list_->setItemWidget(item, widget);
    }

    if (file_search_mode_) {
        file_path_label_->setText(QStringLiteral("搜索：%1").arg(current_file_query_));
    } else {
        file_path_label_->setText(current_file_path_);
    }
    file_path_label_->setToolTip(file_search_mode_ ? current_file_query_ : current_file_path_);
    file_back_btn_->setEnabled(file_search_mode_ || current_file_path_ != QStringLiteral("/"));

    file_empty_state_label_->setText(file_search_mode_
        ? QStringLiteral("没有匹配的文件\n修改关键词后回车重新搜索")
        : QStringLiteral("当前目录为空\n点击“+ 新建”或使用“上传”开始管理文件"));
    file_empty_state_label_->setVisible(file_list_->count() == 0);
    file_list_->setVisible(file_list_->count() > 0);

    if (file_list_->count() > 0) {
        file_list_->setCurrentRow(0);
        setFileStatus(file_search_mode_
                          ? QStringLiteral("搜索完成，共找到 %1 项。").arg(file_list_->count())
                          : QStringLiteral("目录已刷新，共 %1 项。").arg(file_list_->count()));
    } else {
        resetFileActionSummary();
        setFileStatus(file_search_mode_
                          ? QStringLiteral("没有找到与“%1”匹配的文件。").arg(current_file_query_)
                          : QStringLiteral("当前目录为空。"));
    }

    updateFileSelectionState();
}

void MainWindow::updateFileSelectionState() {
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

    const bool has_selection = file_list_->currentItem() != nullptr;
    const bool is_file = has_selection && !selectedFileIsDir();
    file_download_btn_->setEnabled(is_file);
    file_share_btn_->setEnabled(is_file);
    file_rename_btn_->setEnabled(has_selection);
    file_move_btn_->setEnabled(has_selection);
    file_delete_btn_->setEnabled(has_selection);
}

void MainWindow::applySelectedFile() {
    updateFileSelectionState();

    const QString path = selectedFilePath();
    if (path.isEmpty()) {
        resetFileActionSummary();
        return;
    }

    const bool is_dir = selectedFileIsDir();
    const QString name = file_list_->currentItem()->data(Qt::UserRole + 2).toString();
    detail_file_target_label_->setText(QStringLiteral("选中：%1").arg(name));
    detail_file_meta_label_->setText(is_dir
        ? QStringLiteral("目录 · 双击可进入")
        : QStringLiteral("文件 · 可下载 / 分享 / 重命名 / 移动 / 删除"));
    detail_file_meta_label_->setToolTip(path);
}

void MainWindow::navigateToFilePath(const QString& path) {
    file_search_mode_ = false;
    current_file_query_.clear();
    setFileStatus(QStringLiteral("正在打开 %1 …").arg(path.isEmpty() ? QStringLiteral("/") : path));
    file_service_.listFiles(path.isEmpty() ? QStringLiteral("/") : path);
}

void MainWindow::openCurrentParentDirectory() {
    if (file_search_mode_) {
        file_search_mode_ = false;
        current_file_query_.clear();
        file_search_edit_->clear();
        setFileStatus(QStringLiteral("返回当前目录…"));
        file_service_.listFiles(current_file_path_);
        return;
    }

    if (current_file_path_ == QStringLiteral("/") || current_file_path_.isEmpty()) {
        return;
    }

    const QString normalized = current_file_path_.endsWith('/')
        ? current_file_path_.left(current_file_path_.size() - 1)
        : current_file_path_;
    const int index = normalized.lastIndexOf('/');
    const QString parent = (index <= 0) ? QStringLiteral("/")
                                        : normalized.left(index);
    navigateToFilePath(parent);
}

void MainWindow::createDirectory() {
    const QString name = QInputDialog::getText(
        this,
        QStringLiteral("新建文件夹"),
        QStringLiteral("请输入新文件夹名称：")).trimmed();
    if (name.isEmpty()) {
        return;
    }
    setFileStatus(QStringLiteral("正在创建“%1”…").arg(name));
    file_service_.createDirectory(current_file_path_, name);
}

void MainWindow::renameSelectedFile() {
    const QString path = selectedFilePath();
    if (path.isEmpty()) {
        return;
    }

    const QString current_name = file_list_->currentItem()->data(Qt::UserRole + 2).toString();
    const QString new_name = QInputDialog::getText(
        this,
        QStringLiteral("重命名"),
        QStringLiteral("请输入新名称："),
        QLineEdit::Normal,
        current_name).trimmed();
    if (new_name.isEmpty() || new_name == current_name) {
        return;
    }
    setFileStatus(QStringLiteral("正在将“%1”重命名为“%2”…").arg(current_name, new_name));
    file_service_.renamePath(path, new_name);
}

void MainWindow::moveSelectedFile() {
    const QString path = selectedFilePath();
    if (path.isEmpty()) {
        return;
    }

    const QString destination = QInputDialog::getText(
        this,
        QStringLiteral("移动"),
        QStringLiteral("请输入目标目录路径（例如 /archive）："),
        QLineEdit::Normal,
        current_file_path_).trimmed();
    if (destination.isEmpty()) {
        return;
    }
    setFileStatus(QStringLiteral("正在移动到 %1 …").arg(destination));
    file_service_.movePath(path, destination);
}

void MainWindow::deleteSelectedFile() {
    const QString path = selectedFilePath();
    if (path.isEmpty()) {
        return;
    }

    const QString current_name = file_list_->currentItem()->data(Qt::UserRole + 2).toString();
    const auto reply = QMessageBox::question(
        this,
        QStringLiteral("删除"),
        QStringLiteral("确认删除“%1”吗？此操作不可恢复。").arg(current_name),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    setFileStatus(QStringLiteral("正在删除“%1”…").arg(current_name));
    file_service_.deletePath(path);
}

void MainWindow::runFileSearch() {
    const QString keyword = file_search_edit_->text().trimmed();
    if (keyword.isEmpty()) {
        file_search_mode_ = false;
        current_file_query_.clear();
        setFileStatus(QStringLiteral("已清空搜索，返回当前目录。"));
        file_service_.listFiles(current_file_path_);
        return;
    }
    setFileStatus(QStringLiteral("正在搜索“%1”…").arg(keyword));
    file_service_.search(keyword);
}

void MainWindow::clearFileSearchIfNeeded(const QString& text) {
    if (!text.trimmed().isEmpty() || !file_search_mode_) {
        return;
    }
    file_search_mode_ = false;
    current_file_query_.clear();
    setFileStatus(QStringLiteral("搜索已清空，返回当前目录。"));
    file_service_.listFiles(current_file_path_);
}

void MainWindow::switchMainTab(int index) {
    center_stack_->setCurrentIndex(index);

    const bool show_sidebar = (index != 2);
    const bool show_detail = (index == 0);
    sidebar_panel_->setVisible(show_sidebar);
    detail_panel_->setVisible(show_detail);

    if (index == 0) {
        sidebar_title_label_->setText(QStringLiteral("消息"));
        contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
        sidebar_action_btn_->setVisible(true);
    } else if (index == 1) {
        sidebar_title_label_->setText(QStringLiteral("文件"));
        contact_search_edit_->setPlaceholderText(QStringLiteral("搜索联系人…"));
        sidebar_action_btn_->setVisible(false);
    } else {
        sidebar_action_btn_->setVisible(false);
    }

    for (int i = 0; i < 3; ++i) {
        QPushButton* button = (i == 0) ? message_tab_btn_
                              : (i == 1) ? file_tab_btn_
                                         : profile_tab_btn_;
        button->setProperty("active", i == index);
        repolish(button);
    }

    if (index == 0) {
        content_splitter_->setSizes({260, std::max(420, width() - 480), 220});
    } else if (index == 1) {
        content_splitter_->setSizes({260, std::max(500, width() - 260), 0});
    } else {
        content_splitter_->setSizes({0, width(), 0});
    }
}

void MainWindow::openOnlineUserDialog() {
    if (!sidebar_panel_->isVisible()) {
        return;
    }

    OnlineUserDialog dialog(friends_, this);
    connect(&dialog, &OnlineUserDialog::refreshRequested,
            this, [this] { friend_service_.flushFriends(); });
    connect(&friend_service_, &cloudvault::FriendService::friendsRefreshed,
            &dialog, &OnlineUserDialog::setUsers);
    connect(&dialog, &OnlineUserDialog::userChosen,
            this, [this](const QString& username) { friend_service_.addFriend(username); });
    dialog.exec();
}

void MainWindow::openGroupListDialog() {
    const QList<GroupListEntry> groups;
    GroupListDialog dialog(groups, this);
    connect(&dialog, &GroupListDialog::groupChosen, this, [this](const QString& group_name) {
        active_chat_peer_.clear();
        active_group_name_ = group_name;
        if (chat_stack_) {
            chat_stack_->setCurrentIndex(2);
        }
        if (group_chat_title_label_) {
            group_chat_title_label_->setText(group_name);
        }
        appendEventLog(QStringLiteral("进入群聊 %1").arg(group_name), QStringLiteral("→"));
    });
    dialog.exec();
}

void MainWindow::openShareFileDialog() {
    const QString path = selectedFilePath();
    if (path.isEmpty() || selectedFileIsDir()) {
        return;
    }

    const QString file_name = file_list_->currentItem()->data(Qt::UserRole + 2).toString();
    ShareFileDialog dialog(file_name, friends_, this);
    connect(&dialog, &ShareFileDialog::shareConfirmed,
            this, [this, path](const QString&, const QStringList& targets) {
                if (targets.isEmpty()) {
                    return;
                }
                setFileStatus(QStringLiteral("正在发送 %1 的分享请求…")
                                  .arg(QFileInfo(path).fileName()));
                share_service_.shareFile(path, targets);
            });
    dialog.exec();
}

void MainWindow::uploadFile() {
    const QStringList local_paths = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("选择要上传的文件"),
        QDir::homePath(),
        QStringLiteral("所有文件 (*.*)"));
    if (local_paths.isEmpty()) {
        return;
    }

    enqueueUploads(local_paths);
}

void MainWindow::downloadSelectedFile() {
    const QString remote_path = selectedFilePath();
    if (remote_path.isEmpty() || selectedFileIsDir()) {
        return;
    }

    const QString default_path = QDir::homePath() + QStringLiteral("/") + QFileInfo(remote_path).fileName();
    const QString save_path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存文件"),
        default_path);
    if (save_path.isEmpty()) {
        return;
    }

    showTransferRow(QStringLiteral("准备下载"), 0, true);
    setFileStatus(QStringLiteral("正在准备下载 %1 …").arg(QFileInfo(remote_path).fileName()));
    file_service_.downloadFile(remote_path, save_path);
}

void MainWindow::enqueueUploads(const QStringList& local_paths) {
    for (const QString& path : local_paths) {
        if (!path.trimmed().isEmpty()) {
            pending_upload_paths_.append(path);
        }
    }
    if (pending_upload_paths_.isEmpty()) {
        return;
    }

    showTransferRow(QStringLiteral("等待上传队列"), 0, true);
    startNextQueuedUpload();
}

void MainWindow::startNextQueuedUpload() {
    if (file_service_.hasActiveTransfer() || pending_upload_paths_.isEmpty()) {
        file_transfer_cancel_btn_->setEnabled(!pending_upload_paths_.isEmpty());
        return;
    }

    const QString next_path = pending_upload_paths_.takeFirst();
    const QString filename = QFileInfo(next_path).fileName();
    showTransferRow(QStringLiteral("准备上传 %1").arg(filename), 0, true);
    setFileStatus(QStringLiteral("正在准备上传 %1 …").arg(filename));
    file_transfer_cancel_btn_->setEnabled(!pending_upload_paths_.isEmpty());
    file_service_.uploadFile(next_path, current_file_path_);
}

void MainWindow::showTransferRow(const QString& title, int percent, bool cancellable) {
    file_transfer_row_->show();
    file_transfer_label_->setText(title);
    file_transfer_bar_->setValue(std::clamp(percent, 0, 100));
    file_transfer_percent_label_->setText(QStringLiteral("%1%").arg(std::clamp(percent, 0, 100)));
    file_transfer_cancel_btn_->setEnabled(cancellable);
}

void MainWindow::hideTransferRow() {
    file_transfer_row_->hide();
    file_transfer_bar_->setValue(0);
    file_transfer_percent_label_->setText(QStringLiteral("0%"));
}

void MainWindow::appendEventLog(const QString& message, const QString& icon) {
    if (!event_log_list_) {
        return;
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    auto* item = new QListWidgetItem(
        QStringLiteral("[%1] %2 %3").arg(timestamp, icon, message),
        event_log_list_);
    item->setToolTip(message);
    while (event_log_list_->count() > 50) {
        delete event_log_list_->takeItem(0);
    }
    event_log_list_->scrollToBottom();
}

void MainWindow::showConnectionBanner(const QString& message) {
    if (!connection_banner_ || !connection_banner_label_) {
        return;
    }
    connection_banner_label_->setText(message);
    connection_banner_->show();
}

void MainWindow::hideConnectionBanner() {
    if (connection_banner_) {
        connection_banner_->hide();
    }
}

void MainWindow::loadProfileDraft() {
    QSettings settings;
    settings.beginGroup(QStringLiteral("profile/%1").arg(current_username_));
    const QString nickname = settings.value(QStringLiteral("nickname"), current_username_).toString();
    const QString signature = settings.value(QStringLiteral("signature")).toString();
    settings.endGroup();

    profile_nickname_edit_->setText(nickname);
    profile_signature_edit_->setText(signature);
    profile_name_label_->setText(nickname.trimmed().isEmpty() ? current_username_ : nickname.trimmed());
}

void MainWindow::saveProfileDraft() {
    const QString nickname = profile_nickname_edit_->text().trimmed();
    const QString signature = profile_signature_edit_->text().trimmed();

    QSettings settings;
    settings.beginGroup(QStringLiteral("profile/%1").arg(current_username_));
    settings.setValue(QStringLiteral("nickname"), nickname);
    settings.setValue(QStringLiteral("signature"), signature);
    settings.endGroup();

    profile_name_label_->setText(nickname.isEmpty() ? current_username_ : nickname);
}

QString MainWindow::selectedFriend() const {
    if (auto* item = contact_list_->currentItem()) {
        return item->data(Qt::UserRole).toString();
    }
    return {};
}

bool MainWindow::selectedFriendOnline() const {
    if (auto* item = contact_list_->currentItem()) {
        return item->data(Qt::UserRole + 1).toBool();
    }
    return false;
}

QString MainWindow::selectedFilePath() const {
    if (auto* item = file_list_->currentItem()) {
        return item->data(Qt::UserRole).toString();
    }
    return {};
}

bool MainWindow::selectedFileIsDir() const {
    if (auto* item = file_list_->currentItem()) {
        return item->data(Qt::UserRole + 1).toBool();
    }
    return false;
}

#include "main_window.moc"

void MainWindow::appendDateDividerIfNeeded(const QString& timestamp) {
    const QString current_date = dateKeyForTimestamp(timestamp);
    if (message_list_->count() > 0) {
        if (auto* last_item = message_list_->item(message_list_->count() - 1)) {
            if (last_item->data(kMessageKindRole).toString() == QStringLiteral("message")) {
                const QString last_date = dateKeyForTimestamp(
                    last_item->data(kMessageTimestampRole).toString());
                if (last_date == current_date) {
                    return;
                }
            } else if (last_item->data(kMessageKindRole).toString() == QStringLiteral("divider") &&
                       last_item->data(kMessageDateRole).toString() == current_date) {
                return;
            }
        }
    }

    auto* divider = new QListWidgetItem(message_list_);
    divider->setData(kMessageKindRole, QStringLiteral("divider"));
    divider->setData(kMessageDateRole, current_date);
}
