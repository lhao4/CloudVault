#include "friend.h"
#include "ui_friend.h"
#include "QDebug"
#include <QInputDialog>
#include <qmessagebox.h>
#include "protocol.h"
#include "client.h"

Friend::Friend(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Friend)
{
    ui->setupUi(this);
    m_pOnlineUser = new OnlineUser;
    flushFriend();
    m_pChat = new Chat;
}

Friend::~Friend()
{
    delete ui;
    delete m_pOnlineUser;
    delete m_pChat;
}

void Friend::flushFriend()
{
    PDU* pdu = mkPDU();
    pdu->uiType = ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST;
    memcpy(pdu->caData,Client::getInstance().m_strLoginName.toStdString().c_str(),32);
    Client::getInstance().sendMsg(pdu);
}

void Friend::updateFriendrList(QStringList FriendList)
{
    ui->listWidget->clear();
    ui->listWidget->addItems(FriendList);
}

QListWidget* Friend::getFriend_LW()
{
    return ui->listWidget;
}

void Friend::on_findUser_PB_clicked()
{
    bool isOk;
    // 增加一个bool参数接收用户操作状态
    QString in_Name = QInputDialog::getText(this, "查找用户", "请输入查找的用户",
                                           QLineEdit::Normal, "", &isOk);

    // 如果用户点击了取消或关闭对话框，直接返回
    if (!isOk) {
        return;
    }

    //QString in_Name = QInputDialog::getText(this, "查找用户", "请输入查找的用户");
    if(in_Name.isEmpty() || in_Name.toStdString().size() > 32)
    {
        QMessageBox::information(this,"提示","用户名长度不合法");
        return;
    }
    PDU* pdu = mkPDU();
    memcpy(pdu->caData,in_Name.toStdString().c_str(),32);
    pdu->uiType = ENUM_MSG_TYPE_FIND_USER_REQUEST;
    Client::getInstance().sendMsg(pdu);
}


void Friend::on_onlineUser_PB_clicked()
{
    if(m_pOnlineUser->isHidden()){
        m_pOnlineUser->show();
    }
    PDU* pdu = mkPDU();
    pdu->uiType = ENUM_MSG_TYPE_ONLINE_USER_REQUEST;
    Client::getInstance().sendMsg(pdu);
}

void Friend::on_flushFriend_PB_clicked()
{
    flushFriend();
}

void Friend::on_delFriend_PB_clicked()
{
    QListWidgetItem* pItem = ui->listWidget->currentItem();
    if(!pItem){
        return;
    }
    QString strDelName = pItem->text();
    int ret = QMessageBox::question(this,"提示",QString("是否确认删除好友%1？").arg(strDelName));
    if(ret != QMessageBox::Yes){
        return;
    }
    PDU* pdu = mkPDU();
    memcpy(pdu->caData,Client::getInstance().m_strLoginName.toStdString().c_str(),32);
    memcpy(pdu->caData+32,strDelName.toStdString().c_str(),32);
    pdu->uiType = ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST;
    Client::getInstance().sendMsg(pdu);
}

void Friend::on_chat_PB_clicked()
{
    QListWidgetItem* pItem = ui->listWidget->currentItem();
    if(!pItem){
        return;
    }
    if(m_pChat->isHidden()){
        m_pChat->show();
    }
    m_pChat->m_strChatName = pItem->text();
}
