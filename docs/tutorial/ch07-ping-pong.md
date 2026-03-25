# 第七章　PING/PONG 联通——Reactor 网络层

> **状态**：✅ 已完成
>
> **本章目标**：在服务端实现基于 epoll 的 Reactor 事件循环，在客户端接入 QTcpSocket，完成第一次端到端消息交互：客户端发送 PING，服务器回复 PONG，客户端 UI 状态栏变绿。
>
> **验收标准**：服务器启动后，客户端登录窗口底部状态栏显示绿色在线状态和服务器地址。

---

## 7.1 Reactor 模式概述

传统的"一连接一线程"模型在并发量大时会因线程切换开销而崩溃。**Reactor 模式**用一个（或少量）线程监听所有 fd 的事件，只有当事件真正发生时才处理：

```
                  ┌─────────────────────────────────────┐
                  │           IO 线程（Reactor）          │
                  │                                     │
  新连接 ──────→  │  epoll_wait()  ←── wakeup_fd         │
  客户端数据 ───→  │       │                              │
                  │    dispatch callbacks                │
                  │       │                              │
                  │  EventLoop::run()                    │
                  └───────┬─────────────────────────────┘
                          │ enqueue(task)
                  ┌───────▼─────────────────────────────┐
                  │        工作线程池（ThreadPool）        │
                  │  worker-0  worker-1  worker-2 ...   │
                  │       业务逻辑（PDU 处理）             │
                  └─────────────────────────────────────┘
```

**关键分工**：
- **IO 线程**：只做 epoll_wait + read/write，不执行耗时业务逻辑
- **工作线程**：处理 PDU、查数据库、计算哈希等耗时操作
- **跨线程通信**：工作线程通过 `EventLoop::runInLoop()` + eventfd 安全地将写操作调度回 IO 线程

---

## 7.2 EventLoop：epoll + eventfd

### 7.2.1 核心机制

```cpp
EventLoop::EventLoop() {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);  // 创建 epoll 实例
    wakeup_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);  // 跨线程唤醒

    // 监听 wakeup_fd_：工作线程提交任务后写入 1，唤醒 epoll_wait
    addFd(wakeup_fd_, EPOLLIN, [this](uint32_t) { handleWakeup(); });
}
```

**eventfd** 是 Linux 特有的轻量级事件通知机制：
- 内核维护一个 64 位计数器
- `write(wakeup_fd_, &1, 8)` → 计数器 +1，fd 变为可读
- `read(wakeup_fd_, &val, 8)` → 读出计数器并清零
- 比 `pipe` 少占一个 fd，性能更好

### 7.2.2 主循环

```cpp
void EventLoop::run() {
    running_.store(true);
    while (running_.load()) {
        // 先处理跨线程提交的任务（swap 技巧避免长时间持锁）
        std::vector<std::function<void()>> tasks;
        {
            std::lock_guard lock(tasks_mutex_);
            tasks.swap(pending_tasks_);
        }
        for (auto& t : tasks) t();

        // epoll_wait 最多阻塞 100ms（保证 running_ = false 后能退出）
        int n = epoll_wait(epoll_fd_, events, kMaxEvents, 100);
        for (int i = 0; i < n; ++i) {
            auto it = callbacks_.find(events[i].data.fd);
            if (it != callbacks_.end()) it->second(events[i].events);
        }
    }
}
```

### 7.2.3 跨线程任务提交

```cpp
void EventLoop::runInLoop(std::function<void()> task) {
    { std::lock_guard lock(tasks_mutex_); pending_tasks_.push_back(std::move(task)); }
    wakeup();  // 写入 eventfd，唤醒 epoll_wait
}
```

工作线程调用 `runInLoop()` 提交 "在 IO 线程执行" 的任务，例如将数据写回客户端：

```cpp
// 工作线程中：
void TcpConnection::send(std::vector<uint8_t> data) {
    { std::lock_guard lock(send_mutex_); send_buf_.insert(...); }
    loop_->runInLoop([self = shared_from_this()] {
        self->flushSendBuf();  // 在 IO 线程执行实际 write()
    });
}
```

---

## 7.3 TcpServer：监听与 accept

```
构造函数
  ├── socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC)
  ├── setsockopt(SO_REUSEADDR)   ← 允许立即重用端口
  ├── bind(host, port)
  └── listen(SOMAXCONN)

start()
  └── loop_->addFd(listen_fd_, EPOLLIN | EPOLLET, onAccept)

onAccept()（ET 模式，必须循环 accept）
  └── while (true)
        ├── accept4(SOCK_NONBLOCK | SOCK_CLOEXEC) → conn_fd
        ├── 创建 TcpConnection
        ├── loop_->addFd(conn_fd, EPOLLIN | EPOLLET, ...)
        └── new_conn_cb_(conn)
```

**边缘触发（ET）vs 水平触发（LT）**：

| 模式 | 触发条件 | 要求 |
|------|---------|------|
| LT（默认）| fd 有数据时每次 epoll_wait 都触发 | 可以分批读 |
| ET | 只在状态变化（新数据到达）时触发一次 | 必须循环读到 EAGAIN |

ET 模式减少了 epoll_wait 的调用次数，但要求循环读完所有数据。

**`accept4` vs `accept + fcntl`**：

```cpp
// 传统写法（两次系统调用）：
int fd = accept(listen_fd_, ...);
fcntl(fd, F_SETFL, O_NONBLOCK);

// accept4（一次系统调用，原子操作）：
int fd = accept4(listen_fd_, ..., SOCK_NONBLOCK | SOCK_CLOEXEC);
```

---

## 7.4 TcpConnection：状态管理

`TcpConnection` 继承 `enable_shared_from_this`，允许在 lambda 中安全持有 `shared_ptr<TcpConnection>`，防止在回调执行期间对象被析构。

