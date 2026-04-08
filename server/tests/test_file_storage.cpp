// =============================================================
// server/tests/test_file_storage.cpp
// FileStorage 单元测试
// =============================================================

#include "server/file_storage.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace cloudvault;

TEST(FileStorage, DirectoryCrudAndSearchLifecycle) {
    test_helpers::TempDir temp_dir;
    FileStorage storage(temp_dir.path);

    storage.createDirectory("alice", "/", "docs");
    storage.createDirectory("alice", "/docs", "drafts");

    const auto root_entries = storage.list("alice", "/");
    ASSERT_EQ(root_entries.size(), 1u);
    EXPECT_EQ(root_entries.front().name, "docs");
    EXPECT_TRUE(root_entries.front().is_dir);

    storage.renamePath("alice", "/docs", "notes");
    auto renamed_entries = storage.list("alice", "/");
    ASSERT_EQ(renamed_entries.size(), 1u);
    EXPECT_EQ(renamed_entries.front().path, "/notes");

    storage.movePath("alice", "/notes/drafts", "/");
    const auto moved_root_entries = storage.list("alice", "/");
    ASSERT_EQ(moved_root_entries.size(), 2u);

    const auto search_results = storage.search("alice", "draft");
    ASSERT_EQ(search_results.size(), 1u);
    EXPECT_EQ(search_results.front().path, "/drafts");

    storage.deletePath("alice", "/drafts");
    const auto final_entries = storage.list("alice", "/");
    ASSERT_EQ(final_entries.size(), 1u);
    EXPECT_EQ(final_entries.front().path, "/notes");
}

TEST(FileStorage, PrepareUploadInspectAndCopyBetweenUsers) {
    test_helpers::TempDir temp_dir;
    FileStorage storage(temp_dir.path);

    storage.createDirectory("alice", "/", "uploads");
    const auto upload_target = storage.prepareUploadTarget("alice", "/uploads", "report.txt");
    EXPECT_EQ(upload_target.path, "/uploads/report.txt");
    EXPECT_EQ(upload_target.name, "report.txt");

    std::ofstream output(upload_target.absolute_path, std::ios::binary);
    ASSERT_TRUE(output.is_open());
    output << "hello cloudvault";
    output.close();

    const auto info = storage.inspectPath("alice", "/uploads/report.txt");
    EXPECT_FALSE(info.is_dir);
    EXPECT_EQ(info.name, "report.txt");
    EXPECT_GT(info.size, 0u);

    const std::string copied_path =
        storage.copyFileToUser("alice", "/uploads/report.txt", "bob");
    EXPECT_EQ(copied_path, "/report.txt");

    const auto copied_info = storage.inspectPath("bob", "/report.txt");
    EXPECT_FALSE(copied_info.is_dir);
    EXPECT_EQ(copied_info.name, "report.txt");
    EXPECT_EQ(copied_info.size, info.size);
}

TEST(FileStorage, RejectsRootDeletionAndPathEscape) {
    test_helpers::TempDir temp_dir;
    FileStorage storage(temp_dir.path);

    EXPECT_THROW(storage.deletePath("alice", "/"), std::runtime_error);
    EXPECT_THROW(storage.list("alice", "/../../outside"), std::runtime_error);
    EXPECT_THROW(storage.createDirectory("alice", "/", "../bad"), std::runtime_error);
}
