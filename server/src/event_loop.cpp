// =============================================================
// server/src/event_loop.cpp
// EventLoop 实现：epoll + eventfd
// =============================================================

#include "server/event_loop.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace cloudvault {

EventLoop::EventLoop() {
    // epoll_create1(EPOLL_CLOEXEC)：创建 epoll 实例
    // EPOLL_CLOEXEC：fork 子进程时自动关闭该 fd，防止文件描述符泄漏
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0) {
        throw std::runtime_error(
            std::string("epoll_create1 failed: ") + strerror(errno));
    }

    // eventfd：轻量级的线程间通知机制
    // EFD_NONBLOCK：read/write 不阻塞
    // EFD_CLOEXEC ：fork 时自动关闭
    wakeup_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeup_fd_ < 0) {
        close(epoll_fd_);
        throw std::runtime_error(
            std::string("eventfd failed: ") + strerror(errno));
    }

    // 监听 wakeup_fd_ 的可读事件（有任务入队时触发）
    addFd(wakeup_fd_, EPOLLIN, [this](uint32_t) { handleWakeup(); });
}

EventLoop::~EventLoop() {
    if (epoll_fd_ >= 0) close(epoll_fd_);
    if (wakeup_fd_ >= 0) close(wakeup_fd_);
}

void EventLoop::addFd(int fd, uint32_t events, EventCallback cb) {
    callbacks_[fd] = std::move(cb);

    epoll_event ev{};
    ev.events  = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        spdlog::warn("epoll_ctl ADD fd={} failed: {}", fd, strerror(errno));
    }
}

void EventLoop::modFd(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events  = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
        spdlog::warn("epoll_ctl MOD fd={} failed: {}", fd, strerror(errno));
    }
}

void EventLoop::removeFd(int fd) {
    callbacks_.erase(fd);
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

void EventLoop::runInLoop(std::function<void()> task) {
    {
        std::lock_guard lock(tasks_mutex_);
        pending_tasks_.push_back(std::move(task));
    }
    wakeup();  // 唤醒 epoll_wait，让 IO 线程尽快处理任务
}

void EventLoop::run() {
    running_.store(true);
    spdlog::debug("EventLoop started");

    constexpr int kMaxEvents = 64;
    epoll_event   events[kMaxEvents];

    while (running_.load()) {
        // ── 先处理跨线程提交的任务 ────────────────────────
        std::vector<std::function<void()>> tasks;
        {
            std::lock_guard lock(tasks_mutex_);
            tasks.swap(pending_tasks_);
        }
        for (auto& t : tasks) t();

        // ── epoll_wait 阻塞等待 I/O 事件 ──────────────────
        // 超时 100ms：防止 running_ 变为 false 后无限等待
        int n = epoll_wait(epoll_fd_, events, kMaxEvents, 100);
        if (n < 0) {
            if (errno == EINTR) continue;  // 被信号打断，继续
            spdlog::error("epoll_wait error: {}", strerror(errno));
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            auto it = callbacks_.find(fd);
            if (it != callbacks_.end()) {
                it->second(events[i].events);
            }
        }
    }

    spdlog::debug("EventLoop stopped");
}

void EventLoop::stop() {
    running_.store(false);
    wakeup();  // 唤醒 epoll_wait，让 run() 尽快退出
}

void EventLoop::wakeup() {
    // 向 eventfd 写入 1，使 epoll_wait 立即返回
    const uint64_t one = 1;
    if (::write(wakeup_fd_, &one, sizeof(one)) < 0 && errno != EAGAIN) {
        spdlog::warn("EventLoop::wakeup write failed: {}", strerror(errno));
    }
}

void EventLoop::handleWakeup() {
    // 读取并清零 eventfd 计数器，防止重复触发
    uint64_t val = 0;
    if (::read(wakeup_fd_, &val, sizeof(val)) < 0 && errno != EAGAIN) {
        spdlog::warn("EventLoop::handleWakeup read failed: {}", strerror(errno));
    }
}

} // namespace cloudvault
