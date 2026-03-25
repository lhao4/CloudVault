// =============================================================
// client/src/network/tcp_client.h
// Qt TCP 客户端（基于 QTcpSocket）
//
// 职责：
//   - 维护与服务器的 TCP 连接（connect/disconnect）
//   - 利用 PDUParser 处理粘包/半包
//   - 收到完整 PDU 后 emit pduReceived 信号
//   - 提供线程安全的 send() 方法（在 Qt 主线程调用）
// =============================================================

#pragma once

#include "common/protocol.h"
#include "common/protocol_codec.h"

#include <QObject>
#include <QTcpSocket>
#include <QString>

#include <memory>

namespace cloudvault {

class TcpClient : public QObject {
    Q_OBJECT

public:
    explicit TcpClient(QObject* parent = nullptr);

    // 异步连接；连接成功/失败通过信号通知
    void connectToServer(const QString& host, quint16 port);

    // 断开连接
    void disconnectFromServer();

    // 发送 PDU（必须在 Qt 主线程调用）
    void send(std::vector<uint8_t> data);

    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void pduReceived(PDUHeader header, std::vector<uint8_t> body);
    void errorOccurred(const QString& message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    QTcpSocket socket_;
    PDUParser  parser_;
};

} // namespace cloudvault
