#include "client.h"
#include "file.h"
#include "ui_file.h"
#include "uploader.h"

#include <QCryptographicHash>
#include <QFileDialog>
#include <qdir.h>
#include <qinputdialog.h>
#include <qmessagebox.h>

File::File(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::File)
{
    ui->setupUi(this);
    m_strUserPath = QString("%1/%2").arg(Client::getInstance().m_strRootPath).arg(Client::getInstance().m_strLoginName);
    m_strCurPath = m_strUserPath;
    flushFile();
    m_pShareFile = new ShareFile;
}

File::~File()
{
    delete ui;
    delete m_pShareFile;
}

void File::flushFile()
{
    PDU* pdu = mkPDU(m_strCurPath.toStdString().size()+1);
    pdu->uiType = ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
    memcpy(pdu->caMsg,m_strCurPath.toStdString().c_str(),m_strCurPath.toStdString().size());
    Client::getInstance().sendMsg(pdu);
}

void File::updateFileList(QList<FileInfo*> pFileList)
{
    foreach(FileInfo* pFileInfo,m_pFileList){
        delete pFileInfo;
    }
    m_pFileList = pFileList;
    ui->listWidget->clear();
    foreach(FileInfo* pFileInfo,pFileList){
        QListWidgetItem* pItem = new QListWidgetItem;
        if(pFileInfo->iType == 0){
            pItem->setIcon(QIcon(QPixmap(":/images/folder.png")));
        }else if(pFileInfo->iType == 1){
            pItem->setIcon(QIcon(QPixmap(":/images/file.png")));
        }
        pItem->setText(pFileInfo->caName);
        ui->listWidget->addItem(pItem);
    }
}

void File::on_mkDir_PB_clicked()
{
    QString strNewDir = QInputDialog::getText(this,"提示","新建文件夹名");
    if(strNewDir.toStdString().size() == 0 || strNewDir.toStdString().size()>32){
        QMessageBox::information(this,"提示","文件夹名长度非法");
        return;
    }
    PDU* pdu = mkPDU(m_strCurPath.toStdString().size()+1);
    pdu->uiType = ENUM_MSG_TYPE_MKDIR_REQUEST;
    memcpy(pdu->caData,strNewDir.toStdString().c_str(),32);
    memcpy(pdu->caMsg,m_strCurPath.toStdString().c_str(),m_strCurPath.toStdString().size());
    Client::getInstance().sendMsg(pdu);
}

void File::on_flushFile_PB_clicked()
{
    flushFile();
}

void File::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    foreach(FileInfo* pFileInfo,m_pFileList){
        if(item->text() == pFileInfo->caName && pFileInfo->iType != 0){
            return;
        }
    }
    m_strCurPath = m_strCurPath + "/" + item->text();
    flushFile();
}

void File::on_return_PB_clicked()
{
    // 检查当前路径是否已经是用户根目录
//    if (m_strCurPath != m_strUserPath) {
//        // 找到最后一个 '/' 的位置
//        int lastIndex = m_strCurPath.lastIndexOf('/');
//        if (lastIndex != -1) {
//            // 截取到最后一个 '/' 之前的路径，即父目录
//            flushFile();
//        }
//    } else {
//        QMessageBox::information(this, "提示", "已经是根目录");
//    }
    if (m_strCurPath != m_strUserPath) {
        int lastIndex = m_strCurPath.lastIndexOf('/');
        m_strCurPath.remove(lastIndex,m_strCurPath.size() - lastIndex);
        flushFile();
    } else {
        QMessageBox::information(this, "提示", "已经是根目录");
    }
}

