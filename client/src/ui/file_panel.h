// =============================================================
// client/src/ui/file_panel.h
// 文件中栏面板
// =============================================================

#pragma once

#include "service/file_service.h"

#include <QPair>
#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
class QFrame;
class QHBoxLayout;

/**
 * @brief 文件中栏面板。
 *
 * 提供路径栏、搜索栏、文件列表、传输进度区与底部操作栏，
 * 通过信号向 MainWindow 上报用户操作。
 */
class FilePanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造文件面板。
     * @param parent Qt 父对象。
     */
    explicit FilePanel(QWidget* parent = nullptr);

    /**
     * @brief 设置文件页状态文本。
     * @param message 状态文本。
     * @param error 是否错误态。
     */
    void setStatusMessage(const QString& message, bool error = false);
    /**
     * @brief 重置底部选中摘要为默认状态。
     */
    void resetSelectionSummary();
    /**
     * @brief 设置底部选中摘要。
     * @param name 文件名或目录名。
     * @param meta 辅助说明文本。
     * @param tooltip 提示信息。
     */
    void setSelectionSummary(const QString& name,
                             const QString& meta,
                             const QString& tooltip = QString());
    /**
     * @brief 显示传输进度行。
     * @param title 进度标题。
     * @param percent 百分比。
     * @param cancellable 是否允许取消。
     */
    void setTransferState(const QString& title, int percent, bool cancellable);
    /**
     * @brief 清空并隐藏传输进度行。
     */
    void clearTransferState();
    /**
     * @brief 用服务端数据重建文件列表。
     * @param entries 文件条目列表。
     */
    void populateEntries(const cloudvault::FileEntries& entries);
    /**
     * @brief 设置路径显示栏。
     * @param text 显示文本。
     * @param tooltip 悬浮提示。
     * @param can_go_back 返回按钮是否可用。
     */
    void setPathState(const QString& text, const QString& tooltip, bool can_go_back);
    /**
     * @brief 设置路径面包屑。
     * @param crumbs 路径段列表（标题, 对应路径）。
     * @param active_path 当前高亮路径。
     */
    void setBreadcrumbs(const QList<QPair<QString, QString>>& crumbs,
                        const QString& active_path);
    /**
     * @brief 设置当前文件上下文摘要。
     * @param title 上下文标题。
     * @param meta 上下文附加信息。
     */
    void setContextSummary(const QString& title, const QString& meta);
    /**
     * @brief 切换文件列表空态。
     * @param text 空态文案。
     * @param empty 是否为空。
     */
    void setEmptyState(const QString& text, bool empty);
    /**
     * @brief 选中列表第一项。
     */
    void selectFirstEntry();
    /**
     * @brief 根据选择状态刷新行高亮。
     */
    void refreshSelectionHighlights();
    /**
     * @brief 清空当前选中项。
     */
    void clearCurrentSelection();
    /**
     * @brief 获取搜索框文本。
     * @return 搜索关键词。
     */
    QString searchText() const;
    /**
     * @brief 清空搜索框。
     */
    void clearSearch();
    /**
     * @brief 聚焦并全选搜索框文本。
     */
    void focusSearchSelectAll();
    /**
     * @brief 获取文件列表项数量。
     * @return 条目数量。
     */
    int itemCount() const;
    /**
     * @brief 是否有选中项。
     * @return true 表示存在选中项。
     */
    bool hasSelection() const;
    /**
     * @brief 获取当前选中项完整路径。
     * @return 路径字符串。
     */
    QString currentPath() const;
    /**
     * @brief 获取当前选中项名称。
     * @return 名称字符串。
     */
    QString currentName() const;
    /**
     * @brief 当前选中项是否目录。
     * @return true 表示目录。
     */
    bool currentIsDir() const;
    /**
     * @brief 批量设置底部操作按钮可用性。
     * @param has_selection 是否有选中项。
     * @param is_file 选中项是否文件。
     */
    void setActionButtonsEnabled(bool has_selection, bool is_file);
    /**
     * @brief 设置传输取消按钮可用性。
     * @param enabled 是否可用。
     */
    void setTransferCancelEnabled(bool enabled);

    /**
     * @brief 暴露文件列表控件给上层做高级定制。
     * @return 文件列表指针。
     */
    QListWidget* fileList() const;

signals:
    /// @brief 返回上级目录。
    void backRequested();
    /// @brief 面包屑路径点击。
    void breadcrumbRequested(const QString& path);
    /// @brief 上传文件请求。
    void uploadRequested();
    /// @brief 刷新文件列表请求。
    void refreshRequested();
    /// @brief 新建目录请求。
    void createRequested();
    /// @brief 搜索请求。
    void searchRequested();
    /// @brief 搜索文本变化。
    void searchTextChanged(const QString& text);
    /// @brief 文件选中变化。
    void selectionChanged();
    /// @brief 文件项被激活（双击）。
    void itemActivated();
    /// @brief 重命名请求。
    void renameRequested();
    /// @brief 移动请求。
    void moveRequested();
    /// @brief 删除请求。
    void deleteRequested();
    /// @brief 下载请求。
    void downloadRequested();
    /// @brief 分享请求。
    void shareRequested();
    /// @brief 取消传输请求。
    void transferCancelRequested();

private:
    /// @brief 当前路径显示标签。
    QLabel* path_label_ = nullptr;
    /// @brief 路径面包屑容器。
    QWidget* breadcrumb_row_ = nullptr;
    /// @brief 路径面包屑布局。
    QHBoxLayout* breadcrumb_layout_ = nullptr;
    /// @brief 上下文摘要标签。
    QLabel* path_meta_label_ = nullptr;
    /// @brief 状态标签（底部元信息）。
    QLabel* status_label_ = nullptr;
    /// @brief 搜索输入框。
    QLineEdit* search_edit_ = nullptr;
    /// @brief 文件列表控件。
    QListWidget* file_list_ = nullptr;
    /// @brief 空状态提示标签。
    QLabel* empty_state_label_ = nullptr;
    /// @brief 返回按钮。
    QPushButton* back_btn_ = nullptr;
    /// @brief 上传按钮。
    QPushButton* upload_btn_ = nullptr;
    /// @brief 刷新按钮。
    QPushButton* refresh_btn_ = nullptr;
    /// @brief 新建按钮。
    QPushButton* create_btn_ = nullptr;
    /// @brief 传输进度行容器。
    QFrame* transfer_row_ = nullptr;
    /// @brief 传输标题。
    QLabel* transfer_label_ = nullptr;
    /// @brief 传输百分比标签。
    QLabel* transfer_percent_label_ = nullptr;
    /// @brief 传输进度条。
    QProgressBar* transfer_bar_ = nullptr;
    /// @brief 传输取消按钮。
    QPushButton* transfer_cancel_btn_ = nullptr;
    /// @brief 选中摘要标签。
    QLabel* selection_label_ = nullptr;
    /// @brief 选中元信息标签。
    QLabel* meta_label_ = nullptr;
    /// @brief 下载按钮。
    QPushButton* download_btn_ = nullptr;
    /// @brief 分享按钮。
    QPushButton* share_btn_ = nullptr;
    /// @brief 重命名按钮。
    QPushButton* rename_btn_ = nullptr;
    /// @brief 移动按钮。
    QPushButton* move_btn_ = nullptr;
    /// @brief 删除按钮。
    QPushButton* delete_btn_ = nullptr;
};
