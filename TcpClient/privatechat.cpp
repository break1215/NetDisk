#include "privatechat.h"
#include "ui_privatechat.h"
#include "protocol.h"
#include "tcpclient.h"
#include <QMessageBox>

PrivateChat::PrivateChat(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PrivateChat)
{
    ui->setupUi(this);
}

PrivateChat::~PrivateChat()
{
    delete ui;
}

PrivateChat &PrivateChat::getInstance()
{
    static PrivateChat instance;
    return instance;
}

void PrivateChat::setChatName(QString &strName)
{
    m_strChatName = strName;
    m_strLoginName = TcpClient::getInstance().loginName();
}

void PrivateChat::updateMsg(const PDU *pdu)
{
    if(pdu == NULL)
    {
        return;
    }
    char caLoginName[32] = {'\0'};
    memcpy(caLoginName, pdu->caData + 32, 32);
    QString msg = QString("%1 says: %2").arg(caLoginName).arg((char*)pdu->caMsg);
    ui->showMsg_te->append(msg);
}

void PrivateChat::on_sendMsg_pb_clicked()
{
    QString strMsg=ui->inputMsg_le->text(); //获取输入框中的消息
    ui->inputMsg_le->clear(); //清空输入框

    if(!strMsg.isEmpty())
    {
        PDU *pdu=mkPDU(strMsg.toUtf8().size()+1); //+1是因为转为char后需要多一个 \0结束符
        pdu->uiMsgType=ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST;

        // 前32位是聊天对象名称，后32位是登录者名称
        memcpy(pdu->caData, m_strChatName.toStdString().c_str(), m_strChatName.size());
        memcpy(pdu->caData + 32, m_strLoginName.toStdString().c_str(), m_strLoginName.size());
        strcpy((char*)pdu->caMsg, strMsg.toStdString().c_str()); //把要发送的消息复制到协议数据单元的消息部分

        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
    else
    {
        QMessageBox::warning(this,"私聊","发送的消息不能为空");
    }
}

