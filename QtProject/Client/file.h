#ifndef FILE_H
#define FILE_H

#include "protocol.h"
#include "sharefile.h"

#include <QWidget>
#include <qlistwidget.h>

namespace Ui {
class File;
}

class File : public QWidget
{
    Q_OBJECT

public:
    explicit File(QWidget *parent = nullptr);
    ~File();
    QString m_strUserPath;
    QString m_strCurPath;
    void flushFile();
    void updateFileList(QList<FileInfo*> pFileList);
    QList<FileInfo*> m_pFileList;
    QString m_strMvName;
    QString m_strMvPath;
    QString strSrcPath;
    QString strTarPath;
    QString m_strUploadFilePath;
    //QSet<QString> m_uploadedHashes;  // 本地去重哈希集合
    void uploadFile();
    ShareFile* m_pShareFile;
    QString m_strShareFileName;

private slots:
    void on_mkDir_PB_clicked();

    void on_flushFile_PB_clicked();

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_return_PB_clicked();

    void on_mvFile_PB_clicked();

    void on_uploadFile_PB_clicked();

    void on_shareFile_PB_clicked();

public slots:
    void uploadError(QString strError);

private:
    Ui::File *ui;
};

#endif // FILE_H
