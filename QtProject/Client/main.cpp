#include "client.h"
#include <QApplication>
#include "index.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Client::getInstance().show();
    return a.exec();
}

