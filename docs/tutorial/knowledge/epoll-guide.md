# epoll 工作原理图解

## 1. I/O 模型演进

阻塞 I/O -> 非阻塞 I/O -> `select` -> `poll` -> `epoll`

## 2. epoll 三个核心 API

```c
int epfd = epoll_create1(0);
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
int n = epoll_wait(epfd, events, maxevents, timeout);
```

## 3. LT 与 ET

1. LT（水平触发）：可读就持续通知，简单。
2. ET（边缘触发）：状态变化时通知，性能更高，但必须一次读到 `EAGAIN`。

## 4. 非阻塞 socket

```c
fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
```

## 5. 本项目建议

1. 监听 fd 使用 LT 起步，先保证正确性。
2. 稳定后再评估 ET。
3. 网络线程只做 I/O，不做数据库和文件重活。
