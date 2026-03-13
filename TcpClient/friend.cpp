#include "friend.h"
#include "protocol.h"
#include "tcpclient.h"
#include<QInputDialog>
#include"privatechat.h"
#include<QMessageBox>

Friend::Friend(QWidget *parent)
    : QWidget{parent}
{
    m_pShowMsgTE = new QTextEdit;
    m_pFriendListWidget = new QListWidget;
    m_pInputMsgLE = new QLineEdit;

    m_pDelFriendPB = new QPushButton("删除好友");
    m_pFlushFriendPB = new QPushButton("刷新好友");
    m_pShowOnlineUserPB = new QPushButton("显示在线用户");
    m_pSearchUserPB = new QPushButton("查找用户");
    m_pMsgSendPB = new QPushButton("信息发送");
    m_pPrivateChatPB = new QPushButton("私聊");

    QVBoxLayout *pRightPBVBL = new QVBoxLayout;
    pRightPBVBL->addWidget(m_pDelFriendPB);
    pRightPBVBL->addWidget(m_pFlushFriendPB);
    pRightPBVBL->addWidget(m_pShowOnlineUserPB);
    pRightPBVBL->addWidget(m_pSearchUserPB);
    pRightPBVBL->addWidget(m_pPrivateChatPB);

    QHBoxLayout *pTopHBL = new QHBoxLayout;
    pTopHBL->addWidget(m_pShowMsgTE);
    pTopHBL->addWidget(m_pFriendListWidget);
    pTopHBL->addLayout(pRightPBVBL);

    QHBoxLayout *pMsgHBL = new QHBoxLayout;
    pMsgHBL->addWidget(m_pInputMsgLE);
    pMsgHBL->addWidget(m_pMsgSendPB);

    m_pOnline=new Online;

    QVBoxLayout *pMain = new QVBoxLayout;
    pMain->addLayout(pTopHBL);
    pMain->addLayout(pMsgHBL);
    pMain->addWidget(m_pOnline);
    m_pOnline->hide();   //默认隐藏在线用户窗口

    setLayout(pMain);   // 显示该布局

    connect(m_pShowOnlineUserPB,SIGNAL(clicked(bool)),this,SLOT(showOnline()));  //点击显示在线用户按钮，触发showOnline()槽函数
    connect(m_pSearchUserPB,SIGNAL(clicked(bool)),this,SLOT(searchUsr()));    //点击搜索用户按钮，触发searchUsr()槽函数
    connect(m_pFlushFriendPB,SIGNAL(clicked(bool)),this,SLOT(flushFriend()));    //点击刷新好友按钮，触发flushFriend()槽函数
    connect(m_pDelFriendPB,SIGNAL(clicked(bool)),this,SLOT(deleteFriend()));    //点击删除好友按钮，触发deleteFriend()槽函数
    connect(m_pPrivateChatPB,SIGNAL(clicked(bool)),this,SLOT(privateChat()));    //点击私聊按钮，触发privateChat()槽函数
    connect(m_pMsgSendPB,SIGNAL(clicked(bool)),this,SLOT(groupChat()));    //点击群聊按钮，触发groupChat()槽函数
}

void Friend::showAllOnlineUsr(PDU *pdu)
{
    if(pdu == NULL)
    {
        return;
    }
    m_pOnline->showUsr(pdu);
}

void Friend::updateFriendList(PDU * pdu)
{
    if(NULL == pdu)
    {
        return ;
    }
    m_pFriendListWidget->clear();
    uint uiSize = pdu->uiMsgLen / 32; //好友数量，每个好友名称占32字节
    for(int i = 0; i < uiSize; i++)
    {
        char caName[32] = {'\0'};
        // 注意，这里的 pdu->caMsg 也应该是 char * 才对，因为他复制的是 char *的内容
        memcpy(caName, (char*)(pdu->caMsg) + i * 32, 32);
        m_pFriendListWidget->addItem(caName);
    }
}

void Friend::updateGroupMsg(PDU *pdu)
{
    // char caLoginName[32] = {'\0'};
    // memcpy(caLoginName, pdu->caData, 32);
    // m_pShowMsgTE->append(QString("%1 says: %2").arg(caLoginName).arg((char*)pdu->caMsg));
    QString strMsg = QString("%1 says: %2").arg(QString(pdu->caData)).arg(QString((char*)pdu->caMsg));
    m_pShowMsgTE->append(strMsg);
}

QListWidget *Friend::getFriendList()
{
    return m_pFriendListWidget;
}

void Friend::showOnline()
{
    if(m_pOnline->isHidden())
    {
        m_pOnline->show();

        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_REQUEST;
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
        m_pShowOnlineUserPB->setText("隐藏在线用户");

    }
    else
    {
        m_pOnline->hide();
        m_pShowOnlineUserPB->setText("显示在线用户");
    }
}

void Friend::searchUsr()
{
    m_strSearchName=QInputDialog::getText(this,"搜索","用户名:");   //弹出输入框，获取用户输入的用户名
    if(!m_strSearchName.isEmpty())
    {
        qDebug()<<m_strSearchName;
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_REQUEST;
        strcpy(pdu->caData, m_strSearchName.toStdString().c_str());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
}

void Friend::flushFriend()
{
    QString strName=TcpClient::getInstance().loginName();  //获取登录用户名
    PDU *pdu=mkPDU(0);
    pdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST;
    strcpy(pdu->caData, strName.toStdString().c_str());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Friend::deleteFriend()
{
    QListWidgetItem *item = m_pFriendListWidget->currentItem();
    if(NULL == item)
    {
        return ;
    }
    QString friendName = item->text();
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST;
    QString loginName = TcpClient::getInstance().loginName();
    memcpy(pdu->caData, friendName.toStdString().c_str(), friendName.size());
    memcpy(pdu->caData + 32, loginName.toStdString().c_str(), loginName.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Friend::privateChat()
{
    QListWidgetItem *item = m_pFriendListWidget->currentItem();
    if(NULL == item)
    {
        QMessageBox::warning(this, "私聊", "请选择私聊对象");
        return ;
    }
    else
    {
        QString friendName = item->text();
        PrivateChat::getInstance().setChatName(friendName);
        if(PrivateChat::getInstance().isHidden())
        {
            PrivateChat::getInstance().show();
        }
    }
}

void Friend::groupChat()
{
    QString msg = m_pInputMsgLE->text();
    if(msg.isEmpty())
    {
        QMessageBox::warning(this, "群聊", "发送消息不能为空");
        return;
    }
    m_pInputMsgLE->clear();
    PDU *pdu = mkPDU(msg.toUtf8().size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_GROUP_CHAT_REQUEST;
    QString strName=TcpClient::getInstance().loginName();
    strncpy(pdu->caData, strName.toStdString().c_str(), strName.size());
    strncpy((char *)pdu->caMsg, msg.toStdString().c_str(), msg.toUtf8().size());
    TcpClient::getInstance().getTcpSocket().write((char *)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}



