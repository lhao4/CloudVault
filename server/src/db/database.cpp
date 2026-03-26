// =============================================================
// server/src/db/database.cpp
// MySQL 连接池实现
// =============================================================

#include "server/db/database.h"

#include <stdexcept>
#include <spdlog/spdlog.h>

namespace cloudvault {

// =============================================================
// 构造 / 析构
// =============================================================
Database::Database(Config cfg) : cfg_(std::move(cfg)) {
    // mysql_library_init 在多线程程序中必须在任何其他 MySQL 调用前执行
    mysql_library_init(0, nullptr, nullptr);

    // 预先建立连接池
    for (int i = 0; i < cfg_.pool_size; ++i) {
        MYSQL* conn = createConnection();
        all_connections_.push_back(conn);
        available_.push(conn);
    }

    spdlog::info("Database pool: {} connections to {}:{}/{}",
                 cfg_.pool_size, cfg_.host, cfg_.port, cfg_.db_name);
}

Database::~Database() {
    for (MYSQL* conn : all_connections_) {
        mysql_close(conn);
    }
    mysql_library_end();
}

// =============================================================
// createConnection()：创建并初始化一个 MySQL 连接
// =============================================================
MYSQL* Database::createConnection() {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        throw std::runtime_error("mysql_init() failed: out of memory");
    }

    // 设置字符集为 utf8mb4（支持 emoji）
    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    // 启用自动重连（短暂断线后自动恢复）
    bool reconnect = true;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(conn,
                            cfg_.host.c_str(),
                            cfg_.user.c_str(),
                            cfg_.password.c_str(),
                            cfg_.db_name.c_str(),
                            static_cast<unsigned int>(cfg_.port),
                            nullptr,    // unix socket
                            0)) {       // client flags
        std::string err = mysql_error(conn);
        mysql_close(conn);
        throw std::runtime_error("mysql_real_connect() failed: " + err);
    }

    return conn;
}

// =============================================================
// acquire()：从池中借出一个连接（阻塞等待）
// =============================================================
Database::Connection Database::acquire() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return !available_.empty(); });

    MYSQL* conn = available_.front();
    available_.pop();
    return Connection(conn, this);
}

// =============================================================
// returnConnection()：归还连接到池（由 Connection 析构调用）
// =============================================================
void Database::returnConnection(MYSQL* conn) {
    {
        std::lock_guard lock(mutex_);
        available_.push(conn);
    }
    cv_.notify_one();
}

// =============================================================
// ping()：检测所有连接活性（可定期调用，如每 60 秒一次）
// =============================================================
void Database::ping() {
    // 只 ping 当前不在使用中的连接
    std::lock_guard lock(mutex_);
    std::queue<MYSQL*> temp;
    while (!available_.empty()) {
        MYSQL* conn = available_.front();
        available_.pop();
        if (mysql_ping(conn) != 0) {
            spdlog::warn("Database: connection lost, reconnecting...");
            mysql_close(conn);
            try {
                conn = createConnection();
            } catch (const std::exception& e) {
                spdlog::error("Database: reconnect failed: {}", e.what());
                // 将空指针放回，下次 acquire 时会失败——比直接崩溃更安全
            }
        }
        temp.push(conn);
    }
    available_ = std::move(temp);
}

// =============================================================
// Connection RAII 守卫
// =============================================================
Database::Connection::Connection(MYSQL* conn, Database* pool)
    : conn_(conn), pool_(pool) {}

Database::Connection::Connection(Connection&& other) noexcept
    : conn_(other.conn_), pool_(other.pool_) {
    other.conn_ = nullptr;
    other.pool_ = nullptr;
}

Database::Connection& Database::Connection::operator=(Connection&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (conn_ && pool_) {
        pool_->returnConnection(conn_);
    }

    conn_ = other.conn_;
    pool_ = other.pool_;
    other.conn_ = nullptr;
    other.pool_ = nullptr;
    return *this;
}

Database::Connection::~Connection() {
    if (conn_ && pool_) {
        pool_->returnConnection(conn_);
    }
}

} // namespace cloudvault
