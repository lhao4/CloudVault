#include "protocol.h"
#include<stdlib.h>
#include<string.h>
PDU *mkPDU(uint uiMsgLen)
{
    //计算总长度，申请空间
    uint uiPDULen = sizeof(PDU) + uiMsgLen;
    PDU* pdu = (PDU*)malloc(uiPDULen);
    if(pdu == NULL){
        exit(1);
    }
    memset(pdu,0,uiPDULen);
    //两个长度的成员赋初值
    pdu->uiPDULen = uiPDULen;
    pdu->uiMsgLen = uiMsgLen;
    return pdu;
}
