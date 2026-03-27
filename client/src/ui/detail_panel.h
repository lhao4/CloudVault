// =============================================================
// client/src/ui/detail_panel.h
// 右侧详情面板
// =============================================================

#pragma once

#include <QFrame>

class QLabel;
class QVBoxLayout;

class DetailPanel : public QFrame {
    Q_OBJECT

public:
    explicit DetailPanel(const QString& current_username, QWidget* parent = nullptr);

    QLabel* contactAvatarLabel() const;
    QLabel* contactNameLabel() const;
    QLabel* contactStatusLabel() const;
    QVBoxLayout* sharedFilesLayout() const;
    QLabel* sharedEmptyLabel() const;

private:
    QLabel* contact_avatar_label_ = nullptr;
    QLabel* contact_name_label_ = nullptr;
    QLabel* contact_status_label_ = nullptr;
    QVBoxLayout* shared_files_layout_ = nullptr;
    QLabel* shared_empty_label_ = nullptr;
};