void File::on_mvFile_PB_clicked()
{
    //判断按钮上的文字，分为移动文件和确认/取消
    if(ui->mvFile_PB->text() == "移动文件"){
        QListWidgetItem* pItem = ui->listWidget->currentItem();
        if(pItem == NULL){
            return;
        }
        QMessageBox::information(this,"移动文件","请选择要移动到的目录");
        //修改按钮上的文字
        ui->mvFile_PB->setText("确认/取消");
        //记录选择的文件名以及该文件的当前路径
        m_strMvName = pItem->text();
        m_strMvPath = m_strCurPath;
        return;
    }
    //点击确认/取消时的逻辑，修改按钮的文字
    ui->mvFile_PB->setText("移动文件");
    //如果用户选择了目录就移动到该目录下，如果没有选择目录（没有选择或者选择的是文件）就移动到当前路径
    QListWidgetItem* pItem = ui->listWidget->currentItem();
    //获取要移动到的目录
    QString strTarPath;
    if(pItem == NULL){
        strTarPath = m_strCurPath;
    }else{
        strTarPath = m_strCurPath + "/" + pItem->text();
        foreach(FileInfo* pFileInfo,m_pFileList){
            if(pItem->text() == pFileInfo->caName && pFileInfo->iType != 0){
                strTarPath = m_strCurPath;
                break;
            }
        }
    }
    //询问用户是否确认移动
    int ret = QMessageBox::information(this,"移动文件",QString("是否确认移动到\n%1?").arg(strTarPath),"确认","取消");
    if(ret != 0){
        return;
    }
    //拼接文件的原完整路径和要移动到的完整路径
    strTarPath = strTarPath + "/" + m_strMvName;
    QString strSrcPath = m_strMvPath + "/" + m_strMvName;
    //两个路径放入caMsg，将两个路径的长度放入caData中，服务器根据路径的长度获取路径
    int iSrcPathLen = strSrcPath.toStdString().size();
    int iTarPathLen = strTarPath.toStdString().size();
    qDebug()<<"iSrcPathLen"<<iSrcPathLen<<"iTarPathLen"<<iTarPathLen;
    PDU* pdu = mkPDU(iSrcPathLen+iTarPathLen+1);
    pdu->uiType = ENUM_MSG_TYPE_MOVE_FILE_REQUEST;
    memcpy(pdu->caData,&iSrcPathLen,sizeof(int));
    memcpy(pdu->caData+sizeof(int),&iTarPathLen,sizeof(int));
    memcpy(pdu->caMsg,strSrcPath.toStdString().c_str(),iSrcPathLen);
    memcpy(pdu->caMsg+iSrcPathLen,strTarPath.toStdString().c_str(),iTarPathLen);
    Client::getInstance().sendMsg(pdu);
}

void File::on_uploadFile_PB_clicked()
{
    m_strUploadFilePath = QFileDialog::getOpenFileName(this,"选择文件","","所有文件(*.*)");
    qDebug()<<"selectedPath"<<m_strUploadFilePath;
    int index = m_strUploadFilePath.lastIndexOf("/");
    QString strFileName = m_strUploadFilePath.right(m_strUploadFilePath.size() - index -1);
    qDebug()<<"selectFileName"<<strFileName;
    QFile file(m_strUploadFilePath);
    qint64 iFileSize = file.size();
    PDU* pdu = mkPDU(m_strCurPath.toStdString().size());
    pdu->uiType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
    memcpy(pdu->caMsg, m_strCurPath.toStdString().c_str(), m_strCurPath.toStdString().size());
    memcpy(pdu->caData,strFileName.toStdString().c_str(),32);
    memcpy(pdu->caData + 32,&iFileSize,sizeof(qint64));
    Client::getInstance().sendMsg(pdu);
}

void File::uploadFile()
{
   Uploader* uploader = new Uploader(m_strUploadFilePath);
   connect(uploader,&Uploader::errorMsgBox,this,&File::uploadError);
   connect(uploader,&Uploader::uploadPDU,&Client::getInstance(),&Client::sendMsg);
   uploader->start();
}

void File::uploadError(QString strError)
{
    QMessageBox::warning(this,"提示",strError);
}


