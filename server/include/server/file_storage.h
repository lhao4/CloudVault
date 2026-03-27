// =============================================================
// server/include/server/file_storage.h
// 第十一章：文件系统操作封装
// =============================================================

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace cloudvault {

/**
 * @brief 文件系统存储抽象。
 *
 * 将用户逻辑路径映射到服务器磁盘目录，封装目录浏览、
 * 增删改查、搜索、上传目标准备和跨用户复制等能力。
 */
class FileStorage {
public:
    /**
     * @brief 文件列表展示条目。
     */
    struct Entry {
        /// @brief 文件/目录名称。
        std::string name;
        /// @brief 相对用户根目录的逻辑路径（以 / 开头）。
        std::string path;
        /// @brief 是否目录。
        bool        is_dir = false;
        /// @brief 文件大小（字节）。
        uint64_t    size = 0;
        /// @brief 修改时间文本。
        std::string modified_at;
    };

    /**
     * @brief 路径检查结果。
     */
    struct FileInfo {
        /// @brief 文件/目录名称。
        std::string           name;
        /// @brief 逻辑路径。
        std::string           path;
        /// @brief 绝对磁盘路径。
        std::filesystem::path absolute_path;
        /// @brief 是否目录。
        bool                  is_dir = false;
        /// @brief 文件大小（字节）。
        uint64_t              size = 0;
    };

    /**
     * @brief 构造文件存储对象。
     * @param storage_root 存储根目录。
     */
    explicit FileStorage(std::filesystem::path storage_root);

    /**
     * @brief 确保用户根目录存在。
     * @param username 用户名。
     */
    void ensureUserRoot(const std::string& username) const;

    /**
     * @brief 列出目录内容。
     * @param username 用户名。
     * @param dir_path 目录逻辑路径。
     * @return 目录条目数组。
     */
    std::vector<Entry> list(const std::string& username,
                            const std::string& dir_path) const;

    /**
     * @brief 创建目录。
     * @param username 用户名。
     * @param parent_path 父目录逻辑路径。
     * @param dir_name 新目录名。
     */
    void createDirectory(const std::string& username,
                         const std::string& parent_path,
                         const std::string& dir_name) const;

    /**
     * @brief 重命名路径。
     * @param username 用户名。
     * @param target_path 目标逻辑路径。
     * @param new_name 新名称。
     */
    void renamePath(const std::string& username,
                    const std::string& target_path,
                    const std::string& new_name) const;

    /**
     * @brief 移动路径。
     * @param username 用户名。
     * @param source_path 源逻辑路径。
     * @param destination_path 目标目录逻辑路径。
     */
    void movePath(const std::string& username,
                  const std::string& source_path,
                  const std::string& destination_path) const;

    /**
     * @brief 删除路径。
     * @param username 用户名。
     * @param target_path 目标逻辑路径。
     */
    void deletePath(const std::string& username,
                    const std::string& target_path) const;

    /**
     * @brief 关键字搜索。
     * @param username 用户名。
     * @param keyword 搜索关键词。
     * @return 匹配条目数组。
     */
    std::vector<Entry> search(const std::string& username,
                              const std::string& keyword) const;

    /**
     * @brief 检查指定逻辑路径并返回元信息。
     * @param username 用户名。
     * @param target_path 目标逻辑路径。
     * @return 路径信息。
     */
    FileInfo inspectPath(const std::string& username,
                         const std::string& target_path) const;

    /**
     * @brief 为上传请求准备最终目标路径。
     * @param username 用户名。
     * @param dir_path 目标目录逻辑路径。
     * @param filename 文件名。
     * @return 目标文件信息。
     */
    FileInfo prepareUploadTarget(const std::string& username,
                                 const std::string& dir_path,
                                 const std::string& filename) const;

    /**
     * @brief 将文件复制到接收用户目录。
     * @param owner_username 文件拥有者用户名。
     * @param source_path 源逻辑路径。
     * @param receiver_username 接收者用户名。
     * @return 复制后目标逻辑路径。
     */
    std::string copyFileToUser(const std::string& owner_username,
                               const std::string& source_path,
                               const std::string& receiver_username) const;

private:
    /// @brief 计算用户根目录绝对路径。
    std::filesystem::path userRoot(const std::string& username) const;
    /// @brief 解析逻辑路径为绝对路径并校验越界。
    std::filesystem::path resolvePath(const std::string& username,
                                      const std::string& relative_path,
                                      bool allow_missing_leaf = false) const;
    /// @brief 绝对路径转换为用户逻辑路径。
    std::string toRelativePath(const std::filesystem::path& username_root,
                               const std::filesystem::path& absolute_path) const;
    /// @brief 构建目录项展示结构。
    Entry buildEntry(const std::filesystem::directory_entry& entry,
                     const std::filesystem::path& username_root) const;

    /// @brief 规范化逻辑路径。
    static std::string normalizeLogicalPath(const std::string& path);
    /// @brief 格式化文件时间。
    static std::string formatTimestamp(const std::filesystem::file_time_type& tp);
    /// @brief 校验文件名合法性。
    static void validateName(const std::string& name);
    /// @brief 校验路径是否仍在用户根目录内。
    static void ensureInsideRoot(const std::filesystem::path& root,
                                 const std::filesystem::path& candidate);

    /// @brief 文件系统存储根目录。
    std::filesystem::path storage_root_;
};

} // namespace cloudvault
