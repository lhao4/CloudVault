// =============================================================
// server/include/server/event_loop.h
// epoll 事件循环封装
//
// 线程模型：
//   主线程（IO 线程）：run() → epoll_wait → 回调
//   工作线程：runInLoop(task) → 写 eventfd → 唤醒主线程 → 主线程执行 task
// =============================================================

#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace cloudvault {

/**
 * @brief epoll 事件循环封装。
 *
 * 负责：
 * 1. 管理 fd 与事件回调关系；
 * 2. 在 IO 线程执行 epoll_wait 主循环；
 * 3. 通过 eventfd 支持跨线程任务投递。
 */
class EventLoop {
public:
    /**
     * @brief fd 事件回调签名。
     */
    using EventCallback = std::function<void(uint32_t events)>;

    /**
     * @brief 构造事件循环对象。
     */
    EventLoop();
    /**
     * @brief 析构事件循环对象并释放系统资源。
     */
    ~EventLoop();

    /**
     * @brief 注册 fd 监听并绑定回调。
     * @param fd 文件描述符。
     * @param events epoll 事件掩码。
     * @param cb 回调函数。
     */
    void addFd(int fd, uint32_t events, EventCallback cb);
    /**
     * @brief 修改 fd 监听事件。
     * @param fd 文件描述符。
     * @param events 新事件掩码。
     */
    void modFd(int fd, uint32_t events);
    /**
     * @brief 注销 fd 监听。
     * @param fd 文件描述符。
     */
    void removeFd(int fd);

    /**
     * @brief 线程安全地向 IO 线程投递任务。
     * @param task 待执行任务。
     */
    void runInLoop(std::function<void()> task);

    /**
     * @brief 启动并阻塞运行事件循环。
     */
    void run();

    /**
     * @brief 请求停止事件循环。
     */
    void stop();

private:
    /**
     * @brief 写入 eventfd 触发唤醒。
     */
    void wakeup();
    /**
     * @brief 处理 eventfd 可读事件并执行挂起任务。
     */
    void handleWakeup();

    /// @brief epoll 实例 fd。
    int epoll_fd_{-1};
    /// @brief eventfd，用于跨线程唤醒 IO 线程。
    int wakeup_fd_{-1};
    /// @brief 事件循环运行标记。
    std::atomic<bool> running_{false};

    /// @brief fd 回调映射表。
    std::unordered_map<int, EventCallback> callbacks_;

    /// @brief 挂起任务互斥锁。
    std::mutex                         tasks_mutex_;
    /// @brief 等待在 IO 线程执行的任务队列。
    std::vector<std::function<void()>> pending_tasks_;
};

} // namespace cloudvault