/*
void File::on_uploadFile_PB_clicked()
{
    //通过文件选择弹窗获取要上传文件的路径，路径作为file的属性存下来
    // 打开选择文件的对话框
    m_strUploadFilePath = QFileDialog::getOpenFileName(this,"选择文件","","所有文件(*.*)");
    if (m_strUploadFilePath.isEmpty()) {
        qDebug() << "未选择文件";
        return;
    }
    qDebug()<<"selectedPath"<<m_strUploadFilePath;
//    int lastIndexOf = selectedPath.lastIndexOf("/");
//    QString selectFileName = selectedPath.right(selectedPath.size() - lastIndexOf -1);
//    qDebug()<<"selectFileName"<<selectFileName;
    QFileInfo fileInfo(m_strUploadFilePath);
    QString selectFileName = fileInfo.fileName();
    qDebug() << "selectFileName:" << selectFileName;
    qint64 fileByte = fileInfo.size();
    qDebug() << "fileByte:" << fileByte;
    //计算文件MD5哈希值（用于去重和校验）
    QFile file(m_strUploadFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "打开文件失败：" << file.errorString();
        return;
    }
    QCryptographicHash hash(QCryptographicHash::Md5);
    const int bufferSize = 4096;
    char buffer[bufferSize];
    while (!file.atEnd()) {
        qint64 bytesRead = file.read(buffer, bufferSize);
        if (bytesRead > 0) {
            hash.addData(buffer, bytesRead);
        }
    }
    file.close();
    QByteArray hashBytes = hash.result();
    QString fileHash = hashBytes.toHex();  // 32字节十六进制哈希字符串
    qDebug() << "文件哈希值：" << fileHash;
    //上传前检查是否重复（本地哈希集合判断）
    if (m_uploadedHashes.contains(fileHash)) {
        qDebug() << "文件已上传，无需重复上传";
        QMessageBox::information(this,"提示","该文件已上传，无需重复上传");
        return;
    }
    // 用 QByteArray 存储含 \0 的数据
    QByteArray caMsgData;
    caMsgData.append(m_strCurPath);
    caMsgData.append('\0');  // 路径终止符
    caMsgData.append(fileHash);  // 哈希值
//    int nullIndex = caMsgData.indexOf('\0');
//    if (nullIndex == -1) {
//        qDebug() << "未找到分隔符\x00，无法提取哈希";
//        return;
//    }
//    QByteArray hashPart = caMsgData.mid(nullIndex + 1);
//    if (hashPart.size() != 32) {
//        qDebug() << "哈希部分长度异常：" << hashPart.size() << "字节（预期32字节）";
//    }
//    qDebug() << "单独的哈希值：" << QString(hashPart);
//    qDebug() << "哈希值的十六进制：" << hashPart.toHex();  // 这里会显示哈希字符串每个字符的十六进制编码

    // 创建 PDU 时使用 QByteArray 的实际长度（含所有 \0 和哈希）
    PDU* pdu = mkPDU(caMsgData.size());
    pdu->uiType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
    // 复制完整数据（memcpy 会按指定长度复制，无视 \0）
    memcpy(pdu->caMsg, caMsgData.constData(), caMsgData.size());
    memcpy(pdu->caData,selectFileName.toStdString().c_str(),32);
    // 用QByteArray存储文件大小（8字节），剩余24字节补0
    QByteArray fileSizeData;
    QDataStream sizeStream(&fileSizeData, QIODevice::WriteOnly);
    sizeStream << fileByte;  // 写入8字节的fileByte（27770）
    fileSizeData.resize(32);  // 不足32字节自动补0
    memcpy(pdu->caData + 32, fileSizeData.constData(), 32);
    Client::getInstance().sendMsg(pdu);
    m_uploadedHashes.insert(fileHash);
}
*/

void File::on_shareFile_PB_clicked()
{
    QListWidgetItem* pItem = ui->listWidget->currentItem();
    if(pItem == NULL){
        return;
    }
    m_strShareFileName = pItem->text();
    m_pShareFile->updataFriend_LW();
    if(m_pShareFile->isHidden()){
        m_pShareFile->show();
    }
}
