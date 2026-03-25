// =============================================================
// common/tests/test_crypto.cpp
// crypto_utils 单元测试
// =============================================================

#include "common/crypto_utils.h"

#include <gtest/gtest.h>

using namespace cloudvault::crypto;

// hashForTransport 是纯函数：相同输入产生相同输出
TEST(Crypto, HashForTransportDeterministic) {
    const std::string h1 = hashForTransport("my_password");
    const std::string h2 = hashForTransport("my_password");
    EXPECT_EQ(h1, h2);
}

// SHA-256 十六进制输出应为 64 字符
TEST(Crypto, HashForTransportLength) {
    EXPECT_EQ(hashForTransport("any_password").size(), 64u);
}

// 不同密码的哈希值不同
TEST(Crypto, HashForTransportDifferentInputs) {
    EXPECT_NE(hashForTransport("password1"),
              hashForTransport("password2"));
}

// hashPassword 输出应包含 ':'（salt:hash 格式）
TEST(Crypto, HashPasswordFormat) {
    const std::string stored = hashPassword("secret");
    EXPECT_NE(stored.find(':'), std::string::npos);
}

// 由于随机 salt，相同密码的两次哈希结果不同
TEST(Crypto, HashPasswordDifferentEachCall) {
    const std::string h1 = hashPassword("secret");
    const std::string h2 = hashPassword("secret");
    EXPECT_NE(h1, h2);
}

// 正确密码验证通过
TEST(Crypto, VerifyCorrectPassword) {
    const std::string stored = hashPassword("correct_password");
    EXPECT_TRUE(verifyPassword("correct_password", stored));
}

// 错误密码验证失败
TEST(Crypto, VerifyWrongPassword) {
    const std::string stored = hashPassword("correct_password");
    EXPECT_FALSE(verifyPassword("wrong_password", stored));
}

// 空密码也能正常哈希和验证
TEST(Crypto, VerifyEmptyPassword) {
    const std::string stored = hashPassword("");
    EXPECT_TRUE(verifyPassword("", stored));
    EXPECT_FALSE(verifyPassword("not_empty", stored));
}

// 格式非法的 stored_hash 应返回 false 而不是崩溃
TEST(Crypto, VerifyInvalidFormat) {
    EXPECT_FALSE(verifyPassword("any", "no_colon_here"));
    EXPECT_FALSE(verifyPassword("any", ""));
}
