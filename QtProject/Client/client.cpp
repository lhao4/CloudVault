#include "client.h"
#include "ui_client.h"
#include "protocol.h"
#include "index.h"
#include <QDebug>
#include <QMessageBox>
Client::Client(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Client)
{
    ui->setupUi(this);
    loadConfig();//加载配置文件
    socket.connectToHost(QHostAddress(m_strIP),m_usPort);
    //连接信号与槽函数
    connect(&socket,&QTcpSocket::connected,this,&Client::showConnect);
    connect(&socket, &QTcpSocket::readyRead, this, &Client::recvMsg);
    rh = new ResHandler;
}

Client::~Client()
{
    delete ui;
    delete rh;
}
//加载配置文件
void Client::loadConfig()
{
    QFile file(":/client.config");
    if(file.open(QIODevice::ReadOnly)){
        QString strData = QString(file.readAll());
        qDebug() << "strData" << strData;
        QStringList strList = strData.split("\r\n");
        m_strIP = strList[0];
        m_usPort = strList[1].toUShort();
        //m_strLoginName = strList[2];
        m_strRootPath = strList[2];
        qDebug() << "loadConfig ip" << m_strIP << "port" << m_usPort<<"m_strLoginName"<< m_strLoginName;
        file.close();
    }else{
        qDebug() << "loadConfig 打开配置文件失败";
    }
}

void Client::sendMsg(PDU *pdu)
{
    socket.write((char*)pdu,pdu->uiPDULen);
    qDebug()<<"pdu->uiPDULen"<<pdu->uiPDULen
            <<"pdu->uiType"<<pdu->uiType
            <<"pdu->caData"<<pdu->caData
            <<"pdu->caData+32"<<pdu->caData+32
            <<"pdu->caMsg"<<pdu->caMsg;
    free(pdu);
    pdu = NULL;
}

void Client::handlerMsg(PDU *pdu)
{
    if(pdu == NULL){
        return;
    }
    rh->pdu = pdu;
    switch(pdu->uiType){
    //注册
    case ENUM_MSG_TYPE_REGIST_RESPOND:{
        rh->regist();
        break;
    }
    //登录
    case ENUM_MSG_TYPE_LOGIN_RESPOND:{
        rh->login();
        break;
    }
    // 处理查找用户响应
    case ENUM_MSG_TYPE_FIND_USER_RESPOND:{
        rh->findUser();
        break;
    }
    // 在线用户
    case ENUM_MSG_TYPE_ONLINE_USER_RESPOND:{
        rh->onlineUser(m_strLoginName);
        break;
    }
    // 添加用户
    case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND:{
        rh->addFriend();
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:{
        rh->addFriendResend();
        break;
    }
    case ENUM_MSG_TYPE_AGREE_ADD_FRIEND_RESPOND:{
        rh->agreeAddFriend();
        break;
    }
    //刷新好友
    case ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND:{
        rh->flushFriend();
        break;
    }
    case ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND:{
        rh->delFriend();
        break;
    }
    case ENUM_MSG_TYPE_CHAT_RESPOND:{
        rh->chat();
        break;
    }
    case ENUM_MSG_TYPE_MKDIR_RESPOND:{
        rh->mkDir();
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FILE_RESPOND:{
        rh->flushFile();
        break;
    }
    case ENUM_MSG_TYPE_MOVE_FILE_RESPOND:{
        rh->moveFile();
        break;
    } 
    case ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND:{
        rh->uploadFile();
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_DATA_RESPOND:{
        rh->uploadFileData();
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_RESPOND:{
        rh->shareFile();
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:{
        rh->shareFileRequest();
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_AGREE_RESPOND:{
        rh->shareFileResult();
        break;
    }
    default:
        break;
    }
}

void Client::showConnect()
{
    qDebug() <<"TCP连接服务器成功";
}

Client& Client::getInstance(){
    static Client instance;
    return instance;
}

void Client::on_regist_PB_clicked()
{
    QString strName = ui->name_LE->text();
    QString strPwd = ui->pwd_LE->text();
    if(strName.isEmpty() || strPwd.isEmpty()){
        QMessageBox::information(this,"提示","用户名或密码不能为空");
        return;
    }
    if(strName.size()>32 || strPwd.size()>32){
        QMessageBox::information(this,"提示","用户名或密码长度不合法");
        return;
    }
    PDU* pdu = mkPDU();
    memcpy(pdu->caData,strName.toStdString().c_str(),32);
    memcpy(pdu->caData+32,strPwd.toStdString().c_str(),32);
    pdu->uiType = ENUM_MSG_TYPE_REGIST_REQUEST;
    sendMsg(pdu);
}

void Client::recvMsg()
{

    qDebug()<<"recvMsg接收消息长度"<<socket.bytesAvailable();
    //定义buffer成员变量，全部未处理的数据
    QByteArray data = socket.readAll();
    buffer.append(data);
    while(buffer.size()>=int(sizeof(PDU))){
        PDU* pdu = (PDU*)buffer.data();
        if(buffer.size()<int(pdu->uiPDULen)){
            break;
        }
        handlerMsg(pdu);
        buffer.remove(0,pdu->uiPDULen);
    }

}

void Client::on_login_PB_clicked()
{
    QString strName = ui->name_LE->text();
    QString strPwd = ui->pwd_LE->text();
    if(strName.isEmpty() || strPwd.isEmpty()){
        QMessageBox::information(this,"提示","用户名或密码不能为空");
        return;
    }
    if(strName.size()>32 || strPwd.size()>32){
        QMessageBox::information(this,"提示","用户名或密码长度不合法");
        return;
    }
    PDU* pdu = mkPDU();
    memcpy(pdu->caData,strName.toStdString().c_str(),32);
    memcpy(pdu->caData+32,strPwd.toStdString().c_str(),32);
    m_strLoginName = strName;
    pdu->uiType = ENUM_MSG_TYPE_LOGIN_REQUEST;
    sendMsg(pdu);
    qDebug()<<"发送登录请求"<<strName;
}


