// =============================================================
// client/src/ui/sidebar_panel.h
// 左侧联系人面板
// =============================================================

#pragma once

#include <QFrame>
#include <QList>

class QLineEdit;
class QListWidget;
class QPushButton;

enum class SidebarMode {
    Message,
    Contact,
    File,
};

enum class SidebarItemType {
    Section,
    Action,
    Friend,
    Group,
    FileContext,
};

/**
 * @brief 左侧上下文侧栏条目数据模型。
 */
struct SidebarEntry {
    /// @brief 条目唯一键。
    QString key;
    /// @brief 主标题文本。
    QString title;
    /// @brief 副标题/预览文本。
    QString preview;
    /// @brief 时间显示文本。
    QString time;
    /// @brief 未读数量。
    int unread = 0;
    /// @brief 在线状态。
    bool online = false;
    /// @brief 条目类型。
    SidebarItemType type = SidebarItemType::Friend;
    /// @brief 关联整型标识（群组 ID 等）。
    int id = 0;
    /// @brief 附加高亮文案。
    QString tag;
};

/**
 * @brief 左侧上下文侧栏面板。
 *
 * 负责顶部标题与操作按钮、搜索框、上下文列表渲染，
 * 并将用户交互通过信号传递给 MainWindow。
 */
class SidebarPanel : public QFrame {
    Q_OBJECT

public:
    /**
     * @brief 构造侧栏面板。
     * @param parent Qt 父对象。
     */
    explicit SidebarPanel(QWidget* parent = nullptr);

    /**
     * @brief 设置顶部栏文案和操作按钮显隐。
     * @param title 标题文本。
     * @param placeholder 搜索框占位文本。
     * @param show_action 是否展示右上角动作按钮。
     */
    void setHeader(const QString& title,
                   const QString& placeholder,
                   bool show_action);
    /**
     * @brief 设置当前侧栏模式。
     * @param mode 侧栏模式。
     */
    void setMode(SidebarMode mode);
    /**
     * @brief 重建侧栏列表并恢复选中态。
     * @param entries 条目列表。
     * @param selected_key 期望选中的唯一键。
     * @param auto_select_first 若未命中选中项是否自动选第一项。
     */
    void populateEntries(const QList<SidebarEntry>& entries,
                          const QString& selected_key,
                          bool auto_select_first);
    /**
     * @brief 刷新列表项高亮样式。
     */
    void refreshSelectionHighlights();
    /**
     * @brief 获取搜索框文本。
     * @return 搜索词。
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
     * @brief 获取当前条目数量。
     * @return 条目数。
     */
    int itemCount() const;
    /**
     * @brief 获取当前选中条目的唯一键。
     * @return 条目 key。
     */
    QString currentKey() const;
    /**
     * @brief 获取当前选中条目的主标题。
     * @return 标题文本。
     */
    QString currentTitle() const;
    /**
     * @brief 获取当前选中条目的类型。
     * @return 条目类型。
     */
    SidebarItemType currentItemType() const;
    /**
     * @brief 获取当前选中条目的关联 ID。
     * @return 整型标识。
     */
    int currentItemId() const;
    /**
     * @brief 获取当前选中条目的在线状态。
     * @return true 表示在线。
     */
    bool currentOnline() const;
    /**
     * @brief 获取当前侧栏模式。
     * @return 侧栏模式。
     */
    SidebarMode mode() const;

signals:
    /// @brief 搜索文本变化。
    void searchTextChanged(const QString& text);
    /// @brief 联系人选中变化。
    void contactSelectionChanged();
    /// @brief 顶部动作按钮点击。
    void actionRequested();

private:
    /// @brief 顶部标题标签。
    class QLabel* title_label_ = nullptr;
    /// @brief 搜索输入框。
    QLineEdit* search_edit_ = nullptr;
    /// @brief 联系人列表控件。
    QListWidget* contact_list_ = nullptr;
    /// @brief 顶部动作按钮。
    QPushButton* action_btn_ = nullptr;
    /// @brief 当前侧栏模式。
    SidebarMode mode_ = SidebarMode::Message;
};