### 7.4.1 ET 模式读循环

```cpp
void TcpConnection::onReadable() {
    char buf[4096];
    while (true) {
        ssize_t n = recv(fd_, buf, sizeof(buf), 0);
        if (n > 0) {
            parser_.feed(buf, n);
            PDUHeader hdr; std::vector<uint8_t> body;
            while (parser_.tryParse(hdr, body)) {
                msg_cb_(shared_from_this(), hdr, std::move(body));
            }
        } else if (n == 0) {
            doClose();  // 对端关闭
            return;
        } else {
            if (errno == EAGAIN) break;  // 读完了
            doClose();                   // 错误
            return;
        }
    }
}
```

### 7.4.2 背压处理（发送缓冲区满）

```cpp
void TcpConnection::flushSendBuf() {
    // ... 循环 send()
    if (errno == EAGAIN) {
        // 内核发送缓冲区满，剩余数据放回 send_buf_
        // 注册 EPOLLOUT：缓冲区有空位时 onWritable() 会被调用
        loop_->modFd(fd_, EPOLLIN | EPOLLOUT | EPOLLET);
        return;
    }
    // 全部发完，取消 EPOLLOUT 监听（避免 busy loop）
    loop_->modFd(fd_, EPOLLIN | EPOLLET);
}
```

---

## 7.5 ThreadPool：工作线程池

```cpp
ThreadPool::ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock lock(mutex_);
                    cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                    if (stop_ && tasks_.empty()) return;  // 优雅退出
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}
```

`condition_variable::wait(lock, predicate)` 是带谓词的等待，等价于：

```cpp
while (!predicate()) cv_.wait(lock);
```

使用谓词可以防止**虚假唤醒（spurious wakeup）**——条件变量在某些实现中可能无故返回。

---

## 7.6 消息流程：PING → PONG

```
客户端（Qt 主线程）
  LoginWindow 构造
    → setupNetwork()
    → TcpClient::connectToServer("127.0.0.1", 5000)
    → QTcpSocket::connectToHost()
    → onServerConnected()
    → TcpClient::send(PDUBuilder(PING).build())

服务器（IO 线程）
  epoll_wait → TcpConnection::onReadable()
    → PDUParser::tryParse() → PDUHeader{ message_type = PING }
    → ThreadPool::enqueue(task)

服务器（工作线程）
  MessageDispatcher::dispatch()
    → PING handler
    → PDUBuilder(PONG).build()
    → TcpConnection::send(pong_bytes)
    → EventLoop::runInLoop(flushSendBuf)

服务器（IO 线程）
  flushSendBuf() → write(conn_fd, pong_bytes)

客户端（Qt 主线程）
  QTcpSocket::readyRead → TcpClient::onReadyRead()
    → PDUParser::tryParse() → PDUHeader{ message_type = PONG }
    → emit pduReceived()
    → ResponseRouter::dispatch()
    → PONG handler
    → ui_->serverAddrLabel->setText("127.0.0.1:5000")
    → ui_->serverDotLabel->setStyleSheet("color: #16A34A;")  ← 绿色 ✓
```

---

## 7.7 客户端网络层（Qt）

客户端不需要 epoll，因为 Qt 的事件循环（`QCoreApplication::exec()`）已经内置了 I/O 多路复用。`QTcpSocket` 将底层 select/epoll 抽象为信号槽：

```cpp
// TcpClient 构造函数中绑定信号
connect(&socket_, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);

void TcpClient::onReadyRead() {
    const QByteArray raw = socket_.readAll();
    parser_.feed(raw.constData(), raw.size());  // 复用 PDUParser

    PDUHeader hdr; std::vector<uint8_t> body;
    while (parser_.tryParse(hdr, body)) {
        emit pduReceived(hdr, std::move(body));
    }
}
```

**ResponseRouter** 在客户端与服务端 **MessageDispatcher** 设计对称，区别是：
- 服务端：handler 在工作线程中执行，不能直接操作 UI
- 客户端：handler 在 Qt 主线程中执行，可以直接操作 UI 控件

---

## 7.8 构建与测试

```bash
# 服务端（WSL）
cmake -B ~/cv-server-build -S /mnt/d/CloudVault/server -G Ninja
cmake --build ~/cv-server-build
cp server/config/server.example.json server.json
~/cv-server-build/cloudvault_server server.json
```

预期输出：
```
[INFO] CloudVault Server v2.0
[INFO] 监听地址: 0.0.0.0:5000
[INFO] 服务已启动，等待连接...
```

启动客户端后，服务器日志应出现：
```
[INFO] New connection from 127.0.0.1:XXXXX (fd=5)
[DEBUG] PING from 127.0.0.1:XXXXX
```

客户端登录窗口底部状态栏应显示绿色圆点 + "127.0.0.1:5000"。

---

## 7.9 本章新知识点

- **Reactor 模式**：IO 线程 + 工作线程池，解耦网络 I/O 与业务逻辑
- **epoll ET 模式**：边缘触发，必须循环读到 EAGAIN
- **eventfd**：轻量级跨线程唤醒机制，比 pipe 少用一个 fd
- **accept4**：原子设置 SOCK_NONBLOCK | SOCK_CLOEXEC，避免竞态
- **enable_shared_from_this**：在成员函数中安全获取 `shared_ptr<this>`
- **背压（back pressure）**：发送缓冲区满时注册 EPOLLOUT，缓冲区空闲后继续写
- **condition_variable 谓词**：防止虚假唤醒
- **QTcpSocket 信号槽**：Qt 对底层 epoll 的封装，与 PDUParser 无缝集成

---

上一章：[第六章 — 协议基础](ch06-protocol.md) ｜ 下一章：第八章 — 用户认证（开发中）
