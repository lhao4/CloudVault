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

class EventLoop {
public:
    using EventCallback = std::function<void(uint32_t events)>;

    EventLoop();
    ~EventLoop();

    // 注册 / 修改 / 注销 fd 的 epoll 监听（仅在 IO 线程调用）
    void addFd(int fd, uint32_t events, EventCallback cb);
    void modFd(int fd, uint32_t events);
    void removeFd(int fd);

    // 线程安全：从任意线程提交任务，在 IO 线程中执行
    void runInLoop(std::function<void()> task);

    // 阻塞运行事件循环（主线程调用）
    void run();

    // 请求停止（可从任意线程或信号处理函数调用）
    void stop();

private:
    void wakeup();
    void handleWakeup();

    int epoll_fd_{-1};
    int wakeup_fd_{-1};  // eventfd，用于跨线程唤醒
    std::atomic<bool> running_{false};

    std::unordered_map<int, EventCallback> callbacks_;

    std::mutex                         tasks_mutex_;
    std::vector<std::function<void()>> pending_tasks_;
};

} // namespace cloudvault
