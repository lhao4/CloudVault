// =============================================================
// client/src/ui/profile_panel.h
// 个人信息面板
// =============================================================

#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QFrame;

class ProfilePanel : public QWidget {
    Q_OBJECT

public:
    explicit ProfilePanel(const QString& current_username, QWidget* parent = nullptr);

    void setDisplayName(const QString& name);
    void setDraftValues(const QString& nickname, const QString& signature);
    QString nicknameText() const;
    QString signatureText() const;

signals:
    void saveRequested();
    void cancelRequested();
    void logoutRequested();

private:
    void resizeEvent(QResizeEvent* event) override;
    void updateAvatarGeometry();

    QFrame* profile_card_ = nullptr;
    QFrame* profile_banner_ = nullptr;
    QLabel* profile_avatar_label_ = nullptr;
    QLabel* profile_name_label_ = nullptr;
    QLabel* profile_id_label_ = nullptr;
    QLineEdit* profile_nickname_edit_ = nullptr;
    QLineEdit* profile_signature_edit_ = nullptr;
};
