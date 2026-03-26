// =============================================================
// server/src/file_storage.cpp
// 第十一章：文件系统操作封装
// =============================================================

#include "server/file_storage.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace cloudvault {

namespace {

bool pathStartsWith(const std::filesystem::path& root,
                    const std::filesystem::path& candidate) {
    auto root_it = root.begin();
    auto candidate_it = candidate.begin();
    for (; root_it != root.end(); ++root_it, ++candidate_it) {
        if (candidate_it == candidate.end() || *root_it != *candidate_it) {
            return false;
        }
    }
    return true;
}

std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

} // namespace

FileStorage::FileStorage(std::filesystem::path storage_root)
    : storage_root_(std::move(storage_root)) {
    if (storage_root_.empty()) {
        storage_root_ = std::filesystem::path("./filesys");
    }
    std::filesystem::create_directories(storage_root_);
}

void FileStorage::ensureUserRoot(const std::string& username) const {
    std::filesystem::create_directories(userRoot(username));
}

std::vector<FileStorage::Entry> FileStorage::list(const std::string& username,
                                                  const std::string& dir_path) const {
    const auto dir = resolvePath(username, dir_path);
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        throw std::runtime_error("目录不存在");
    }

    const auto root = std::filesystem::weakly_canonical(userRoot(username));
    std::vector<Entry> entries;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        entries.push_back(buildEntry(entry, root));
    }

    std::sort(entries.begin(), entries.end(),
              [](const Entry& lhs, const Entry& rhs) {
                  if (lhs.is_dir != rhs.is_dir) {
                      return lhs.is_dir > rhs.is_dir;
                  }
                  return lowerCopy(lhs.name) < lowerCopy(rhs.name);
              });
    return entries;
}

void FileStorage::createDirectory(const std::string& username,
                                  const std::string& parent_path,
                                  const std::string& dir_name) const {
    validateName(dir_name);
    const auto parent = resolvePath(username, parent_path);
    if (!std::filesystem::exists(parent) || !std::filesystem::is_directory(parent)) {
        throw std::runtime_error("父目录不存在");
    }

    const auto target = parent / dir_name;
    if (std::filesystem::exists(target)) {
        throw std::runtime_error("目标已存在");
    }

    std::filesystem::create_directory(target);
}

void FileStorage::renamePath(const std::string& username,
                             const std::string& target_path,
                             const std::string& new_name) const {
    validateName(new_name);
    const auto root = std::filesystem::weakly_canonical(userRoot(username));
    const auto target = resolvePath(username, target_path);
    if (target == root) {
        throw std::runtime_error("根目录不允许重命名");
    }

    const auto renamed = target.parent_path() / new_name;
    ensureInsideRoot(root, renamed.lexically_normal());
    if (std::filesystem::exists(renamed)) {
        throw std::runtime_error("新名称已存在");
    }

    std::filesystem::rename(target, renamed);
}

void FileStorage::movePath(const std::string& username,
                           const std::string& source_path,
                           const std::string& destination_path) const {
    const auto root = std::filesystem::weakly_canonical(userRoot(username));
    const auto source = resolvePath(username, source_path);
    if (source == root) {
        throw std::runtime_error("根目录不允许移动");
    }

    auto destination = resolvePath(username, destination_path, true);
    if (std::filesystem::exists(destination) && std::filesystem::is_directory(destination)) {
        destination /= source.filename();
    }

    const auto normalized = destination.lexically_normal();
    ensureInsideRoot(root, normalized);

    if (!std::filesystem::exists(normalized.parent_path())) {
        throw std::runtime_error("目标目录不存在");
    }
    if (std::filesystem::exists(normalized)) {
        throw std::runtime_error("目标已存在");
    }
    if (std::filesystem::is_directory(source) && pathStartsWith(source, normalized)) {
        throw std::runtime_error("不能移动到自己的子目录");
    }

    std::filesystem::rename(source, normalized);
}

void FileStorage::deletePath(const std::string& username,
                             const std::string& target_path) const {
    const auto root = std::filesystem::weakly_canonical(userRoot(username));
    const auto target = resolvePath(username, target_path);
    if (target == root) {
        throw std::runtime_error("根目录不允许删除");
    }
    if (!std::filesystem::exists(target)) {
        throw std::runtime_error("目标不存在");
    }

    std::filesystem::remove_all(target);
}

std::vector<FileStorage::Entry> FileStorage::search(const std::string& username,
                                                    const std::string& keyword) const {
    const std::string trimmed = normalizeLogicalPath(keyword);
    if (trimmed.empty()) {
        throw std::runtime_error("搜索关键词不能为空");
    }

    const auto root = std::filesystem::weakly_canonical(userRoot(username));
    std::vector<Entry> results;
    const auto lowered_keyword = lowerCopy(trimmed);
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        const auto filename = entry.path().filename().string();
        if (lowerCopy(filename).find(lowered_keyword) == std::string::npos) {
            continue;
        }
        results.push_back(buildEntry(entry, root));
    }

    std::sort(results.begin(), results.end(),
              [](const Entry& lhs, const Entry& rhs) {
                  if (lhs.is_dir != rhs.is_dir) {
                      return lhs.is_dir > rhs.is_dir;
                  }
                  return lowerCopy(lhs.path) < lowerCopy(rhs.path);
              });
    return results;
}

