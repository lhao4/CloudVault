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

/**
 * @brief 基于 QTcpSocket 的客户端网络适配器。
 *
 * 提供连接管理、PDU 发送、粘包/半包解析与上层信号通知。
 */
class TcpClient : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造 TCP 客户端对象。
     * @param parent Qt 父对象。
     */
    explicit TcpClient(QObject* parent = nullptr);

    /**
     * @brief 发起异步连接。
     * @param host 服务端地址。
     * @param port 服务端端口。
     */
    void connectToServer(const QString& host, quint16 port);

    /**
     * @brief 主动断开连接。
     */
    void disconnectFromServer();

    /**
     * @brief 发送一个完整 PDU 字节数组。
     * @param data 已编码的协议字节。
     */
    void send(std::vector<uint8_t> data);

    /**
     * @brief 查询当前是否已连接。
     * @return true 表示已建立 TCP 连接。
     */
    bool isConnected() const;

signals:
    /// @brief 连接建立成功。
    void connected();
    /// @brief 连接断开。
    void disconnected();
    /// @brief 收到完整 PDU。
    void pduReceived(PDUHeader header, std::vector<uint8_t> body);
    /// @brief 网络错误。
    void errorOccurred(const QString& message);

private slots:
    /**
     * @brief 套接字 connected 信号回调。
     */
    void onConnected();
    /**
     * @brief 套接字 disconnected 信号回调。
     */
    void onDisconnected();
    /**
     * @brief 套接字 readyRead 信号回调，执行拆包。
     */
    void onReadyRead();
    /**
     * @brief 套接字错误回调。
     * @param error Qt Socket 错误码。
     */
    void onSocketError(QAbstractSocket::SocketError error);

private:
    /// @brief 底层 TCP 套接字。
    QTcpSocket socket_;
    /// @brief 协议解析器，处理粘包/半包。
    PDUParser  parser_;
};

} // namespace cloudvault
