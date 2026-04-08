// =============================================================
// client/src/ui/widget_helpers.h
// 客户端 UI 公共辅助函数
//
// 这里集中定义在多个面板中重复使用的纯工具函数，
// 每个 .cpp 文件通过 using namespace cv_ui 引入，
// 不再各自维护一份副本。
// =============================================================

#pragma once

#include <QColor>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QStyle>
#include <QWidget>

namespace cv_ui {

/// 根据字符串 seed 返回 0-5 的头像色板索引。
inline int avatarVariantForSeed(const QString& seed) {
    return static_cast<int>(qHash(seed)) % 6;
}

/// 强制 QSS 重新应用到 widget（unpolish + polish + update）。
inline void repolish(QWidget* widget) {
    if (!widget) {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

/// 将 QLabel 初始化为圆形头像徽章（objectName="avatarBadge"）。
inline void prepareAvatarBadge(QLabel* label, const QString& seed, int size) {
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

/// 创建带圆形头像和可选在线指示点的包装 widget。
inline QWidget* createAvatarWidget(const QString& seed,
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

/// 创建固定尺寸的图标按钮（objectName="iconBtn"）。
inline QPushButton* createIconButton(const QString& text,
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

/// 将字节数格式化为人类可读的文件大小字符串（B / KB / MB / GB）。
inline QString formatFileSize(quint64 size) {
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

/// 为 widget 添加投影效果。
inline void applyShadow(QWidget* widget,
                        int blur = 40,
                        int offset_y = 8,
                        int alpha = 30) {
    auto* effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(blur);
    effect->setOffset(0, offset_y);
    effect->setColor(QColor(17, 24, 39, alpha));
    widget->setGraphicsEffect(effect);
}

} // namespace cv_ui
