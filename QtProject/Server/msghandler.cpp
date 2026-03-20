#include "msghandler.h"
#include "operatedb.h"
#include "mytcpsocket.h"
#include "mytcpserver.h"
#include "server.h"

#include <QDir>
MsgHandler::MsgHandler()
{

}

PDU *MsgHandler::regist()
{
    char caName[32] = {'\0'};
    memcpy(caName,pdu->caData,32);
    char caPwd[32] = {'\0'};
    memcpy(caPwd,pdu->caData+32,32);
    bool ret = OperateDB::getInstance().handleRegist(caName,caPwd);
    qDebug()<<"ret"<<ret;
    if(ret){
        QDir dir;
        bool mkRet = dir.mkdir(QString("%1/%2").arg(Server::getInstance().m_strRootPath).arg(caName));
        qDebug()<<"mkdir ret"<<mkRet;
    }
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(bool));
    respdu->uiType = ENUM_MSG_TYPE_REGIST_RESPOND;
    return respdu;
}

PDU *MsgHandler::login(QString& strLogiName)
{
    char caName[32] = {'\0'};
    char caPwd[32] = {'\0'};
    memcpy(caName,pdu->caData,32);
    memcpy(caPwd,pdu->caData+32,32);
    bool ret = OperateDB::getInstance().handleLogin(caName,caPwd);
    qDebug()<<"ret"<<ret;
    if(ret){
        strLogiName = caName;
    }
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(bool));
    respdu->uiType = ENUM_MSG_TYPE_LOGIN_RESPOND;
    return respdu;
}

PDU *MsgHandler::findUser()
{
    char caName[32] = {'\0'};
    memcpy(caName,pdu->caData,32);
    int ret = OperateDB::getInstance().handleFindUser(caName);
    qDebug() << "查找用户" << caName << "结果：" << ret;
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,caName,32);
    memcpy(respdu->caData+32,&ret,sizeof(int));
    respdu->uiType = ENUM_MSG_TYPE_FIND_USER_RESPOND;
    return respdu;
}

PDU *MsgHandler::onlineUser()
{
    QStringList res = OperateDB::getInstance().handleOnlineUser();
    qDebug() << "res"<<res.size();
    PDU* respdu = mkPDU(res.size()*32);
    for(int i = 0;i<res.size();i++){
        memcpy(respdu->caMsg+i*32,res[i].toStdString().c_str(),32);
    }
    respdu->uiType = ENUM_MSG_TYPE_ONLINE_USER_RESPOND;
    return respdu;
}

PDU* MsgHandler::addFriend()
{
    char curName[32] = {'\0'};
    memcpy(curName,pdu->caData,32);
    char tarName[32] = {'\0'};
    memcpy(tarName,pdu->caData+32,32);
    int ret = OperateDB::getInstance().handleAddFriend(curName,tarName);
    qDebug()<<"addFriend ret"<<ret;
    if(ret == 1){
        MyTcpServer::getInstance().resend(tarName,pdu);
    }
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(ret));
    respdu->uiType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
    return respdu;
}

PDU *MsgHandler::agreeAddFriend()
{
    char curName[32] = {'\0'};
    memcpy(curName,pdu->caData,32);
    char tarName[32] = {'\0'};
    memcpy(tarName,pdu->caData+32,32);
    bool ret = OperateDB::getInstance().handleAgreeAddFriend(curName,tarName);
    qDebug()<<"agreeAddFriend ret"<<ret;
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(ret));
    respdu->uiType = ENUM_MSG_TYPE_AGREE_ADD_FRIEND_RESPOND;
    MyTcpServer::getInstance().resend(curName,respdu);
    return respdu;
}

PDU *MsgHandler::flushFriend()
{
    char caName[32] = {'\0'};
    memcpy(caName,pdu->caData,32);
    QStringList res = OperateDB::getInstance().handleFlushOnlineUser(caName);
    qDebug() << "res"<<res.size();
    PDU* respdu = mkPDU(res.size()*32);
    for(int i = 0;i<res.size();i++){
        memcpy(respdu->caMsg+i*32,res[i].toStdString().c_str(),32);
    }
    respdu->uiType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
    return respdu;
}

