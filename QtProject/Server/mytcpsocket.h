#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QObject>
#include <qtcpsocket.h>
#include "protocol.h"


class MsgHandler;
class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    MyTcpSocket();
    QString m_strLoginName;
    PDU* handleMsg(PDU* pdu);
    void sendMsg(PDU* respdu);
    MsgHandler* mh;
    ~MyTcpSocket();
    QByteArray buffer;

public slots:
    void recvMsg();
    void clientOffline();

};

#endif // MYTCPSOCKET_H
