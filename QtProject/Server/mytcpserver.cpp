#include "clienttask.h"
#include "mytcpserver.h"
#include "mytcpsocket.h"
#include <QDebug>

MyTcpServer &MyTcpServer::getInstance()
{
    static MyTcpServer instance;
    return instance;
}

void MyTcpServer::incomingConnection(qintptr handle)
{
    qDebug()<<"新客户端连接";
    MyTcpSocket* psocket = new MyTcpSocket;
    psocket->setSocketDescriptor(handle);
    m_tcpSocketList.append(psocket);

//    for(int i = 0;i<m_tcpSocketList.size();i++){
//        qDebug()<<m_tcpSocketList[i];
//    }
    ClientTask* task = new ClientTask(psocket);
    threadPool->start(task);

}

void MyTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    m_tcpSocketList.removeOne(mysocket);
    mysocket->deleteLater();
    mysocket = NULL;
}

void MyTcpServer::resend(char* strName, PDU *pdu)
{
    if(strName == NULL || pdu == NULL){
        return;
    }
    for(int i = 0;i<m_tcpSocketList.size();i++){
        if(QString(strName) == m_tcpSocketList[i]->m_strLoginName){
            m_tcpSocketList[i]->write((char*)pdu,pdu->uiPDULen);
            qDebug() << "resendMsg pdu->uiPDULen" <<pdu->uiPDULen
                     << "pdu->uiMsgLen" <<pdu->uiMsgLen
                     << "pdu->uiType" <<pdu->uiType
                     << "pdu->caData" <<pdu->caData
                     << "pdu->caData + 32" <<pdu->caData + 32
                     << "pdu->caMsg" <<pdu->caMsg;
            break;
        }
    }
}

MyTcpServer::MyTcpServer()
{
    threadPool = QThreadPool::globalInstance();
    threadPool->setMaxThreadCount(8);
}


