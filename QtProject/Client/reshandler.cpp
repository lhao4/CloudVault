#include "client.h"
#include "index.h"
#include "reshandler.h"
#include "stdlib.h"

#include <QDir>
#include <qmessagebox.h>
ResHandler::ResHandler()
{

}

void ResHandler::regist()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        QMessageBox::information(&Client::getInstance(),"提示","注册成功");
    }else{
        QMessageBox::warning(&Client::getInstance(),"提示","注册失败");
    }
}

void ResHandler::login()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().show();
        Index::getInstance().setWindowTitle(QString("首页-%1").arg(Client::getInstance().m_strLoginName));
        Client::getInstance().hide();
    }else{
        QMessageBox::warning(&Client::getInstance(),"提示","登录失败");
    }
}

void ResHandler::findUser()
{
    char caName[32] = {'\0'};
    memcpy(caName,pdu->caData,32);
    int ret;
    // 从PDU数据区拷贝返回值
    memcpy(&ret, pdu->caData+32, sizeof(int));
    qDebug()<<"findUser ret"<<ret;
    // 根据不同的返回值显示相应提示
    if (ret == 2) {
        QMessageBox::information(&Index::getInstance(), "提示", QString("%1不存在").arg(caName));
    } else if (ret == -1) {
        QMessageBox::information(&Index::getInstance(), "提示", "查找失败");
    }  else if (ret == 0) {
        QMessageBox::information(&Index::getInstance(), "提示", QString("%1不在线").arg(caName));
    }else if (ret == 1) {
        int ret = QMessageBox::information(&Index::getInstance(), "提示", QString("%1在线").arg(caName),"添加好友","取消");
        qDebug()<<"ret"<<ret;
        if(ret == 0){
            QString strCurName = Client::getInstance().m_strLoginName;
            PDU* pdu = mkPDU();
            pdu->uiType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
            memcpy(pdu->caData,strCurName.toStdString().c_str(),32);
            memcpy(pdu->caData+32,caName,32);
            Client::getInstance().sendMsg(pdu);
        }
    }
}

void ResHandler::onlineUser(QString& strLoginName)
{
    int iSize = pdu->uiMsgLen/32;
    QStringList res;
    char caName[32] = {'\0'};
    for(int i = 0;i<iSize;i++){
        memcpy(caName,pdu->caMsg+i*32,32);
        if(QString(caName) == strLoginName){
            continue;
        }
        res.append(caName);
    }
    Index::getInstance().getFriend()->m_pOnlineUser->updateUserList(res);
}

void ResHandler::addFriend()
{
    int ret;
    memcpy(&ret,pdu->caData,sizeof(int));
    qDebug()<<"addFriend ret"<<ret;
    if(ret == 2){
        QMessageBox::information(&Index::getInstance(),"提示","该用户已是好友");
    }
    else if(ret == -1){
        QMessageBox::information(&Index::getInstance(),"提示","查询失败");
    }
    else if(ret == 0){
        QMessageBox::information(&Index::getInstance(),"提示","该用户不在线");
    }
}

void ResHandler::addFriendResend()
{
    char curName[32] = {'\0'};
    memcpy(curName,pdu->caData,32);
    int ret = QMessageBox::question(&Index::getInstance(),"提示",QString("%1 请求添加您为好友，是否同意？").arg(curName));
    if(ret != QMessageBox::Yes){
        return;
    }
    PDU* respdu = mkPDU();
    respdu->uiType = ENUM_MSG_TYPE_AGREE_ADD_FRIEND_REQUEST;
    memcpy(respdu->caData,pdu->caData,64);
    Client::getInstance().sendMsg(respdu);
}

void ResHandler::agreeAddFriend()
{
    bool ret;
    memcpy(&ret, pdu->caData, sizeof(bool));
    if (ret) {
        Index::getInstance().getFriend()->flushFriend();
    } else {
        QMessageBox::warning(&Client::getInstance(), "提示", "添加好友失败");
    }
}

void ResHandler::flushFriend()
{
    int iSize = pdu->uiMsgLen/32;
    QStringList res;
    char caName[32] = {'\0'};
    for(int i = 0;i<iSize;i++){
        memcpy(caName,pdu->caMsg+i*32,32);
        res.append(caName);
    }
    Index::getInstance().getFriend()->updateFriendrList(res);
}

void ResHandler::delFriend()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"delFriend ret"<<ret;
    if(ret){
        QMessageBox::information(&Index::getInstance(),"提示","删除成功");
        flushFriend();
    }
    else {
        QMessageBox::information(&Index::getInstance(),"提示","删除失败");
    }
}

void ResHandler::chat()
{
    Chat* c = Index::getInstance().getFriend()->m_pChat;
    if(c->isHidden()){
        c->show();
    }
    char caChatName[32] = {'\0'};
    memcpy(caChatName,pdu->caData,32);
    c->m_strChatName = caChatName;
    c->addMsg(QString("%1 : %2").arg(caChatName).arg(pdu->caMsg));

}

void ResHandler::mkDir()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().getFile()->flushFile();
    }else{
        QMessageBox::warning(&Index::getInstance(),"提示","创建文件夹失败");
    }
}

void ResHandler::flushFile()
{
    int iCount = pdu->uiMsgLen/sizeof(FileInfo);
    qDebug()<<"iCount"<<iCount;
    QList<FileInfo*> pFileList;
    for(int i = 0;i<iCount;i++){
        FileInfo* pFileInfo = new FileInfo;
        memcpy(pFileInfo,pdu->caMsg+i*sizeof(FileInfo),sizeof(FileInfo));
        pFileList.append(pFileInfo);
    }
    Index::getInstance().getFile()->updateFileList(pFileList);
}

void ResHandler::moveFile()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().getFile()->flushFile();
    }else{
        QMessageBox::warning(&Index::getInstance(),"提示","移动文件夹失败");
    }
}

void ResHandler::uploadFile()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().getFile()->uploadFile();
    }else{
        QMessageBox::warning(&Index::getInstance(),"提示","服务器初始化失败");
    }
}

void ResHandler::uploadFileData()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().getFile()->flushFile();
    }else{
        QMessageBox::warning(&Index::getInstance(),"提示","上传文件失败");
    }
}

void ResHandler::shareFile()
{
    QMessageBox::information(Index::getInstance().getFile(),"分享文件","文件已分享");
}

void ResHandler::shareFileRequest()
{
    QString strSharePath = QString(pdu->caMsg);
    int index = strSharePath.lastIndexOf('/');
    QString strFileName = strSharePath.right(strSharePath.size()-index-1);
    QString strMsg = QString("%1 分享文件: %2\n是否接收？").arg(pdu->caData).arg(strFileName);
    int ret = QMessageBox::question(&Index::getInstance(),"分享文件",strMsg);
    if(ret != QMessageBox::Yes){
        return;
    }
    PDU* respdu = mkPDU(pdu->uiMsgLen);
    respdu->uiType = ENUM_MSG_TYPE_SHARE_FILE_AGREE_REQUEST;
    QString strName = Client::getInstance().m_strLoginName;
    memcpy(respdu->caData,strName.toStdString().c_str(),32);
    memcpy(respdu->caMsg,pdu->caMsg,pdu->uiMsgLen);
    Client::getInstance().sendMsg(respdu);
}

void ResHandler::shareFileResult()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().getFile()->flushFile();
    }else{
        QMessageBox::warning(&Index::getInstance(),"提示","分享文件失败");
    }
}



