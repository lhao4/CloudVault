#include "client.h"
#include "onlineuser.h"
#include "ui_onlineuser.h"

OnlineUser::OnlineUser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OnlineUser)
{
    ui->setupUi(this);
}

OnlineUser::~OnlineUser()
{
    delete ui;
}

void OnlineUser::updateUserList(QStringList userList)
{
    ui->listWidget->clear();
    ui->listWidget->addItems(userList);
}

void OnlineUser::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString curName = Client::getInstance().m_strLoginName;
    QString tarName = item->text();
    PDU* pdu = mkPDU();
    pdu->uiType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
    memcpy(pdu->caData,curName.toStdString().c_str(),32);
    memcpy(pdu->caData+32,tarName.toStdString().c_str(),32);
    Client::getInstance().sendMsg(pdu);
}
