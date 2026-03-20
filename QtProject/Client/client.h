#ifndef CLIENT_H
#define CLIENT_H

#include "protocol.h"
#include "reshandler.h"

#include <QWidget>
#include <QTcpSocket>
#include <QDebug>
#include <QFile>
#include <QHostAddress>

QT_BEGIN_NAMESPACE
namespace Ui { class Client; }
QT_END_NAMESPACE

class Client : public QWidget
{
    Q_OBJECT

public:
    ~Client();
    void loadConfig();
    QString m_strIP;//服务器IP
    quint16 m_usPort;//服务器端口号
    QTcpSocket socket;//TCP套接字对象

    QString m_strRootPath;
    QString m_strLoginName;
    //单例模式获取对象
    static Client& getInstance();
    void recvMsg();
    void sendMsg(PDU* pdu);
    void handlerMsg(PDU* pdu);
    ResHandler* rh;
    QByteArray buffer;

//添加信号槽声明
public slots:
    void showConnect();//连接成功的处理槽函数

private slots:

//  void on_pushButton_clicked();

    void on_regist_PB_clicked();

    void on_login_PB_clicked();

private:
    Ui::Client *ui;
    Client(QWidget *parent = nullptr);
    Client(const Client& instance) = delete;
    Client& operator = (const Client&) = delete;

};
#endif // CLIENT_H

