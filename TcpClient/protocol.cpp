#include"protocol.h"

PDU *mkPDU(uint uiMsgLen)
{
    uint uiPDuLen=sizeof(PDU)+uiMsgLen; //协议数据单元的总长度=协议数据单元结构体的大小+实际消息的长度
    PDU *pdu=(PDU *)malloc(uiPDuLen);
    if(NULL==pdu)
    {
        exit(EXIT_FAILURE);
    }
    memset(pdu,0,uiPDuLen);
    pdu->uiPDULen=uiPDuLen;
    pdu->uiMsgLen=uiMsgLen;
    return pdu;
}
