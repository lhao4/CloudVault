// =============================================================
// client/src/network/tcp_client.cpp
// Qt TCP 客户端实现
// =============================================================

#include "tcp_client.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcNet, "network.tcp")

namespace cloudvault {

TcpClient::TcpClient(QObject* parent) : QObject(parent) {
    // 绑定 QTcpSocket 的信号到本类的槽
    connect(&socket_, &QTcpSocket::connected,
            this, &TcpClient::onConnected);
    connect(&socket_, &QTcpSocket::disconnected,
            this, &TcpClient::onDisconnected);
    connect(&socket_, &QTcpSocket::readyRead,
            this, &TcpClient::onReadyRead);
    connect(&socket_, &QAbstractSocket::errorOccurred,
            this, &TcpClient::onSocketError);
}

void TcpClient::connectToServer(const QString& host, quint16 port) {
    qCDebug(lcNet) << "Connecting to" << host << ":" << port;
    socket_.connectToHost(host, port);
}

void TcpClient::disconnectFromServer() {
    socket_.disconnectFromHost();
}

void TcpClient::send(std::vector<uint8_t> data) {
    if (!isConnected()) return;

    const qint64 written = socket_.write(
        reinterpret_cast<const char*>(data.data()),
        static_cast<qint64>(data.size()));

    if (written < 0) {
        qCWarning(lcNet) << "write failed:" << socket_.errorString();
    }
}

bool TcpClient::isConnected() const {
    return socket_.state() == QAbstractSocket::ConnectedState;
}

// ── 槽函数 ────────────────────────────────────────────────

void TcpClient::onConnected() {
    qCDebug(lcNet) << "Connected to server";
    parser_.reset();
    emit connected();
}

void TcpClient::onDisconnected() {
    qCDebug(lcNet) << "Disconnected from server";
    emit disconnected();
}

void TcpClient::onReadyRead() {
    // 将所有可用字节喂给 PDUParser
    const QByteArray raw = socket_.readAll();
    parser_.feed(raw.constData(), static_cast<size_t>(raw.size()));

    PDUHeader            hdr;
    std::vector<uint8_t> body;
    while (parser_.tryParse(hdr, body)) {
        emit pduReceived(hdr, std::move(body));
        body.clear();  // tryParse 使用 move，需重置
    }
}

void TcpClient::onSocketError(QAbstractSocket::SocketError /*error*/) {
    const QString msg = socket_.errorString();
    qCWarning(lcNet) << "Socket error:" << msg;
    emit errorOccurred(msg);
}

} // namespace cloudvault