FileStorage::FileInfo FileStorage::inspectPath(const std::string& username,
                                               const std::string& target_path) const {
    const auto root = std::filesystem::weakly_canonical(userRoot(username));
    const auto absolute = resolvePath(username, target_path);

    FileInfo info;
    info.name = absolute.filename().string();
    info.path = toRelativePath(root, absolute);
    info.absolute_path = absolute;
    info.is_dir = std::filesystem::is_directory(absolute);
    if (!info.is_dir) {
        std::error_code ec;
        info.size = std::filesystem::file_size(absolute, ec);
        if (ec) {
            info.size = 0;
        }
    }
    return info;
}

std::string FileStorage::copyFileToUser(const std::string& owner_username,
                                        const std::string& source_path,
                                        const std::string& receiver_username) const {
    const auto source = inspectPath(owner_username, source_path);
    if (source.is_dir) {
        throw std::runtime_error("当前只支持分享普通文件");
    }

    ensureUserRoot(receiver_username);
    const auto receiver_root = std::filesystem::weakly_canonical(userRoot(receiver_username));
    const auto target = (receiver_root / source.name).lexically_normal();
    ensureInsideRoot(receiver_root, target);

    if (std::filesystem::exists(target)) {
        throw std::runtime_error("接收方根目录已存在同名文件");
    }

    std::error_code ec;
    std::filesystem::copy_file(source.absolute_path, target, ec);
    if (ec) {
        throw std::runtime_error("复制文件失败");
    }

    return toRelativePath(receiver_root, target);
}

std::filesystem::path FileStorage::userRoot(const std::string& username) const {
    return storage_root_ / username;
}

std::filesystem::path FileStorage::resolvePath(const std::string& username,
                                               const std::string& relative_path,
                                               bool allow_missing_leaf) const {
    ensureUserRoot(username);

    const auto root = std::filesystem::weakly_canonical(userRoot(username));
    const auto logical = normalizeLogicalPath(relative_path);
    const auto candidate = (logical.empty() ? root
                                            : (root / std::filesystem::path(logical)).lexically_normal());
    ensureInsideRoot(root, candidate);

    if (!allow_missing_leaf && !std::filesystem::exists(candidate)) {
        throw std::runtime_error("路径不存在");
    }
    return candidate;
}

std::string FileStorage::toRelativePath(const std::filesystem::path& username_root,
                                        const std::filesystem::path& absolute_path) const {
    const auto relative = absolute_path.lexically_relative(username_root).generic_string();
    if (relative.empty() || relative == ".") {
        return "/";
    }
    return "/" + relative;
}

FileStorage::Entry FileStorage::buildEntry(const std::filesystem::directory_entry& entry,
                                           const std::filesystem::path& username_root) const {
    Entry result;
    result.name = entry.path().filename().string();
    result.path = toRelativePath(username_root, entry.path());
    result.is_dir = entry.is_directory();
    result.modified_at = formatTimestamp(entry.last_write_time());
    if (!result.is_dir) {
        std::error_code ec;
        result.size = entry.file_size(ec);
        if (ec) {
            result.size = 0;
        }
    }
    return result;
}

std::string FileStorage::normalizeLogicalPath(const std::string& path) {
    std::string result = path;
    while (!result.empty() && (result.front() == '/' || result.front() == '\\')) {
        result.erase(result.begin());
    }
    if (result == ".") {
        result.clear();
    }
    return std::filesystem::path(result).lexically_normal().generic_string();
}

std::string FileStorage::formatTimestamp(const std::filesystem::file_time_type& tp) {
    using namespace std::chrono;
    const auto system_tp = time_point_cast<system_clock::duration>(
        tp - std::filesystem::file_time_type::clock::now() + system_clock::now());
    const std::time_t time_value = system_clock::to_time_t(system_tp);
    std::tm tm_value{};
#ifdef _WIN32
    localtime_s(&tm_value, &time_value);
#else
    localtime_r(&time_value, &tm_value);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_value, "%Y-%m-%d %H:%M");
    return oss.str();
}

void FileStorage::validateName(const std::string& name) {
    if (name.empty() || name == "." || name == "..") {
        throw std::runtime_error("名称不合法");
    }
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
        throw std::runtime_error("名称中不能包含路径分隔符");
    }
}

void FileStorage::ensureInsideRoot(const std::filesystem::path& root,
                                   const std::filesystem::path& candidate) {
    if (!pathStartsWith(root, candidate)) {
        throw std::runtime_error("路径越权");
    }
}

} // namespace cloudvault
