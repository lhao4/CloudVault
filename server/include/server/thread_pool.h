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

/**
 * @brief 固定大小工作线程池。
 *
 * IO 线程将解析后的业务任务投递到线程池，
 * 由工作线程并发执行业务逻辑。
 */
class ThreadPool {
public:
    /**
     * @brief 构造线程池并启动工作线程。
     * @param num_threads 线程数量。
     */
    explicit ThreadPool(size_t num_threads);
    /**
     * @brief 析构线程池并等待工作线程退出。
     */
    ~ThreadPool();

    /**
     * @brief 线程安全提交任务。
     * @param task 待执行任务。
     */
    void enqueue(std::function<void()> task);

private:
    /// @brief 工作线程集合。
    std::vector<std::thread>           workers_;
    /// @brief 任务队列。
    std::queue<std::function<void()>>  tasks_;
    /// @brief 队列互斥锁。
    std::mutex                         mutex_;
    /// @brief 条件变量，通知工作线程有新任务。
    std::condition_variable            cv_;
    /// @brief 停止标志。
    bool                               stop_{false};
};

} // namespace cloudvault
