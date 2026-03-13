#include"protocol.h"

PDU *mkPDU(uint uiMsgLen)
{
    uint uiPDuLen=sizeof(PDU)+uiMsgLen;
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
