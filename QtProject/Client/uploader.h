#ifndef UPLOADER_H
#define UPLOADER_H

#include "protocol.h"

#include <QObject>

class Uploader : public QObject
{
    Q_OBJECT
public:
    Uploader(QString strUploadFilePath);
    void start();
    QString m_strUploadFilePath;
public slots:
    void uploadFile();

signals:
    void finished();
    void errorMsgBox(QString strError);
    void uploadPDU(PDU* pdu);

};

#endif // UPLOADER_H
