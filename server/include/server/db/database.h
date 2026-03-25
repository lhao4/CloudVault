// =============================================================
// server/include/server/db/database.h
// TODO（第八章）：MySQL 连接池
//
// 职责：
//   - 启动时创建固定数量的 MySQL 连接（RAII 管理）
//   - 提供 Connection Guard：析构时自动归还连接到池
//   - 定期 mysql_ping() 检测连接活性，自动重连
//   - 所有 Repository 通过 Database::acquire() 获取连接
// =============================================================

#pragma once

// TODO（第八章）：实现 Database 类
