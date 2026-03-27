// =============================================================
// client/src/ui/detail_panel.h
// 右侧详情面板
// =============================================================

#pragma once

#include <QFrame>

class QLabel;
class QVBoxLayout;

/**
 * @brief 右侧详情面板。
 *
 * 展示当前会话联系人基础信息和共享文件摘要区。
 */
class DetailPanel : public QFrame {
    Q_OBJECT

public:
    /**
     * @brief 构造详情面板。
     * @param current_username 当前用户名，用于初始化空态头像。
     * @param parent Qt 父对象。
     */
    explicit DetailPanel(const QString& current_username, QWidget* parent = nullptr);

    /**
     * @brief 切换到空状态。
     * @param current_username 当前用户名。
     */
    void showEmptyState(const QString& current_username);
    /**
     * @brief 展示联系人信息。
     * @param username 联系人名称。
     * @param online 在线状态。
     */
    void showContact(const QString& username, bool online);
    /**
     * @brief 清空共享文件摘要区。
     */
    void clearSharedFiles();
    /**
     * @brief 添加共享摘要条目（当前实现为单条覆盖）。
     * @param title 标题。
     * @param meta 元信息。
     */
    void addSharedSummary(const QString& title, const QString& meta);
    /**
     * @brief 展示共享区空态文案。
     * @param message 空态文本。
     */
    void showSharedEmptyMessage(const QString& message);

    /**
     * @brief 获取联系人头像标签。
     * @return 标签指针。
     */
    QLabel* contactAvatarLabel() const;
    /**
     * @brief 获取联系人名称标签。
     * @return 标签指针。
     */
    QLabel* contactNameLabel() const;
    /**
     * @brief 获取联系人状态标签。
     * @return 标签指针。
     */
    QLabel* contactStatusLabel() const;
    /**
     * @brief 获取共享文件布局。
     * @return 布局指针。
     */
    QVBoxLayout* sharedFilesLayout() const;
    /**
     * @brief 获取共享空态标签。
     * @return 标签指针。
     */
    QLabel* sharedEmptyLabel() const;

private:
    /// @brief 联系人头像标签。
    QLabel* contact_avatar_label_ = nullptr;
    /// @brief 联系人名称标签。
    QLabel* contact_name_label_ = nullptr;
    /// @brief 联系人状态标签。
    QLabel* contact_status_label_ = nullptr;
    /// @brief 共享文件条目布局。
    QVBoxLayout* shared_files_layout_ = nullptr;
    /// @brief 共享区空态标签。
    QLabel* shared_empty_label_ = nullptr;
};
