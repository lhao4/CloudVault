// =============================================================
// server/include/server/thread_pool.h
// TODO（第七章）：固定大小工作线程池
//
// 职责：
//   - 创建 N 个工作线程（数量由 config/thread_count 决定）
//   - 提供线程安全的任务队列（std::function<void()>）
//   - 主线程将解析好的 PDU 封装为任务 enqueue() 给线程池
//   - 工作线程从队列取任务，调用 MessageDispatcher 分发处理
// =============================================================

#pragma once

// TODO（第七章）：实现 ThreadPool 类
