#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QThreadPool>
#include "mytcpsocket.h"

class MyTcpServer : public QTcpServer
{
    Q_OBJECT
public:
   static MyTcpServer& getInstance();
   void incomingConnection(qintptr handle) override;
   QList<MyTcpSocket*> m_tcpSocketList;//用于存客户端的socket指针
   void deleteSocket(MyTcpSocket* mysocket);
   void resend(char* strName,PDU* pdu);//转发
   QThreadPool* threadPool;

private:
    MyTcpServer();
    MyTcpServer(const MyTcpServer& instance) = delete;
    MyTcpServer& operator=(const MyTcpServer&) = delete;
};

#endif // MYTCPSERVER_H

