// =============================================================
// common/include/common/crypto_utils.h
// 密码哈希工具（基于 OpenSSL SHA-256）
//
// 安全模型：
//   1. 注册时：服务端调用 hashPassword() 生成带随机 salt 的哈希存入数据库
//   2. 登录时：服务端调用 verifyPassword() 验证用户输入
//   3. 传输时：客户端调用 hashForTransport() 对密码做一次 SHA-256 后再发送，
//              避免明文密码在网络上传输（TLS 之外的额外一层保护）
//
// 存储格式：
//   "salt:hash"   其中 salt 是 32 位十六进制（16 随机字节），
//                 hash 是 SHA-256(salt + password) 的十六进制字符串
// =============================================================

#pragma once

#include <string>

namespace cloudvault::crypto {

// 注册时使用：生成随机 salt，返回 "salt:sha256(salt+password)"
// 每次调用生成不同的 salt，相同密码产生不同的存储哈希
std::string hashPassword(const std::string& password);

// 登录时使用：从 stored_hash 中提取 salt，重新计算并对比
// stored_hash 必须是 hashPassword() 返回的格式
bool verifyPassword(const std::string& password,
                    const std::string& stored_hash);

// 传输时使用（客户端调用）：对密码做一次 SHA-256
// 服务端存储的是 hashPassword(hashForTransport(password)) 的结果
std::string hashForTransport(const std::string& password);

} // namespace cloudvault::crypto
