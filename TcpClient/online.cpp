#include "online.h"
#include "ui_online.h"
#include"QDebug"
#include"tcpclient.h"

Online::Online(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Online)
{
    ui->setupUi(this);
}

Online::~Online()
{
    delete ui;
}

void Online::showUsr(PDU *pdu)
{
    if(pdu == NULL)
    {
        return;
    }
    uint uiSize=pdu->uiMsgLen/32; //在线用户数量
    ui->online_lw->clear(); //先清空在线用户列表
    char caTmp[32];
    for(uint i=0;i<uiSize;i++)
    {
        char caName[32];
        memcpy(caTmp,(char*)(pdu->caMsg)+i*32,32);
        ui->online_lw->addItem(caTmp);

    }
}

void Online::on_addFriend_pb_clicked()
{
    QListWidgetItem *pItem= ui->online_lw->currentItem();
    QString strPerUsrName =pItem->text();   //获取选中的在线用户名
    QString strLoginName = TcpClient::getInstance().loginName();  //获取登录用户名
    PDU *pdu=mkPDU(0);
    pdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;

    //把选中的在线用户名复制到协议数据单元的消息部分
    memcpy(pdu->caData,strPerUsrName.toStdString().c_str(),strPerUsrName.size());
    //把登录用户名复制到协议数据单元的消息部分
    memcpy(pdu->caData+32,strLoginName.toStdString().c_str(),strLoginName.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu=NULL;
}

