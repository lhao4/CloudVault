#include "uploader.h"

#include <QFile>
#include <QThread>

Uploader::Uploader(QString strUploadFilePath)
{
    m_strUploadFilePath = strUploadFilePath;
}

void Uploader::start()
{
    QThread* thread = new QThread;
    this->moveToThread(thread);
    connect(thread,&QThread::started,this,&Uploader::uploadFile);
    connect(this,&Uploader::finished,thread,&QThread::quit);
    connect(thread,&QThread::finished,thread,&QThread::deleteLater);
    thread->start();
}

void Uploader::uploadFile()
{
    QFile file(m_strUploadFilePath);
    if(!file.open(QIODevice::ReadOnly)){
        emit errorMsgBox("上传文件:打开文件失败");
        emit finished();
        return;
    }
    while (true) {
        PDU* pdu = mkPDU(4096);
        pdu->uiType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_REQUEST;
        qint64 ret = file.read(pdu->caMsg,4094);
        if(ret == 0){
            break;
        }
        if(ret < 0){
            emit errorMsgBox("上传文件:读取文件失败");
            break;
        }
        pdu->uiMsgLen = ret;
        pdu->uiPDULen = ret + sizeof(PDU);
        emit uploadPDU(pdu);
    }
    file.close();
    emit finished();
}
