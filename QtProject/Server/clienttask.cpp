#include "clienttask.h"
#include <QThread>


ClientTask::ClientTask(MyTcpSocket* socket):socket(socket)
{

}
void ClientTask::run(){
    socket->moveToThread(QThread::currentThread());
    // 连接信号与槽
    connect(socket, &QTcpSocket::readyRead, socket, &MyTcpSocket::recvMsg);
    connect(socket, &QTcpSocket::disconnected, socket, &MyTcpSocket::clientOffline);

}
