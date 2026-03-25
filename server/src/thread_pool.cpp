// =============================================================
// server/src/thread_pool.cpp
// 固定大小工作线程池实现
// =============================================================

#include "server/thread_pool.h"

namespace cloudvault {

ThreadPool::ThreadPool(size_t num_threads) {
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] {
            // 每个工作线程：不断从任务队列取任务执行
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock lock(mutex_);
                    // 等待条件：有任务 或 线程池停止
                    cv_.wait(lock, [this] {
                        return stop_ || !tasks_.empty();
                    });

                    if (stop_ && tasks_.empty()) {
                        return;  // 优雅退出：先处理完所有剩余任务
                    }
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();  // 唤醒所有工作线程，让它们检测到 stop_ = true
    for (auto& w : workers_) {
        w.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();  // 通知一个空闲工作线程
}

} // namespace cloudvault
