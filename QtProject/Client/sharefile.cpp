#include "client.h"
#include "index.h"
#include "sharefile.h"
#include "ui_sharefile.h"
#include <QDebug>

ShareFile::ShareFile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ShareFile)
{
    ui->setupUi(this);
}

void ShareFile::updataFriend_LW()
{
    ui->friend_LW->clear();
    QListWidget* friendList = Index::getInstance().getFriend()->getFriend_LW();
    for (int i=0;i<friendList->count();i++) {
        QListWidgetItem* pFriendItem = new QListWidgetItem(*friendList->item(i));
        ui->friend_LW->addItem(pFriendItem);
    }
}

ShareFile::~ShareFile()
{
    delete ui;
}


void ShareFile::on_allSelect_PB_clicked()
{
    for (int i = 0;i<ui->friend_LW->count();i++) {
        ui->friend_LW->item(i)->setSelected(true);
    }
}

void ShareFile::on_cancleSelec_PB_clicked()
{
    for (int i = 0;i<ui->friend_LW->count();i++) {
        ui->friend_LW->item(i)->setSelected(false);
    }
}

void ShareFile::on_confirm_PB_clicked()
{
    QString curFileName = Index::getInstance().getFile()->m_strShareFileName;
    QString curFilePath = Index::getInstance().getFile()->m_strCurPath;
    QString strPath = QString("%1/%2").arg(curFilePath).arg(curFileName);
    qDebug()<<"strPath:"<<strPath;
    QString strCurName = Client::getInstance().m_strLoginName;
    QList<QListWidgetItem*> pItems = ui->friend_LW->selectedItems();
    int friend_num = pItems.size();
    qDebug()<<"friend_num:"<<friend_num;
    PDU* pdu = mkPDU(pItems.size()*32 + strPath.toStdString().size()+1);
    pdu->uiType = ENUM_MSG_TYPE_SHARE_FILE_REQUEST;
    memcpy(pdu->caData,strCurName.toStdString().c_str(),32);
    memcpy(pdu->caData+32,&friend_num,sizeof(int));
    for (int i = 0;i<friend_num;i++) {
        memcpy((char*)pdu->caMsg+i*32,pItems.at(i)->text().toStdString().c_str(),32);
    }
    memcpy((char*)pdu->caMsg+friend_num*32,strPath.toStdString().c_str(),strPath.size());
    Client::getInstance().sendMsg(pdu);

}
