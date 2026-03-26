// =============================================================
// server/include/server/db/database.h
// MySQL 连接池（RAII 设计）
//
// 用法：
//   auto conn = db.acquire();   // 从池中借出连接
//   mysql_query(conn.get(), "SELECT 1");
//   // conn 析构时自动归还到池
// =============================================================

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <mysql/mysql.h>

namespace cloudvault {

class Database {
public:
    struct Config {
        std::string host;
        int         port     = 3306;
        std::string db_name;
        std::string user;
        std::string password;
        int         pool_size = 8;
    };

    explicit Database(Config cfg);
    ~Database();

    // 禁止拷贝
    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // ── RAII 连接守卫 ─────────────────────────────────────
    // 析构时自动将 MYSQL* 归还连接池，无需手动 release()
    class Connection {
    public:
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&& other) noexcept;
        Connection& operator=(Connection&& other) noexcept;
        ~Connection();

        MYSQL* get() const { return conn_; }

        // 支持 if (conn) 判断是否有效
        explicit operator bool() const { return conn_ != nullptr; }

    private:
        friend class Database;
        Connection(MYSQL* conn, Database* pool);

        MYSQL*    conn_;
        Database* pool_;
    };

    // 从池中借出一个连接（阻塞直到有空闲连接）
    Connection acquire();

    // 检查所有连接的活性，重建断开的连接（可定期调用）
    void ping();

private:
    MYSQL* createConnection();
    void   returnConnection(MYSQL* conn);

    Config                cfg_;
    std::vector<MYSQL*>   all_connections_;   // 所有连接（用于 ping 和析构）
    std::queue<MYSQL*>    available_;
    std::mutex            mutex_;
    std::condition_variable cv_;
};

} // namespace cloudvault
