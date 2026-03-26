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

class FileStorage {
public:
    struct Entry {
        std::string name;
        std::string path;         // 相对用户根目录的路径，以 / 开头
        bool        is_dir = false;
        uint64_t    size = 0;
        std::string modified_at;
    };

    explicit FileStorage(std::filesystem::path storage_root);

    void ensureUserRoot(const std::string& username) const;

    std::vector<Entry> list(const std::string& username,
                            const std::string& dir_path) const;

    void createDirectory(const std::string& username,
                         const std::string& parent_path,
                         const std::string& dir_name) const;

    void renamePath(const std::string& username,
                    const std::string& target_path,
                    const std::string& new_name) const;

    void movePath(const std::string& username,
                  const std::string& source_path,
                  const std::string& destination_path) const;

    void deletePath(const std::string& username,
                    const std::string& target_path) const;

    std::vector<Entry> search(const std::string& username,
                              const std::string& keyword) const;

private:
    std::filesystem::path userRoot(const std::string& username) const;
    std::filesystem::path resolvePath(const std::string& username,
                                      const std::string& relative_path,
                                      bool allow_missing_leaf = false) const;
    std::string toRelativePath(const std::filesystem::path& username_root,
                               const std::filesystem::path& absolute_path) const;
    Entry buildEntry(const std::filesystem::directory_entry& entry,
                     const std::filesystem::path& username_root) const;

    static std::string normalizeLogicalPath(const std::string& path);
    static std::string formatTimestamp(const std::filesystem::file_time_type& tp);
    static void validateName(const std::string& name);
    static void ensureInsideRoot(const std::filesystem::path& root,
                                 const std::filesystem::path& candidate);

    std::filesystem::path storage_root_;
};

} // namespace cloudvault
