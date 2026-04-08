// =============================================================
// client/src/ui/chat_panel.cpp
// 聊天中栏面板
// =============================================================

#include "chat_panel.h"
#include "ui/widget_helpers.h"

#include <QAbstractItemView>
#include <QDate>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace cv_ui;

namespace {

constexpr int kMessageKindRole = Qt::UserRole + 40;
constexpr int kMessageContentRole = Qt::UserRole + 41;
constexpr int kMessageTimestampRole = Qt::UserRole + 42;
constexpr int kMessageOutgoingRole = Qt::UserRole + 43;
constexpr int kMessageAvatarSeedRole = Qt::UserRole + 44;
constexpr int kMessageDateRole = Qt::UserRole + 45;

QString defaultEmptyStatus() {
    return QStringLiteral("请选择联系人开始聊天");
}

QDateTime parseConversationTimestamp(const QString& timestamp) {
    static const QStringList text_formats = {
        QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"),
        QStringLiteral("yyyy-MM-dd HH:mm:ss"),
    };

    for (const auto& format : text_formats) {
        const QDateTime dt = QDateTime::fromString(timestamp, format);
        if (dt.isValid()) {
            return dt;
        }
    }
    for (const auto format : {Qt::ISODateWithMs, Qt::ISODate}) {
        const QDateTime dt = QDateTime::fromString(timestamp, format);
        if (dt.isValid()) {
            return dt;
        }
    }
    return {};
}

QString conversationDateKey(const QString& timestamp) {
    const QDateTime dt = parseConversationTimestamp(timestamp);
    if (!dt.isValid()) {
        return QStringLiteral("unknown");
    }
    return dt.date().toString(QStringLiteral("yyyy-MM-dd"));
}

QString formatConversationTime(const QString& timestamp) {
    const QDateTime dt = parseConversationTimestamp(timestamp);
    if (!dt.isValid()) {
        return QStringLiteral("--:--");
    }
    return dt.time().toString(QStringLiteral("HH:mm"));
}

QString formatDividerDate(const QString& key) {
    const QDate date = QDate::fromString(key, QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid()) {
        return key;
    }
    return date.toString(QStringLiteral("yyyy-MM-dd"));
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

} // namespace

ChatPanel::ChatPanel(const QString& current_username, QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("chatPage"));
    auto* page_layout = new QVBoxLayout(this);
    page_layout->setContentsMargins(0, 0, 0, 0);
    page_layout->setSpacing(0);

    stack_ = new QStackedWidget(this);
    page_layout->addWidget(stack_);

    auto* empty_page = new QWidget(stack_);
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

    auto* empty_hint = new QLabel(QStringLiteral("从左侧会话或联系人视图中选择一个对象。"), empty_page);
    empty_hint->setObjectName(QStringLiteral("chatEmptyHint"));
    empty_hint->setAlignment(Qt::AlignCenter);
    empty_layout->addWidget(empty_hint);
    empty_layout->addStretch();
    stack_->addWidget(empty_page);

    auto* conversation_page = new QWidget(stack_);
    auto* conversation_layout = new QVBoxLayout(conversation_page);
    conversation_layout->setContentsMargins(0, 0, 0, 0);
    conversation_layout->setSpacing(0);

    auto* chat_top_bar = new QFrame(conversation_page);
    chat_top_bar->setObjectName(QStringLiteral("chatTopBar"));
    chat_top_bar->setFixedHeight(64);
    auto* chat_top_layout = new QHBoxLayout(chat_top_bar);
    chat_top_layout->setContentsMargins(20, 0, 20, 0);
    chat_top_layout->setSpacing(10);

    chat_top_layout->addWidget(createAvatarWidget(current_username, 34, false, chat_top_bar));
    avatar_label_ = chat_top_bar->findChild<QLabel*>(QStringLiteral("avatarBadge"));
    prepareAvatarBadge(avatar_label_, current_username, 34);

    auto* chat_text_layout = new QVBoxLayout();
    chat_text_layout->setContentsMargins(0, 7, 0, 7);
    chat_text_layout->setSpacing(1);

    title_label_ = new QLabel(QStringLiteral("联系人"), chat_top_bar);
    title_label_->setObjectName(QStringLiteral("chatTitle"));
    chat_text_layout->addWidget(title_label_);

    status_label_ = new QLabel(QStringLiteral("请选择联系人开始聊天"), chat_top_bar);
    status_label_->setObjectName(QStringLiteral("chatStatus"));
    chat_text_layout->addWidget(status_label_);
    chat_top_layout->addLayout(chat_text_layout, 1);

