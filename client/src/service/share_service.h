// =============================================================
// client/src/service/share_service.h
// 第十三章：文件分享协议封装
// =============================================================

#pragma once

#include "network/response_router.h"
#include "network/tcp_client.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace cloudvault {

/**
 * @brief 文件分享业务服务。
 *
 * 封装“向好友分享文件”和“接收分享文件”两条协议流程。
 */
class ShareService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造分享服务。
     * @param client TCP 客户端引用。
     * @param router 响应路由器引用。
     * @param parent Qt 父对象。
     */
    explicit ShareService(TcpClient& client,
                          ResponseRouter& router,
                          QObject* parent = nullptr);

    /**
     * @brief 向多个好友发送文件分享请求。
     * @param file_path 待分享文件路径。
     * @param targets 目标用户名列表。
     */
    void shareFile(const QString& file_path, const QStringList& targets);
    /**
     * @brief 接收某个分享请求。
     * @param sender_name 发送方用户名。
     * @param source_path 发送方文件路径。
     */
    void acceptShare(const QString& sender_name, const QString& source_path);

signals:
    /// @brief 分享请求发送成功。
    void shareRequestSent(const QString& message);
    /// @brief 分享请求发送失败。
    void shareRequestFailed(const QString& reason);
    /// @brief 收到分享请求推送。
    void incomingShareRequest(const QString& from_user, const QString& file_path);
    /// @brief 接收分享成功。
    void shareAccepted(const QString& message);
    /// @brief 接收分享失败。
    void shareAcceptFailed(const QString& reason);

private:
    /// @brief 处理分享响应。
    void onShareResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理分享通知推送。
    void onShareNotify(const PDUHeader& hdr, const std::vector<uint8_t>& body);
    /// @brief 处理同意分享响应。
    void onShareAgreeResponse(const PDUHeader& hdr, const std::vector<uint8_t>& body);

    /// @brief TCP 客户端引用。
    TcpClient& client_;
    /// @brief 响应路由器引用。
    ResponseRouter& router_;

    /// @brief 当前挂起的分享文件路径。
    QString pending_share_path_;
    /// @brief 当前挂起的分享目标列表。
    QStringList pending_share_targets_;
    /// @brief 当前挂起的接收方用户名。
    QString pending_accept_sender_;
    /// @brief 当前挂起的接收源路径。
    QString pending_accept_path_;
};

} // namespace cloudvault
