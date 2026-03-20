#include "mytcpsocket.h"
#include <QDebug>
#include <QTcpServer>
#include "operatedb.h"
#include "mytcpserver.h"
#include "msghandler.h"

MyTcpSocket::MyTcpSocket()
{
    mh = new MsgHandler;
}

PDU *MyTcpSocket::handleMsg(PDU *pdu)
{
    if(pdu == NULL){
        return NULL;
    }
    qDebug() << "handleMsg pdu->uiPDULen" <<pdu->uiPDULen
             << "pdu->uiMsgLen" <<pdu->uiMsgLen
             << "pdu->uiType" <<pdu->uiType
             << "pdu->caData" <<pdu->caData
             << "pdu->caData + 32" <<pdu->caData + 32
             << "pdu->caMsg" <<pdu->caMsg;
    mh->pdu = pdu;
    switch(pdu->uiType){
    //注册
    case ENUM_MSG_TYPE_REGIST_REQUEST:
        return mh->regist();
    //登录
    case ENUM_MSG_TYPE_LOGIN_REQUEST:
        return mh->login(m_strLoginName);
    //查找用户
    case ENUM_MSG_TYPE_FIND_USER_REQUEST:
        return mh->findUser();
    //在线用户
    case ENUM_MSG_TYPE_ONLINE_USER_REQUEST:
        return mh->onlineUser();
    //添加好友
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        return mh->addFriend();
    //是否添加好友
    case ENUM_MSG_TYPE_AGREE_ADD_FRIEND_REQUEST:
        return mh->agreeAddFriend();
    case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:
        return mh->flushFriend();
    case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:
        return mh->delFriend();
    case ENUM_MSG_TYPE_CHAT_REQUEST:
        return mh->chat();
    case ENUM_MSG_TYPE_MKDIR_REQUEST:
        return mh->mkDir();
    case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:
        return mh->flushFile();
    case ENUM_MSG_TYPE_MOVE_FILE_REQUEST:
        return mh->moveFile();
    case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:
        return mh->uploadFile();
    case ENUM_MSG_TYPE_UPLOAD_FILE_DATA_REQUEST:
        return mh->uploadFileData();
    case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:
        return mh->shareFlie();
    case ENUM_MSG_TYPE_SHARE_FILE_AGREE_REQUEST:
        return mh->shareFlieAgree();
    default:
        return NULL;
    }
}

void MyTcpSocket::sendMsg(PDU *pdu)
{
    if(pdu == NULL){
        return;
    }
    this->write((char*)pdu,pdu->uiPDULen);
    qDebug() << "sendMsg pdu->uiPDULen" <<pdu->uiPDULen
             << "pdu->uiMsgLen" <<pdu->uiMsgLen
             << "pdu->uiType" <<pdu->uiType
             << "pdu->caData" <<pdu->caData
             << "pdu->caData + 32" <<pdu->caData + 32
             << "pdu->caMsg" <<pdu->caMsg;
    free(pdu);
    pdu = NULL;
}

MyTcpSocket::~MyTcpSocket()
{
    delete mh;
}

void MyTcpSocket::recvMsg()
{
    qDebug()<<"recvMsg接收消息长度"<<this->bytesAvailable();
    //定义buffer成员变量，全部未处理的数据
    QByteArray data = this->readAll();
    buffer.append(data);
    while(buffer.size()>=int(sizeof(PDU))){
        PDU* pdu = (PDU*)buffer.data();
        if(buffer.size()<int(pdu->uiPDULen)){
            break;
        }
        PDU* respdu = handleMsg(pdu);
        sendMsg(respdu);
        buffer.remove(0,pdu->uiPDULen);
    }
}

void MyTcpSocket::clientOffline()
{
    OperateDB::getInstance().handleOfflint(m_strLoginName.toStdString().c_str());
    MyTcpServer::getInstance().deleteSocket(this);
}
