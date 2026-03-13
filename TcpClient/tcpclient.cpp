#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QByteArray>
#include <QDebug.h>
#include <QMessageBox>
#include <QHostAddress>
#include"privatechat.h"

TcpClient::TcpClient(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TcpClient)
{
    ui->setupUi(this);

    resize(600,300);
    //加载配置文件
    loadConfig();

    //当连接服务器成功时，触发connected()信号，调用showConnect()槽函数显示连接成功的消息框
    connect(&m_tcpSocket,SIGNAL(connected()),this,SLOT(showConnect()));
    //当服务器有数据发送过来时，触发readyRead()信号，调用recvMsg()槽函数处理服务器发送过来的数据
    connect(&m_tcpSocket,SIGNAL(readyRead()),this,SLOT(recvMsg()));
    m_tcpSocket.connectToHost(QHostAddress(m_strIP),m_usPort);    //连接服务器

}

TcpClient::~TcpClient()
{
    delete ui;
}

void TcpClient::loadConfig()
{

    // : 代表读取的是资源文件
    // 设置文件路径
    QFile file(":/client.config");
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray baData=file.readAll();
        QString strData=baData.toStdString().c_str();
        file.close();

        strData.replace("\r\n"," ");

        QStringList strList=strData.split(" ");

        m_strIP = strList.at(0);
        m_usPort = strList.at(1).toUShort();
        qDebug() <<"ip:" << m_strIP<< "port:"<<m_usPort;
    }
    else
    {
        QMessageBox::critical(this,"错误","加载配置文件错误");
    }
}

TcpClient &TcpClient::getInstance()
{
    static TcpClient instance;
    return instance;
}

QTcpSocket &TcpClient::getTcpSocket()
{
    return m_tcpSocket;
}

QString TcpClient::loginName()
{
    return m_strLoginName;
}

QString TcpClient::curPath()
{
    return m_strCurPath;
}

void TcpClient::showConnect()
{
    QMessageBox::information(this,"连接服务器","连接服务器成功");
}