PDU *MsgHandler::delFriend()
{
    char curName[32] = {'\0'};
    memcpy(curName,pdu->caData,32);
    char tarName[32] = {'\0'};
    memcpy(tarName,pdu->caData+32,32);
    bool ret = OperateDB::getInstance().handleDelFriend(curName,tarName);
    qDebug()<<"delFriend ret"<<ret;
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(ret));
    respdu->uiType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
    return respdu;
}

PDU *MsgHandler::chat()
{
    char tarName[32] = {'\0'};
    memcpy(tarName,pdu->caData+32,32);
    pdu->uiType = ENUM_MSG_TYPE_CHAT_RESPOND;
    MyTcpServer::getInstance().resend(tarName,pdu);
    return NULL;
}

PDU *MsgHandler::mkDir()
{
    QDir dir;
    QString strPath = QString("%1/%2").arg(pdu->caMsg).arg(pdu->caData);
    qDebug()<<"mkdir path"<<strPath;
    bool ret;
    if(dir.exists(strPath)){
        ret = false;
        qDebug()<<"文件夹名已存在";
    }
    ret = dir.mkdir(QString("%1/%2").arg(pdu->caMsg).arg(pdu->caData));
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(ret));
    respdu->uiType = ENUM_MSG_TYPE_MKDIR_RESPOND;
    return respdu;
}

PDU *MsgHandler::flushFile()
{
    qDebug()<<"flushFile path"<<pdu->caMsg;
    QDir dir(pdu->caMsg);
    QFileInfoList fileInfoList = dir.entryInfoList();
    QList<QFileInfo> validFiles;
    foreach(QFileInfo info, fileInfoList){
        QString fileName = info.fileName();
        if(fileName != "." && fileName != ".."){
            validFiles.append(info);
        }
    }
    int iFileCount = validFiles.size();
    PDU* respdu = mkPDU(iFileCount*sizeof(FileInfo));
    respdu->uiType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
    for(int i = 0;i<iFileCount;i++){
        FileInfo* pFileInfo = (FileInfo*)respdu->caMsg+i;
        memcpy(pFileInfo->caName,validFiles[i].fileName().toStdString().c_str(),32);
        if(validFiles[i].isDir()){
            pFileInfo->iType = 0;
        }
        else{
            pFileInfo->iType = 1;
        }
        qDebug()<<"pFileInfo->caName"<<pFileInfo->caName;
    }
    return respdu;
}

PDU *MsgHandler::moveFile()
{
    int iSrcPathLen = 0;
    int iTarPathLen = 0;

    memcpy(&iSrcPathLen,pdu->caData,sizeof(int));
    memcpy(&iTarPathLen,pdu->caData+sizeof(int),sizeof(int));

    char* strSrcPath = new char[iSrcPathLen + 1];
    memset(strSrcPath, '\0', iSrcPathLen + 1);

    char* strTarPath = new char[iTarPathLen + 1];
    memset(strTarPath, '\0', iTarPathLen + 1);

    memcpy(strSrcPath,pdu->caMsg,iSrcPathLen);
    memcpy(strTarPath,pdu->caMsg+iSrcPathLen,iTarPathLen);

    qDebug()<<"strSrcPath"<< strSrcPath <<"strTarPath"<< strTarPath;

    bool ret = QFile::rename(strSrcPath,strTarPath);
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(ret));
    respdu->uiType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
    delete [] strSrcPath;
    delete [] strTarPath;
    strSrcPath = NULL;
    strTarPath = NULL;
    return respdu;
}

PDU *MsgHandler::uploadFile()
{
    m_iReceiveSzie = 0;
    char caFileName[32] = {'\0'};
    memcpy(caFileName,pdu->caData,32);
    memcpy(&m_iUploadFileSize,pdu->caData+32,sizeof(qint64));
    QString strPath = QString("%1/%2").arg(pdu->caMsg).arg(caFileName);
    m_fUploadFile.setFileName(strPath);
    bool ret = m_fUploadFile.open(QIODevice::WriteOnly);
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(ret));
    respdu->uiType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
    return respdu;
}