    auto* groups_btn = createIconButton(QStringLiteral("📋"),
                                       QStringLiteral("群组列表"),
                                       30,
                                       chat_top_bar);
    chat_top_layout->addWidget(groups_btn);
    chat_top_layout->addWidget(createIconButton(QStringLiteral("📎"),
                                                QStringLiteral("附件"),
                                                30,
                                                chat_top_bar));
    chat_top_layout->addWidget(createIconButton(QStringLiteral("⋯"),
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
    auto* compose_layout = new QVBoxLayout(compose_bar);
    compose_layout->setContentsMargins(0, 0, 0, 0);
    compose_layout->setSpacing(0);

    auto* input_toolbar = new QFrame(compose_bar);
    input_toolbar->setObjectName(QStringLiteral("inputToolbar"));
    input_toolbar->setFixedHeight(40);
    auto* input_toolbar_layout = new QHBoxLayout(input_toolbar);
    input_toolbar_layout->setContentsMargins(20, 0, 20, 0);
    input_toolbar_layout->setSpacing(8);
    input_toolbar_layout->addWidget(createIconButton(QStringLiteral("😊"),
                                                     QStringLiteral("表情"),
                                                     24,
                                                     input_toolbar));
    input_toolbar_layout->addWidget(createIconButton(QStringLiteral("📎"),
                                                     QStringLiteral("发送文件"),
                                                     24,
                                                     input_toolbar));
    members_btn_ = createIconButton(QStringLiteral("👥"),
                                    QStringLiteral("群成员"),
                                    24,
                                    input_toolbar);
    members_btn_->setVisible(false);
    input_toolbar_layout->addWidget(members_btn_);
    input_toolbar_layout->addStretch();
    compose_layout->addWidget(input_toolbar);

    auto* send_text_edit = new SendTextEdit(compose_bar);
    send_text_edit->setObjectName(QStringLiteral("messageInput"));
    send_text_edit->setPlaceholderText(QStringLiteral("输入消息…"));
    send_text_edit->setMinimumHeight(80);
    message_input_ = send_text_edit;
    compose_layout->addWidget(message_input_, 1);

    auto* input_send_bar = new QFrame(compose_bar);
    input_send_bar->setObjectName(QStringLiteral("inputSendBar"));
    input_send_bar->setFixedHeight(48);
    auto* input_send_layout = new QHBoxLayout(input_send_bar);
    input_send_layout->setContentsMargins(20, 0, 20, 0);
    input_send_layout->setSpacing(8);
    input_send_layout->addStretch();

    send_button_ = new QPushButton(QStringLiteral("发送"), input_send_bar);
    send_button_->setObjectName(QStringLiteral("sendBtn"));
    send_button_->setCursor(Qt::PointingHandCursor);
    send_button_->setFixedSize(88, 32);
    input_send_layout->addWidget(send_button_);
    compose_layout->addWidget(input_send_bar);

    conversation_layout->addWidget(compose_bar);

    connect(send_text_edit, &SendTextEdit::submitRequested,
            this, &ChatPanel::sendRequested);
    connect(send_button_, &QPushButton::clicked,
            this, &ChatPanel::sendRequested);
    connect(groups_btn, &QPushButton::clicked,
            this, &ChatPanel::groupListRequested);
    connect(members_btn_, &QPushButton::clicked,
            this, &ChatPanel::groupListRequested);

    stack_->addWidget(conversation_page);
}

void ChatPanel::showEmptyState() {
    if (stack_) {
        stack_->setCurrentIndex(0);
    }
    if (title_label_) {
        title_label_->setText(QStringLiteral("CloudVault"));
    }
    if (status_label_) {
        status_label_->setText(defaultEmptyStatus());
    }
    if (message_list_) {
        message_list_->clear();
    }
}

void ChatPanel::showConversation(const QString& title,
                                  const QString& subtitle,
                                  bool is_group) {
    is_group_ = is_group;
    if (stack_) {
        stack_->setCurrentIndex(1);
    }
    if (title_label_) {
        title_label_->setText(title);
    }
    if (status_label_) {
        status_label_->setText(subtitle);
    }
    const QString seed = is_group ? QStringLiteral("群") : title;
    prepareAvatarBadge(avatar_label_, seed, 34);
    if (members_btn_) {
        members_btn_->setVisible(is_group);
    }
    if (message_input_) {
        message_input_->setPlaceholderText(is_group ? QStringLiteral("输入群消息…")
                                                    : QStringLiteral("输入消息…"));
    }
    // 切换会话时立即清空旧消息，避免上一个会话的气泡在新历史到达前短暂显示
    if (message_list_) {
        message_list_->clear();
    }
}

void ChatPanel::setConversationStatus(const QString& status) {
    if (status_label_) {
        status_label_->setText(status);
    }
}

void ChatPanel::appendMessage(const cloudvault::ChatMessage& message,
                              const QString& current_username) {
    appendDateDividerIfNeeded(message.timestamp);

    auto* item = new QListWidgetItem(message_list_);
    item->setData(kMessageKindRole, QStringLiteral("message"));
    item->setData(kMessageContentRole, message.content);
    item->setData(kMessageTimestampRole, message.timestamp);
    item->setData(kMessageOutgoingRole, message.from == current_username);
    item->setData(kMessageAvatarSeedRole,
                  (message.from == current_username) ? current_username : message.from);
    message_list_->scrollToBottom();
}

void ChatPanel::rebuildMessages(const QList<cloudvault::ChatMessage>& messages,
                                const QString& current_username) {
    if (!message_list_) {
        return;
    }
    message_list_->clear();
    for (const auto& message : messages) {
        appendMessage(message, current_username);
    }
    message_list_->scrollToBottom();
}

QString ChatPanel::inputText() const {
    return message_input_ ? message_input_->toPlainText() : QString();
}

void ChatPanel::clearInput() {
    if (message_input_) {
        message_input_->clear();
    }
}

void ChatPanel::appendDateDividerIfNeeded(const QString& timestamp) {
    if (!message_list_) {
        return;
    }

    const QString current_date = conversationDateKey(timestamp);
    if (message_list_->count() > 0) {
        if (auto* last_item = message_list_->item(message_list_->count() - 1)) {
            if (last_item->data(kMessageKindRole).toString() == QStringLiteral("message")) {
                const QString last_date = conversationDateKey(
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
    divider->setFlags(Qt::NoItemFlags);
}

QStackedWidget* ChatPanel::stack() const { return stack_; }
QLabel* ChatPanel::avatarLabel() const { return avatar_label_; }
QLabel* ChatPanel::titleLabel() const { return title_label_; }
QLabel* ChatPanel::statusLabel() const { return status_label_; }
QListWidget* ChatPanel::messageList() const { return message_list_; }

#include "chat_panel.moc"