void TcpClient::recvMsg()
{
    // 如果是文件下载状态中
    if(OpeWidget::getInstance().getBook()->getDownloadStatus())
    {
        QByteArray buffer = m_tcpSocket.readAll();
        // 简化命名使用
        Book *pBook = OpeWidget::getInstance().getBook();
        pBook->m_pFile.write(buffer);
        pBook->m_iRevice += buffer.size();
        if(pBook->m_iTotal == pBook->m_iRevice)
        {
            pBook->m_pFile.close();
            pBook->m_iTotal = 0;
            pBook->m_iRevice = 0;
            pBook->setDownloadStatus(false);
            QMessageBox::critical(this, "下载文件", "下载文件成功");
        }
        else if(pBook->m_iTotal < pBook->m_iRevice)
        {
            pBook->m_pFile.close();
            pBook->m_iTotal = 0;
            pBook->m_iRevice = 0;
            pBook->setDownloadStatus(false);
            QMessageBox::critical(this, "下载文件", "下载文件失败：传输的文件错误");
        }
        //其他情况代表数据还未下载完成
        return ;
    }
    uint uiPDULen=0;
    m_tcpSocket.read((char *)&uiPDULen,sizeof(uint)); // 读取协议数据单元的长度
    uint uiMsgLen=uiPDULen-sizeof(PDU); // 计算实际消息的长度:sizeof(PDU)只会计算结构体大小，而不是分配的大小
    PDU *pdu=mkPDU(uiMsgLen);
    m_tcpSocket.read((char*)pdu+sizeof(uint),uiPDULen-sizeof(uint));
    switch (pdu->uiMsgType)
    {
    case ENUM_MSG_TYPE_REGIST_RESPOND: //注册响应
    {
        if(0 == strcmp(pdu->caData,REGIST_OK))
        {
            QMessageBox::information(this,"注册",REGIST_OK);
        }
        else if((0 == strcmp(pdu->caData,REGIST_FAILED)))
        {
            QMessageBox::warning(this,"注册",REGIST_FAILED);
        }
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_RESPOND: //登录响应
    {
        if(0 == strcmp(pdu->caData,LOGIN_OK))
        {
            m_strCurPath = QString("./%1").arg(m_strLoginName); // 设置当前路径为登录用户的目录
            QMessageBox::information(this,"登录",LOGIN_OK);
            OpeWidget::getInstance().show();        //显示操作界面
            hide();                                 //隐藏登录界面
        }
        else if((0 == strcmp(pdu->caData,LOGIN_FAILED)))
        {
            QMessageBox::warning(this,"登录",LOGIN_FAILED);
        }
        break;
    }
    case ENUM_MSG_TYPE_ALL_ONLINE_RESPOND: //在线用户响应
    {
        OpeWidget::getInstance().getFriend()->showAllOnlineUsr(pdu); //显示在线用户
    }
    case ENUM_MSG_TYPE_SEARCH_USR_RESPOND: //搜索用户响应
    {
        if(0 == strcmp(pdu->caData,SEARCH_USR_NO))
        {
            QMessageBox::information(this,"搜索",QString("%1: not exist").arg(OpeWidget::getInstance().getFriend()->m_strSearchName));
        }
        else if(0 == strcmp(pdu->caData,SEARCH_USR_ONLINE))
        {
            QMessageBox::information(this,"搜索",QString("%1: online").arg(OpeWidget::getInstance().getFriend()->m_strSearchName));
        }
        else if(0 == strcmp(pdu->caData,SEARCH_USR_OFFLINE))
        {
            QMessageBox::information(this,"搜索",QString("%1: offline").arg(OpeWidget::getInstance().getFriend()->m_strSearchName));
        }
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST: //添加好友请求
    {
        char caName[32]={'\0'};
        strncpy(caName,pdu->caData+32,32);
        int ret=QMessageBox::question(this,"添加好友",QString("%1 want to add you as friend.").arg(caName), QMessageBox::Yes, QMessageBox::No);
        PDU *respdu=mkPDU(0);
        memcpy(respdu->caData,pdu->caData,64); //把请求添加好友的用户名和登录用户名复制到响应协议数据单元的消息部分，服务器需要这两个用户名来处理添加好友的请求
        if(ret == QMessageBox::Yes)
        {
            respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_AGREE;
        }
        else if(ret == QMessageBox::No)
        {
            respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_REFUSE;
        }
        m_tcpSocket.write((char*)respdu,respdu->uiPDULen);  //把同意或拒绝添加好友的消息发送给服务器
        free(respdu);
        respdu=NULL;
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND: //添加好友响应
    {
        QMessageBox::information(this,"添加好友",pdu->caData);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_AGREE: //同意添加好友
    {
        char friendName[32] = {'\0'};//好友的名称
        strncpy(friendName, pdu->caData, 32);
        QMessageBox::information(this, "添加好友", QString("添加好友成功！%1 同意了您的好友添加请求").arg(friendName));
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE: //拒绝添加好友
    {
        char friendName[32] = {'\0'};//好友的名称
        strncpy(friendName, pdu->caData, 32);
        QMessageBox::information(this, "添加好友", QString("添加好友失败！%1 拒绝了您的好友添加请求").arg(friendName));
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND: //刷新好友响应
    {
        OpeWidget::getInstance().getFriend()->updateFriendList(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST: //删除好友请求
    {
        char caName[32]={'\0'};
        strncpy(caName,pdu->caData+32,32);
        QMessageBox::information(this, "删除好友", QString("%1 已经删除了您的好友").arg(caName));
        break;
    }
    case ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND: //删除好友响应
    {
        QMessageBox::information(this,"删除好友",pdu->caData);
        break;
    }
    case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST: //私聊请求
    {
        if(PrivateChat::getInstance().isHidden())
        {
            PrivateChat::getInstance().show();
        }
        // 给最新的一个发送过来的好友发送消息
        char loginName[32] = {'\0'};
        memcpy(loginName, pdu->caData + 32, 32);
        QString strLoginName = QString(loginName);
        PrivateChat::getInstance().setChatName(strLoginName);
        PrivateChat::getInstance().updateMsg(pdu);
        break;
    }
    case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST: //群聊请求
    {
        OpeWidget::getInstance().getFriend()->updateGroupMsg(pdu);
        break;
    }
    case ENUM_MSG_TYPE_CREATE_DIR_RESPOND: //创建文件夹响应
    {
        QMessageBox::information(this,"创建文件夹",pdu->caData);
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_DIR_RESPOND: //刷新文件夹响应
    {
        OpeWidget::getInstance().getBook()->updateDirList(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DEL_DIR_RESPOND: //删除文件夹响应
    {
        QMessageBox::information(this,"删除文件夹",pdu->caData);
        break;
    }
    case ENUM_MSG_TYPE_RENAME_DIR_RESPOND: //重命名文件夹响应
    {
        QMessageBox::information(this,"重命名文件夹",pdu->caData);
        break;
    }
    case ENUM_MSG_TYPE_ENTER_DIR_RESPOND: //进入文件夹响应
    {
        if(0 == strcmp(pdu->caData,ENTER_DIR_OK))
        {
            setCurPath(OpeWidget::getInstance().getBook()->getEnterPath());
            OpeWidget::getInstance().getBook()->updateDirList(pdu); //更新目录列表
        }
        else if(0 == strcmp(pdu->caData,CUR_DIR_NOT_EXIST))
        {
            QMessageBox::warning(this,"进入文件夹","进入文件夹失败：文件夹不存在");
        }
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND: //上传文件响应
    {
        QMessageBox::information(this, "上传文件", pdu->caData);
        break;
    }
    case ENUM_MSG_TYPE_DEL_FILE_RESPOND:    //删除文件响应
    {
        QMessageBox::information(this, "删除文件", pdu->caData);
        break;
    }
    case ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND:    //下载文件响应
    {
        qDebug() << pdu->caData;
        // downloadFilePre(pdu);
        char caFileName[32] = {'\0'};
        sscanf(pdu->caData, "%s %lld", caFileName, &(OpeWidget::getInstance().getBook()->m_iTotal));
        // 如果是有效数据
        if(strlen(caFileName) > 0 && OpeWidget::getInstance().getBook()->m_iTotal > 0)
        {
            OpeWidget::getInstance().getBook()->setDownloadStatus(true);//标记开始下载文件
            OpeWidget::getInstance().getBook()->m_iRevice = 0;
            OpeWidget::getInstance().getBook()->updateLocalDownloadFileName();
            // 只写模式打开文件，文件如果不存在则会被创建
            if(!OpeWidget::getInstance().getBook()->m_pFile.open(QIODevice::WriteOnly))
            {
                QMessageBox::warning(this, "下载文件", "下载文件失败：本地文件无法操作");
                // 由于服务器已经开始发送下载文件的数据了，这个时候应该怎么办呢？【】
                return ;
            }
        }
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_RESPOND:  //分享文件响应
    {
        QMessageBox::information(this, "共享文件", pdu->caData);
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_NOTE: //收到共享文件通知响应
    {
        qDebug() << "开始准备接受文件";
        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath, (char*)pdu->caMsg, pdu->uiMsgLen);
        // aa/bb/cc/a.txt
        qDebug() << pPath;
        char *pos = strrchr(pPath, '/'); //找到最后一个 / 出现的位置
        qDebug() << pos;
        if(NULL != pos)
        {
            pos++; // 向右移动一位，因为 / 这个字符我们不需要，只需要文件名称，即a.txt
            QString strNote = QString("%1 share file -> %2\n do you accecpt").arg(pdu->caData).arg(pos);
            int ret = QMessageBox::question(this, "共享文件", strNote);
            if(QMessageBox::Yes == ret)
            {
                PDU *retPdu = mkPDU(pdu->uiMsgLen);
                retPdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE_REQUEST;
                memcpy(retPdu->caMsg, pdu->caMsg, pdu->uiMsgLen);
                QString strName = TcpClient::getInstance().loginName();
                strcpy(retPdu->caData, strName.toStdString().c_str());
                m_tcpSocket.write((char*)retPdu,retPdu->uiPDULen);
            }
        }

    }
    case ENUM_MSG_TYPE_MOVE_FILE_RESPOND:   //移动文件响应
    {
        QMessageBox::information(this, "移动文件", pdu->caData);
        break;
    }
    default:
        break;
    }

    free(pdu);  //释放空间
    pdu=NULL;
}


#if 0
void Tcpclient::on_pushButton_clicked()
{
    QString strMsg=ui->lineEdit->text();
    if(!strMsg.isEmpty())
    {
        PDU *pdu=mkPDU(strMsg.size());
        pdu->uiMsgType=1000;
        memcpy(pdu->caMsg,strMsg.toStdString().c_str(),strMsg.size());
        m_tcpSocket.write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
    else
    {
        QMessageBox::warning(this,"发送消息","消息不能为空");
    }
}
#endif

void TcpClient::on_login_pb_clicked()
{
    QString strName=ui->name_le->text();
    QString strPwd=ui->pwd_le->text();
    if(!strName.isEmpty() && !strPwd.isEmpty())
    {
        m_strLoginName=strName; //保存登录的用户名
        PDU *pdu=mkPDU(0);
        pdu->uiMsgType=ENUM_MSG_TYPE_LOGIN_REQUEST;
        strncpy(pdu->caData,strName.toStdString().c_str(),32);
        strncpy(pdu->caData+32,strPwd.toStdString().c_str(),32);
        m_tcpSocket.write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
    else
    {
        QMessageBox::critical(this,"登录","登录失败：用户名或密码为空");
    }
}


void TcpClient::on_regist_pb_clicked()
{
    QString strName=ui->name_le->text();
    QString strPwd=ui->pwd_le->text();
    if(!strName.isEmpty() && !strPwd.isEmpty())
    {
        PDU *pdu=mkPDU(0);
        pdu->uiMsgType=ENUM_MSG_TYPE_REGIST_REQUEST;
        strncpy(pdu->caData,strName.toStdString().c_str(),32);
        strncpy(pdu->caData+32,strPwd.toStdString().c_str(),32);
        m_tcpSocket.write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
    else
    {
        QMessageBox::critical(this,"注册","注册失败：用户名或密码为空");
    }
}


void TcpClient::on_cancel_pb_clicked()
{
    //注销

}



void TcpClient::setCurPath(QString strPath)
{
    m_strCurPath = strPath;
}

