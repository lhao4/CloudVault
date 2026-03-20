#include "operatedb.h"
#include "server.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Server::getInstance();
    OperateDB::getInstance().connect();
    return a.exec();
}
