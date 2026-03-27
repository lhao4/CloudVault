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

/**
 * @brief MySQL 连接池。
 *
 * 提供固定大小连接池和 RAII 连接守卫，
 * 支持阻塞借出、自动归还和连接活性探测。
 */
class Database {
public:
    /**
     * @brief 数据库配置。
     */
    struct Config {
        /// @brief 数据库主机地址。
        std::string host;
        /// @brief 数据库端口。
        int         port     = 3306;
        /// @brief 数据库名。
        std::string db_name;
        /// @brief 用户名。
        std::string user;
        /// @brief 密码。
        std::string password;
        /// @brief 连接池大小。
        int         pool_size = 8;
    };

    /**
     * @brief 构造数据库连接池并创建连接。
     * @param cfg 配置参数。
     */
    explicit Database(Config cfg);
    /**
     * @brief 析构连接池并关闭所有连接。
     */
    ~Database();

    // 禁止拷贝
    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    /**
     * @brief RAII 连接守卫。
     *
     * 析构时自动将 MYSQL* 归还连接池。
     */
    class Connection {
    public:
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        /**
         * @brief 移动构造。
         * @param other 源对象。
         */
        Connection(Connection&& other) noexcept;
        /**
         * @brief 移动赋值。
         * @param other 源对象。
         * @return 自身引用。
         */
        Connection& operator=(Connection&& other) noexcept;
        /**
         * @brief 析构时自动归还连接。
         */
        ~Connection();

        /**
         * @brief 获取原生 MYSQL 连接指针。
         * @return MYSQL* 指针。
         */
        MYSQL* get() const { return conn_; }

        /**
         * @brief 判断连接是否有效。
         */
        explicit operator bool() const { return conn_ != nullptr; }

    private:
        friend class Database;
        /**
         * @brief 内部构造函数。
         * @param conn 原生连接指针。
         * @param pool 所属连接池。
         */
        Connection(MYSQL* conn, Database* pool);

        /// @brief 原生 MySQL 连接指针。
        MYSQL*    conn_;
        /// @brief 所属连接池指针。
        Database* pool_;
    };

    /**
     * @brief 从池中借出连接（阻塞直到可用）。
     * @return RAII 连接守卫。
     */
    Connection acquire();

    /**
     * @brief 探测并修复连接池中的断连连接。
     */
    void ping();

private:
    /// @brief 创建单个 MySQL 连接。
    MYSQL* createConnection();
    /// @brief 归还连接到可用队列。
    void   returnConnection(MYSQL* conn);

    /// @brief 连接池配置。
    Config                cfg_;
    /// @brief 所有连接集合（用于析构与 ping）。
    std::vector<MYSQL*>   all_connections_;
    /// @brief 可借出的连接队列。
    std::queue<MYSQL*>    available_;
    /// @brief 队列互斥锁。
    std::mutex            mutex_;
    /// @brief 可用连接条件变量。
    std::condition_variable cv_;
};

} // namespace cloudvault
