#ifndef CLIENTTASK_H
#define CLIENTTASK_H

#include "mytcpsocket.h"

#include <QObject>
#include <QRunnable>

class ClientTask : public QObject,public QRunnable
{
    Q_OBJECT
public:
    ClientTask(MyTcpSocket* socket);
    void run() override;
    MyTcpSocket* socket;

signals:

};

#endif // CLIENTTASK_H
