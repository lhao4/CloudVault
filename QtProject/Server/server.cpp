#include "mytcpserver.h"
#include "server.h"
#include "ui_server.h"

Server::Server(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Server)
{
    ui->setupUi(this);
    loadConfig();
    MyTcpServer::getInstance().listen(QHostAddress(m_strIP),m_usport);

}

Server &Server::getInstance()
{
    static Server instance;
    return instance;
}

Server::~Server()
{
    delete ui;
}

//加载配置文件
void Server::loadConfig()
{
    QFile file(":/client.config");
    if(file.open(QIODevice::ReadOnly)){
        QString strData = QString(file.readAll());
        QStringList strList = strData.split("\r\n");
        m_strIP = strList[0];
        m_usport = strList[1].toUShort();
        m_strRootPath = strList[2];
        qDebug() << "loadConfig ip" << m_strIP << "port" << m_usport<<"m_strRootPath"<<m_strRootPath;
        file.close();
    }else{
        qDebug() << "loadConfig 打开配置文件失败";
    }
}
