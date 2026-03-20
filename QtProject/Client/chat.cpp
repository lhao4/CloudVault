#include "chat.h"
#include "client.h"
#include "protocol.h"
#include "ui_chat.h"

Chat::Chat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Chat)
{
    ui->setupUi(this);
}

Chat::~Chat()
{
    delete ui;
}

void Chat::on_Send_PB_clicked()
{
    QString strMsg = ui->Msg->text();
    if (strMsg.isEmpty()) {
        return;
    }
    ui->Msg->clear();
    PDU* pdu = mkPDU(strMsg.toStdString().size());
    pdu->uiType = ENUM_MSG_TYPE_CHAT_REQUEST;
    memcpy(pdu->caData,Client::getInstance().m_strLoginName.toStdString().c_str(),32);
    memcpy(pdu->caData+32,m_strChatName.toStdString().c_str(),32);
    memcpy(pdu->caMsg,strMsg.toStdString().c_str(),strMsg.toStdString().size());
    Client::getInstance().sendMsg(pdu);
    ui->Show->append(strMsg);
}


void Chat::addMsg(QString strMsg)
{
    ui->Show->append(strMsg);
}
