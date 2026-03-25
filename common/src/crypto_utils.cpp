// =============================================================
// common/src/crypto_utils.cpp
// 密码哈希工具实现（基于 OpenSSL EVP SHA-256）
// =============================================================

#include "common/crypto_utils.h"

#include <openssl/evp.h>    // EVP_MD_CTX，高层摘要接口
#include <openssl/rand.h>   // RAND_bytes，密码学安全随机数

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace cloudvault::crypto {

// ── 内部工具函数 ──────────────────────────────────────────
namespace {

// 将字节数组转换为十六进制字符串（小写）
// 例：{0xDE, 0xAD} → "dead"
std::string toHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(data[i]);
    }
    return oss.str();
}

// 对任意字节序列计算 SHA-256，返回十六进制字符串（64 字符）
// 使用 EVP 接口（OpenSSL 推荐方式，兼容 OpenSSL 1.x 和 3.x）
std::string sha256Hex(const std::string& input) {
    // EVP_MD_CTX：摘要上下文，持有算法状态
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digestLen = 0;

    // 初始化 → 喂入数据 → 生成摘要
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx,
        reinterpret_cast<const unsigned char*>(input.data()),
        input.size());
    EVP_DigestFinal_ex(ctx, digest, &digestLen);

    EVP_MD_CTX_free(ctx);  // 必须释放，否则内存泄漏

    return toHex(digest, digestLen);
}

// 生成 16 字节密码学安全随机数，返回 32 字符十六进制 salt
std::string generateSalt() {
    unsigned char buf[16];
    // RAND_bytes：OpenSSL 的密码学安全随机数生成器
    // 返回 1 表示成功，0 或 -1 表示失败
    if (RAND_bytes(buf, static_cast<int>(sizeof(buf))) != 1) {
        throw std::runtime_error("RAND_bytes failed: cannot generate salt");
    }
    return toHex(buf, sizeof(buf));
}

} // namespace

// =============================================================
// 公开接口实现
// =============================================================

std::string hashPassword(const std::string& password) {
    // 1. 生成随机 salt（每次注册不同，防止彩虹表攻击）
    const std::string salt = generateSalt();
    // 2. 计算 SHA-256(salt + password)
    const std::string hash = sha256Hex(salt + password);
    // 3. 返回 "salt:hash" 格式，两部分都存入数据库同一字段
    return salt + ":" + hash;
}

bool verifyPassword(const std::string& password,
                    const std::string& stored_hash) {
    // 从存储哈希中提取 salt（冒号前的部分）
    const auto sep = stored_hash.find(':');
    if (sep == std::string::npos) {
        return false;  // 格式不合法
    }

    const std::string salt     = stored_hash.substr(0, sep);
    const std::string expected = stored_hash.substr(sep + 1);

    // 用相同的 salt 对输入密码重新计算哈希，与存储值对比
    const std::string actual = sha256Hex(salt + password);

    // 逐字符比较（注意：生产环境应使用 CRYPTO_memcmp 防时序攻击）
    return actual == expected;
}

std::string hashForTransport(const std::string& password) {
    // 客户端发送前对密码做一次 SHA-256
    // 服务端存储的是 hashPassword(hashForTransport(password)) 的结果
    return sha256Hex(password);
}

} // namespace cloudvault::crypto