PDU *MsgHandler::uploadFileData()
{
    m_fUploadFile.write(pdu->caMsg,pdu->uiMsgLen);
    m_iReceiveSzie += pdu->uiMsgLen;
    if(m_iReceiveSzie < m_iUploadFileSize){
        return NULL;
    }
    m_fUploadFile.close();
    PDU* respdu = mkPDU();
    respdu->uiType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_RESPOND;
    bool ret = m_iReceiveSzie == m_iUploadFileSize;
    memcpy(respdu->caData,&ret,sizeof (ret));
    return respdu;
}

PDU *MsgHandler::shareFlie()
{
    char strCurName[32] = {'\0'};
    int friend_num = 0;
    memcpy(strCurName,pdu->caData,32);
    memcpy(&friend_num,pdu->caData+32,sizeof(int));
    qDebug()<<"strCurName"<<strCurName;
    qDebug()<<"friend_num"<<friend_num;
    int size = friend_num*32;
    PDU* resendpdu = mkPDU(pdu->uiMsgLen - size);
    resendpdu->uiType = pdu->uiType;
    memcpy(resendpdu->caData,strCurName,32);
    memcpy(resendpdu->caMsg,pdu->caMsg+size,pdu->uiMsgLen-size);
    char caRecvName[32] = {'\0'};
    for(int i=0;i<friend_num;i++){
        memcpy(caRecvName,pdu->caMsg+i*32,32);
        qDebug()<<"caRecvName"<<caRecvName;
        MyTcpServer::getInstance().resend(caRecvName,resendpdu);
    }
    free(resendpdu);
    resendpdu = NULL;
    PDU* respdu = mkPDU();
    respdu->uiType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
    return respdu;
}

PDU *MsgHandler::shareFlieAgree()
{
    QString strRecvPath = QString("%1/%2").arg(Server::getInstance().m_strRootPath).arg(pdu->caData);
    QString strShareFilePath = pdu->caMsg;
    int index = strShareFilePath.lastIndexOf('/');
    QString strFileName = strShareFilePath.right(strShareFilePath.size()-index-1);
    strRecvPath = strRecvPath+ '/' + strFileName;
    QFileInfo fileInfo(strShareFilePath);
    qDebug()<<"strShareFilePath"<<strShareFilePath
            <<"strRecvPath"<<strRecvPath;
    bool ret = true;
    if(fileInfo.isFile()){
        ret = QFile::copy(strShareFilePath,strRecvPath);
        qDebug()<<"shareFlieAgree ret:"<<ret;
    }else if(fileInfo.isDir()){
        ret = copyDir(strShareFilePath,strRecvPath);
    }
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(bool));
    respdu->uiMsgLen = ENUM_MSG_TYPE_SHARE_FILE_AGREE_RESPOND;
    return respdu;
}

bool MsgHandler::copyDir(QString strSrcDir, QString strDestDir)
{
    QDir dir;
    dir.mkdir(strDestDir);
    dir.setPath(strSrcDir);
    QFileInfoList fileInfoList = dir.entryInfoList();
    bool ret = true;
    QString srcTmp;
    QString destTmp;
    for(int i=0;i<fileInfoList.size();i++){
        if(fileInfoList[i].isFile()){
            srcTmp = strSrcDir + '/' + fileInfoList[i].fileName();
            destTmp = strDestDir+ '/' + fileInfoList[i].fileName();
            if(!QFile::copy(srcTmp,destTmp)){
                ret = false;
            }else if(fileInfoList[i].isDir()){
                if(fileInfoList[i].fileName() == QString(".") || fileInfoList[i].fileName() == QString("..")){
                    continue;
                }
            }
            srcTmp = strSrcDir + '/' + fileInfoList[i].fileName();
            destTmp = strDestDir + '/' + fileInfoList[i].fileName();
            if(!copyDir(srcTmp,destTmp)){
                ret = false;
            }
        }
    }
    return ret;
}

