#ifndef OPERATEDB_H
#define OPERATEDB_H

#include <QObject>
#include <QSqlDatabase>

class OperateDB : public QObject
{
    Q_OBJECT
public:

    QSqlDatabase m_db;
    ~OperateDB();
    void connect();
    static OperateDB& getInstance();
    // 注册功能：返回注册是否成功
    bool handleRegist(const char* caName, const char* caPwd);
    // 登录功能：返回登录是否成功
    bool handleLogin(const char* caName, const char* caPwd);
    // 离线处理
    void handleOfflint(const char* caName);
    //查找用户
    int handleFindUser(const char* caName);
    //在线用户
    QStringList handleOnlineUser();
    //处理添加好友
    int handleAddFriend(const char* curName,const char* tarName);
    //是否成功添加好友
    bool handleAgreeAddFriend(const char* curName,const char* tarName);
    QStringList handleFlushOnlineUser(const char* caName);
    bool handleDelFriend(const char* curName,const char* tarName);
private:
    explicit OperateDB(QObject *parent = nullptr);
    OperateDB(const OperateDB& instance) = delete;
    OperateDB& operator=(const OperateDB&) = delete;


signals:

};

#endif // OPERATEDB_H
