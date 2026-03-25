# Qt 信号槽机制详解

## 1. 是什么

信号槽是 Qt 的对象通信机制，解耦发送者与接收者。

## 2. Q_OBJECT 的作用

`Q_OBJECT` 触发 MOC 生成元对象代码；缺失会导致信号槽失效或链接错误。

## 3. connect 常见写法

```cpp
connect(sender, &Sender::sig, receiver, &Receiver::slot);
```

## 4. 连接类型

1. 直接连接：同线程立即调用。
2. 队列连接：跨线程进入事件队列。

## 5. 内存管理

QObject 父子关系会自动释放子对象，避免手动 `delete`。
