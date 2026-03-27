// =============================================================
// client/src/ui/profile_panel.h
// 个人信息面板
// =============================================================

#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QFrame;
class QResizeEvent;

/**
 * @brief 个人信息面板。
 *
 * 提供昵称/签名草稿编辑、保存/取消、退出登录入口，
 * 并实现带顶部横幅与悬浮头像的资料卡片布局。
 */
class ProfilePanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造个人信息面板。
     * @param current_username 当前用户名。
     * @param parent Qt 父对象。
     */
    explicit ProfilePanel(const QString& current_username, QWidget* parent = nullptr);

    /**
     * @brief 设置展示名称。
     * @param name 展示用昵称。
     */
    void setDisplayName(const QString& name);
    /**
     * @brief 批量设置草稿输入框内容。
     * @param nickname 昵称草稿。
     * @param signature 个性签名草稿。
     */
    void setDraftValues(const QString& nickname, const QString& signature);
    /**
     * @brief 获取昵称输入文本。
     * @return 昵称文本。
     */
    QString nicknameText() const;
    /**
     * @brief 获取签名输入文本。
     * @return 签名文本。
     */
    QString signatureText() const;

signals:
    /// @brief 点击保存按钮时发出。
    void saveRequested();
    /// @brief 点击取消按钮时发出。
    void cancelRequested();
    /// @brief 点击退出登录按钮时发出。
    void logoutRequested();

private:
    /**
     * @brief 响应尺寸变化，重新放置悬浮头像。
     * @param event Qt 尺寸事件。
     */
    void resizeEvent(QResizeEvent* event) override;
    /**
     * @brief 根据横幅位置更新头像几何。
     */
    void updateAvatarGeometry();

    /// @brief 资料卡片容器。
    QFrame* profile_card_ = nullptr;
    /// @brief 卡片顶部横幅。
    QFrame* profile_banner_ = nullptr;
    /// @brief 悬浮头像标签。
    QLabel* profile_avatar_label_ = nullptr;
    /// @brief 展示名称标签。
    QLabel* profile_name_label_ = nullptr;
    /// @brief 用户 ID 标签（@username）。
    QLabel* profile_id_label_ = nullptr;
    /// @brief 昵称输入框。
    QLineEdit* profile_nickname_edit_ = nullptr;
    /// @brief 个性签名输入框。
    QLineEdit* profile_signature_edit_ = nullptr;
};
