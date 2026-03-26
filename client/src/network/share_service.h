// =============================================================
// client/src/network/share_service.h
// 第十三章：文件分享协议封装
// =============================================================

#pragma once

#include "network/response_router.h"
#include "network/tcp_client.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace cloudvault {

class ShareService : public QObject {
    Q_OBJECT

public:
    explicit ShareService(TcpClient& client,
                          ResponseRouter& router,
                          QObject* parent = nullptr);

    void shareFile(const QString& file_path, const QStringList& targets);
    void acceptShare(const QString& sender_name, const QString& source_path);

signals:
    void shareRequestSent(const QString& message);
    void shareRequestFailed(const QString& reason);
    void incomingShareRequest(const QString& from_user, const QString& file_path);
    void shareAccepted(const QString& message);
    void shareAcceptFailed(const QString& reason);

private:
    void onShareResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onShareNotify(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    void onShareAgreeResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);

    TcpClient& client_;
    ResponseRouter& router_;

    QString pending_share_path_;
    QStringList pending_share_targets_;
    QString pending_accept_sender_;
    QString pending_accept_path_;
};

} // namespace cloudvault
