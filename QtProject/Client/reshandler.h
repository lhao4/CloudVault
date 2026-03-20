#ifndef RESHANDLER_H
#define RESHANDLER_H

#include "protocol.h"

#include <QString>



class ResHandler
{
public:
    ResHandler();
    PDU* pdu;
    void regist();
    void login();
    void findUser();
    void onlineUser(QString& strLoginName);
    void addFriend();
    void addFriendResend();
    void agreeAddFriend();
    void flushFriend();
    void delFriend();
    void chat();
    void mkDir();
    void flushFile();
    void moveFile();
    void uploadFile();
    void uploadFileData();
    void shareFile();
    void shareFileRequest();
    void shareFileResult();
};

#endif // RESHANDLER_H
