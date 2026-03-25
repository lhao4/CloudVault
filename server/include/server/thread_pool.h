// =============================================================
// server/include/server/thread_pool.h
// 固定大小工作线程池
//
// 职责：IO 线程将 PDU 封装为任务 enqueue() 给线程池，
//       工作线程异步执行业务逻辑
// =============================================================

#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace cloudvault {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    // 提交任务（线程安全）
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread>           workers_;
    std::queue<std::function<void()>>  tasks_;
    std::mutex                         mutex_;
    std::condition_variable            cv_;
    bool                               stop_{false};
};

} // namespace cloudvault
