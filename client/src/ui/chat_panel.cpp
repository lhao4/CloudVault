// =============================================================
// client/src/ui/chat_panel.cpp
// 聊天中栏面板
// =============================================================

#include "chat_panel.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QTextEdit>
#include <QVBoxLayout>

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

    auto* empty_hint = new QLabel(QStringLiteral("从左侧联系人列表中选择一个会话。"), empty_page);
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
    chat_top_bar->setFixedHeight(56);
    auto* chat_top_layout = new QHBoxLayout(chat_top_bar);
    chat_top_layout->setContentsMargins(16, 0, 16, 0);
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

    group_list_button_ = createIconButton(QStringLiteral("组"),
                                          QStringLiteral("群组列表"),
                                          30,
                                          chat_top_bar);
    chat_top_layout->addWidget(group_list_button_);
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

    auto* send_text_edit = new SendTextEdit(compose_bar);
    send_text_edit->setObjectName(QStringLiteral("messageInput"));
    send_text_edit->setPlaceholderText(QStringLiteral("输入消息…"));
    message_input_ = send_text_edit;
    compose_layout->addWidget(message_input_, 1);

    send_button_ = new QPushButton(QStringLiteral("➤"), compose_bar);
    send_button_->setObjectName(QStringLiteral("primaryBtn"));
    send_button_->setCursor(Qt::PointingHandCursor);
    send_button_->setFixedSize(44, 44);
    compose_layout->addWidget(send_button_);
    conversation_layout->addWidget(compose_bar);

    connect(send_text_edit, &SendTextEdit::submitRequested,
            this, &ChatPanel::sendRequested);
    connect(group_list_button_, &QPushButton::clicked,
            this, &ChatPanel::groupListRequested);

    stack_->addWidget(conversation_page);

    auto* group_page = new QWidget(stack_);
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

    group_title_label_ = new QLabel(QStringLiteral("群聊"), group_top_bar);
    group_title_label_->setObjectName(QStringLiteral("chatTitle"));
    group_text_layout->addWidget(group_title_label_);

    group_status_label_ = new QLabel(QStringLiteral("当前版本尚未接入群聊后端"), group_top_bar);
    group_status_label_->setObjectName(QStringLiteral("chatStatus"));
    group_text_layout->addWidget(group_status_label_);
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

    stack_->addWidget(group_page);
}

QStackedWidget* ChatPanel::stack() const { return stack_; }
QLabel* ChatPanel::avatarLabel() const { return avatar_label_; }
QLabel* ChatPanel::titleLabel() const { return title_label_; }
QLabel* ChatPanel::statusLabel() const { return status_label_; }
QLabel* ChatPanel::groupTitleLabel() const { return group_title_label_; }
QLabel* ChatPanel::groupStatusLabel() const { return group_status_label_; }
QListWidget* ChatPanel::messageList() const { return message_list_; }
QTextEdit* ChatPanel::messageInput() const { return message_input_; }
QPushButton* ChatPanel::sendButton() const { return send_button_; }
QPushButton* ChatPanel::groupListButton() const { return group_list_button_; }

#include "chat_panel.moc"
