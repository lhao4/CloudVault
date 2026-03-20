#ifndef SERVER_H
#define SERVER_H

#include <QWidget>
#include <QDebug>
#include <QFile>


QT_BEGIN_NAMESPACE
namespace Ui { class Server; }
QT_END_NAMESPACE

class Server : public QWidget
{
    Q_OBJECT

public:
    static Server& getInstance();
    ~Server();
    void loadConfig();
    QString m_strIP;
    quint16 m_usport;
    QString m_strRootPath;

private:
    Ui::Server *ui;
    Server(QWidget *parent = nullptr);
    Server(const Server& instance) = delete;
    Server& operator=(const Server&) = delete;

};
#endif // SERVER_H
