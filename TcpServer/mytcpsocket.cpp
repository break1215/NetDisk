#include "mytcpsocket.h"
#include<QDebug>
#include"mytcpserver.h"
#include<QDir>
#include<QFileInfoList>
#include<QFileInfo>


MyTcpSocket::MyTcpSocket()
{

    m_pTimer = new QTimer;  //定时器对象，用于上传文件时控制发送文件数据的速度，防止发送过快导致粘包问题
    m_bUpload = false;//定义最开始不是上传文件的状态

    connect(this,SIGNAL(readyRead()),this,SLOT(recvMsg()));  //当套接字对象有数据可读时，触发readyRead()信号，调用recvMsg()槽函数来处理接收到的数据
    connect(this,SIGNAL(disconnected()),this,SLOT(clientOffline())); //当套接字对象断开连接时，触发disconnected()信号，调用clientOffline()槽函数来处理用户离线的情况
    connect(m_pTimer, SIGNAL(timeout()),this, SLOT(sendFileDataToClient()));  //当定时器超时时，触发timeout()信号，调用sendFileDataToClient()槽函数来发送文件数据给客户端
}


QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::recvMsg()
{
    // 如果是上传文件
    if(m_bUpload)
    {
        // 上传的文件过大可能会导致客户端崩溃
        // 这里需要readAll多次，因为可能文件还没有发送完成
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
        // 写入文件
        QByteArray buff = readAll();
        m_file.write(buff);
        m_iRecved += buff.size();

        qDebug() << QString("第 %1 次传入文件,接受数据大小:%2").arg(m_iCount).arg(buff.size());
        m_iCount++;
        // 读取文件内容完成，向客户端发送信息上传成功
        if(m_iTotal == m_iRecved)
        {
            m_bUpload = false;
            m_file.close();
            strcpy(pdu->caData, UPLOAD_FILE_OK);
            write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
        // 如果接受到的数据大小超过了文件的总大小，说明上传过程中发生了错误，可能是网络问题或者客户端发送了错误的数据,向客户端发送信息提示结束上传
        else if(m_iTotal < m_iRecved)
        {
            m_bUpload = false;
            m_file.close();
            strcpy(pdu->caData, UPLOAD_FILE_FAIL);
            write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
        // 其他情况代表文件上传未完成，等待结束数据
        return ;
    }
    uint uiPDULen=0;   //协议数据单元的长度
    this->read((char *)&uiPDULen,sizeof(uint)); //从套接字中读取协议数据单元的长度，存储在uiPDULen变量中
    uint uiMsgLen=uiPDULen-sizeof(PDU); //协议数据单元的消息部分的长度，等于协议数据单元的总长度减去协议数据单元头部的长度
    PDU *pdu=mkPDU(uiMsgLen);
    this->read((char*)pdu+sizeof(uint),uiPDULen-sizeof(uint));
    switch (pdu->uiMsgType)
    {
    case ENUM_MSG_TYPE_REGIST_REQUEST:
    {
        char caName[32]={'\0'};
        char caPwd[32]={'\0'};
        strncpy(caName,pdu->caData,32);
        strncpy(caPwd,pdu->caData+32,32);
        bool ret=OpeDB::getInstance().handleRegist(caName,caPwd);
        PDU *respdu=mkPDU(0);
        respdu->uiMsgType=ENUM_MSG_TYPE_REGIST_RESPOND; //注册响应
        if(ret)
        {
            strcpy(respdu->caData,REGIST_OK);
            QDir dir;
            qDebug()<<"create dir"<<dir.mkdir(QString("./%1").arg(caName)); //为注册成功的用户创建一个以用户名命名的目录
        }
        else
        {
            strcpy(respdu->caData,REGIST_FAILED);
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu=NULL;
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_REQUEST:   //登录请求
    {
        char caName[32]={'\0'};
        char caPwd[32]={'\0'};
        strncpy(caName,pdu->caData,32);
        strncpy(caPwd,pdu->caData+32,32);
        bool ret=OpeDB::getInstance().handleLogin(caName,caPwd);
        PDU *respdu=mkPDU(0);
        respdu->uiMsgType=ENUM_MSG_TYPE_LOGIN_RESPOND;
        if(ret)
        {
            strcpy(respdu->caData,LOGIN_OK);
            m_strName=caName; //保存登录的用户名
        }
        else
        {
            strcpy(respdu->caData,LOGIN_FAILED);
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu=NULL;
        break;
    }
    case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST: //在线用户请求
    {
        QStringList ret=OpeDB::getInstance().handleAllOnline();
        uint uiMsgLen=ret.size()*32;
        PDU *respdu=mkPDU(uiMsgLen);
        respdu->uiMsgType=ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
        for(int i=0;i<ret.size();i++)
        {
            //把在线用户列表中的每个用户名复制到协议数据单元的消息部分，每个用户名占32字节
            memcpy((char*)respdu->caMsg+i*32,ret.at(i).toStdString().c_str(),ret.at(i).size());
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu=NULL;
        break;
    }
    case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:    //搜索用户请求
    {
        int ret=OpeDB::getInstance().handleSearchUsr(pdu->caData);
        PDU *respdu=mkPDU(0);
        respdu->uiMsgType=ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
        if(ret == -1)      //没有该用户
        {
            strcpy(respdu->caData,SEARCH_USR_NO);
        }
        else if(ret == 0)  //离线
        {
            strcpy(respdu->caData,SEARCH_USR_OFFLINE);
        }
        else if(ret == 1)  //在线
        {
            strcpy(respdu->caData,SEARCH_USR_ONLINE);
        }
        write((char*)respdu,respdu->uiPDULen);  //把搜索结果发送给客户端
        free(respdu);
        respdu=NULL;
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:    //添加好友请求
    {
        // 获取客户端传输过来的好友名称
        char caFriendName[32] = {'\0'};
        char caLoginName[32] = {'\0'};
        // 前32位是好友名称，后32位是登录者名称
        strncpy(caFriendName, pdu->caData, 32);
        strncpy(caLoginName, pdu->caData + 32, 32);
        // 添加好友判断
        int ret = OpeDB::getInstance().handleAddfriendCheck(caFriendName, caLoginName);
        // 未添加好友且好友在线
        if(ret == 1)
        {
            MyTcpServer::getInstance().resend(caFriendName, pdu);
        }
        else
        {
            PDU *retPdu = mkPDU(0);
            retPdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            // 好友名称为NULL
            if(ret == -1)
            {
                strcpy(retPdu->caData, UNKNOWN_ERROR);
            }
            // 好友已存在
            else if(ret == 0)
            {
                strcpy(retPdu->caData, ADD_FRIEND_EXISTS);
            }
            // 好友不存在，但该用户离线
            else if(ret == 2)
            {
                strcpy(retPdu->caData, ADD_FRIEND_OFFLINE);
            }
            // 好友名称错误
            else if(ret == 3)
            {
                strcpy(retPdu->caData, ADD_FRIEND_NOT_EXISTS);
            }
            this->write((char *)retPdu, pdu->uiPDULen);
            free(retPdu);
            retPdu = NULL;
        }
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:    //同意添加好友
    {
        // 获取客户端传输过来的好友名称
        char caFriendName[32] = {'\0'};
        char caLoginName[32] = {'\0'};
        // 前32位是好友名称，后32位是登录者名称
        strncpy(caFriendName, pdu->caData, 32);
        strncpy(caLoginName, pdu->caData + 32, 32);
        // 添加好友
        OpeDB::getInstance().handleAddfriend(caFriendName, caLoginName);
        // 转发消息，添加好友成功
        MyTcpServer::getInstance().resend(caLoginName, pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:   //拒绝添加好友
    {
        // 获取客户端传输过来的好友名称
        char caLoginName[32] = {'\0'};
        // 前32位是好友名称，后32位是登录者名称
        strncpy(caLoginName, pdu->caData + 32, 32);
        // 转发消息，申请加好友失败
        MyTcpServer::getInstance().resend(caLoginName, pdu);
    }
    case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:    //刷新好友请求
    {
        // 获取客户端传输过来的好友名称
        char caLoginName[32] = {'\0'};
        strncpy(caLoginName, pdu->caData, 32);
        QStringList ret = OpeDB::getInstance().handleFlushFriend(caLoginName);
        uint uiMsgLen = ret.size() * 32;
        PDU *retPdu = mkPDU(uiMsgLen);
        retPdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
        for(int i = 0; i < ret.size(); i++)
        {
            // 记住，这里放的应该的char*，而不是int[], 因为是要复制char*的内容过来
            memcpy((char*)(retPdu->caMsg) + i * 32
                   , ret.at(i).toStdString().c_str()
                   , ret.at(i).size());
        }
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:   //删除好友请求
    {
        // 获取客户端传输过来的好友名称
        char caFriendName[32] = {'\0'};
        char caLoginName[32] = {'\0'};
        // 前32位是好友名称，后32位是登录者名称
        strncpy(caFriendName, pdu->caData, 32);
        strncpy(caLoginName, pdu->caData + 32, 32);
        // 删除好友
        OpeDB::getInstance().handleDeletefriend(caFriendName, caLoginName);
        // 通知删除成功
        PDU *retPdu = mkPDU(0);
        retPdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
        strcpy(retPdu->caData, DELETE_FRIEND_OK);
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        // 转发消息给被删除的好友
        MyTcpServer::getInstance().resend(caFriendName, pdu);
        break;
    }
    case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:    //私聊请求
    {
        // 获取客户端传输过来的好友名称
        char caFriendName[32] = {'\0'};
        strncpy(caFriendName, pdu->caData, 32);
        qDebug() << "私聊请求，好友名称：" << caFriendName;
        MyTcpServer::getInstance().resend(caFriendName, pdu);
        break;
    }
    case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:  //群聊请求
    {
        QStringList allUser = OpeDB::getInstance().handleAllOnline();
        for(int i = 0; i < allUser.size(); i++)
        {
            MyTcpServer::getInstance().resend(allUser.at(i).toStdString().c_str(), pdu);
        }
        break;
    }
    case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:  //创建文件夹请求
    {
        // 获取客户端传输过来的名称
        char caLoginName[32] = {'\0'};
        char caDirName[32] = {'\0'};
        // 前32位是登录者名称，后32位是新增文件夹名称
        strncpy(caLoginName, pdu->caData, 32);
        strncpy(caDirName, pdu->caData + 32, 32);
        QString strCurPath = QString("%1").arg((char*)pdu->caMsg);  //当前文件夹路径
        QString strNewPath = strCurPath + "/" + caDirName;  //新建文件夹路径

        PDU *retPdu = mkPDU(0);
        retPdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;

        QDir dir;
        bool ret = dir.exists(strCurPath);  //判断当前文件夹路径是否存在
        if(ret) // 存在
        {
            ret=dir.exists(strNewPath);    //判断新建文件夹路径是否存在
            if(ret)     // 已存在
            {
                strcpy(retPdu->caData, FILE_NAME_EXIST);
                write((char*)retPdu, retPdu->uiPDULen);
                free(retPdu);
                retPdu = NULL;
                return;
            }
            else    // 不存在,可以创建
            {
                ret = dir.mkdir(strNewPath);
                if(!ret)      // 创建失败
                {
                    strcpy(retPdu->caData, CREATE_DIR_ERROR);
                    write((char*)retPdu, retPdu->uiPDULen);
                    free(retPdu);
                    retPdu = NULL;
                    return ;
                }
                else    // 创建成功
                {
                    strcpy(retPdu->caData, CREATE_DIR_OK);
                    write((char*)retPdu, retPdu->uiPDULen);
                    free(retPdu);
                    retPdu = NULL;
                    return ;
                }
            }
        }
        else    // 不存在
        {
            strcpy(retPdu->caData, CUR_DIR_NOT_EXIST);
            write((char*)retPdu, retPdu->uiPDULen);
            free(retPdu);
            retPdu = NULL;
            return;
        }
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_DIR_REQUEST:   //刷新文件夹请求
    {
        char *pCurPath = new char[pdu->uiMsgLen];
        memcpy(pCurPath, (char*)pdu->caMsg, pdu->uiMsgLen);
        qDebug() << "刷新文件夹请求，当前路径：" << pCurPath;
        PDU *retPdu = getDirFilePDU(pCurPath);
        retPdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_DIR_RESPOND;
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_DEL_DIR_REQUEST:     //删除文件夹请求
    {
        char caName[32] = {'\0'};
        strcpy(caName, pdu->caData);
        char *curPath = new char[pdu->uiMsgLen];
        memcpy(curPath, pdu->caMsg, pdu->uiMsgLen);
        QString strPath = QString("%1/%2").arg(curPath).arg(caName); //要删除的文件夹路径
        qDebug() << "删除文件夹请求，要删除的文件夹路径：" << strPath;
        // 文件对象
        QFileInfo fileInfo(strPath);
        PDU *retPdu = NULL;
        // 如果不是文件夹的话
        if(!fileInfo.isDir())
        {
            retPdu = mkPDU(strlen(DEL_DIR_TYPE_ERROR) + 1);
            retPdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
            memcpy(retPdu->caData, DEL_DIR_TYPE_ERROR, strlen(DEL_DIR_TYPE_ERROR));
            write((char*)retPdu, retPdu->uiPDULen);
            free(retPdu);
            retPdu = NULL;
            return ;
        }
        QDir dir;
        dir.setPath(strPath);
        // 删文件包括其中的内容
        bool ret = dir.removeRecursively();
        // 如果删除失败
        if(!ret)
        {
            retPdu = mkPDU(sizeof(DEL_DIR_SYSTEM_ERROR) + 1);
            memcpy(retPdu->caData, DEL_DIR_SYSTEM_ERROR, strlen(DEL_DIR_SYSTEM_ERROR));
        }
        else
        {
            retPdu = mkPDU(sizeof(DEL_DIR_OK) + 1);
            memcpy(retPdu->caData, DEL_DIR_OK, strlen(DEL_DIR_OK));
        }
        retPdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_RENAME_DIR_REQUEST:  //重命名文件夹请求
    {
        char caOldName[32] = {'\0'};
        char caNewName[32] = {'\0'};
        strncpy(caOldName, pdu->caData, 32);
        strncpy(caNewName, pdu->caData + 32, 32);

        char *curPath = new char[pdu->uiMsgType];
        memcpy(curPath, (char*)pdu->caMsg, pdu->uiMsgLen);
        QString strOldPath = QString("%1/%2").arg(curPath).arg(caOldName);
        QString strNewPath = QString("%1/%2").arg(curPath).arg(caNewName);
        QDir dir;
        bool ret = dir.rename(strOldPath, strNewPath);
        PDU *retPdu = mkPDU(0);
        retPdu->uiMsgType = ENUM_MSG_TYPE_RENAME_DIR_RESPOND;
        // 如果重命名成功
        if(ret)
        {
            strcpy(retPdu->caData, RENAME_FILE_OK);
        }
        else
        {
            strcpy(retPdu->caData, RENAME_FILE_FIAL);
        }
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:  //进入文件夹请求
    {
        char caEnterName[32] = {'\0'};
        strncpy(caEnterName, pdu->caData, 32);

        char *caCurPath = new char[pdu->uiMsgLen];
        memcpy(caCurPath, (char*)pdu->caMsg, pdu->uiMsgLen);

        QString strPath = QString("%1/%2").arg(caCurPath).arg(caEnterName);
        QFileInfo fileInfo(strPath);
        if(fileInfo.isFile())
        {
            PDU *retPdu = mkPDU(0);
            retPdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
            strcpy(retPdu->caData, ENTER_DIR_FAIL);
            write((char*)retPdu, retPdu->uiPDULen);
            free(retPdu);
            retPdu = NULL;
        }
        else if(fileInfo.isDir())
        {
            PDU *retPdu = getDirFilePDU(strPath);
            retPdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
            strcpy(retPdu->caData, ENTER_DIR_OK);
            write((char*)retPdu, retPdu->uiPDULen);
            free(retPdu);
            retPdu = NULL;
        }
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:  //上传文件请求
    {
        char caFileName[32] = {'\0'};
        qint64 fileSize = 0;
        sscanf(pdu->caData, "%s %lld", caFileName, &fileSize); //从协议数据单元的caData字段中解析出文件名称和文件大小
        char *curPath = new char[pdu->uiMsgLen];
        memcpy(curPath, (char*)pdu->caMsg, pdu->uiMsgLen);
        QString strPath = QString("%1/%2").arg(curPath).arg(caFileName);
        delete []curPath;
        curPath = NULL;
        m_file.setFileName(strPath);
        // 以只写的方式打开文件，如果文件不存在，则自动创建文件
        if(m_file.open(QIODevice::WriteOnly))
        {
            m_bUpload = true;
            m_iTotal = fileSize;
            m_iRecved = 0;
            m_iCount = 1;
        }
        break;
    }
    case ENUM_MSG_TYPE_DEL_FILE_REQUEST:     //删除文件请求
    {
        char caFileName[32] = {'\0'};
        strcpy(caFileName, pdu->caData);
        char *curPath = new char[pdu->uiMsgLen];
        memcpy(curPath, pdu->caMsg, pdu->uiMsgLen);
        QString strPath = QString("%1/%2").arg(curPath).arg(caFileName);
        delete []curPath;
        curPath = NULL;
        // 文件对象
        QFileInfo fileInfo(strPath);
        PDU *retPdu = NULL;
        // 如果不是文件夹的话
        if(fileInfo.isDir())
        {
            retPdu = mkPDU(strlen(DEL_FILE_TYPE_ERROR) + 1);
            retPdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
            memcpy(retPdu->caData, DEL_FILE_TYPE_ERROR, strlen(DEL_FILE_TYPE_ERROR));
            write((char*)retPdu, retPdu->uiPDULen);
            free(retPdu);
            retPdu = NULL;
            return ;
        }
        QDir dir;
        // 删除文件目录中的该文件
        bool ret = dir.remove(strPath);
        // 如果删除失败
        if(!ret)
        {
            retPdu = mkPDU(sizeof(DEL_FILE_SYSTEM_ERROR) + 1);
            memcpy(retPdu->caData, DEL_FILE_SYSTEM_ERROR, strlen(DEL_FILE_SYSTEM_ERROR));
        }
        else
        {
            retPdu = mkPDU(sizeof(DEL_FILE_OK) + 1);
            memcpy(retPdu->caData, DEL_FILE_OK, strlen(DEL_FILE_OK));
        }
        retPdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:   //下载文件请求
    {
        char caFileName[32] = {'\0'};
        strcpy(caFileName, pdu->caData);
        char *curPath = new char[pdu->uiMsgLen];
        memcpy(curPath, pdu->caMsg, pdu->uiMsgLen);
        QString strPath = QString("%1/%2").arg(curPath).arg(caFileName);
        delete []curPath;
        curPath = NULL;
        QFileInfo fileInfo(strPath);
        qint64 fileSize = fileInfo.size();
        PDU *retPdu = mkPDU(0);
        retPdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
        //把要下载的文件名称和文件大小放入协议数据单元的caData字段中，发送给客户端
        sprintf(retPdu->caData, "%s %lld", caFileName, fileSize);
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        // 借用 m_file对象进行下载文件，因为目前是单线程阻塞的，所以不影响
        m_file.setFileName(strPath);
        m_file.open(QIODevice::ReadOnly);
        // 定时器设置1s后发送数据
        m_pTimer->start(1000);
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_REQUEST://分享文件请求
    {
        char caSendName[32] = {'\0'};
        int num = 0;    //被分享者数量
        sscanf(pdu->caData, "%s %d", caSendName, &num);
        int size = num * 32;
        PDU *retPdu = mkPDU(pdu->uiMsgLen - size);//下载文件的路径的大小
        retPdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE;
        strcpy(retPdu->caData, caSendName);
        memcpy((char*)retPdu->caMsg, (char*)(pdu->caMsg) + size, pdu->uiMsgLen - size);
        char caRecvName[32] = {'\0'};
        for(int i = 0; i < num ; i++)
        {
            memcpy(caRecvName, (char*)(pdu->caMsg) + i * 32, 32);
            qDebug() << "要转发给该好友文件:"
                     <<caRecvName;
            MyTcpServer::getInstance().resend(caRecvName, retPdu);
        }
        free(retPdu);
        retPdu = NULL;
        retPdu = mkPDU(0);
        retPdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
        strcpy(retPdu->caData, "send share file msg ok");
        write((char*)retPdu, retPdu->uiPDULen);
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_NOTE_REQUEST://收到共享文件通知响应
    {
        //接收文件的路径，caData字段中存储的是接收者的用户名，可以根据用户名构造出接收文件的路径
        QString strRecvPath = QString("./%1").arg(pdu->caData);
        QString strShareFilePath = QString("%1").arg((char*)pdu->caMsg);//共享文件的路径
        int index = strShareFilePath.lastIndexOf('/');
        QString fileName = strShareFilePath.right(strShareFilePath.size() - index - 1);
        strRecvPath = strRecvPath + "/" + fileName;
        QFileInfo qFileInfo(strShareFilePath);
        if(qFileInfo.isFile())
        {
            QFile::copy(strShareFilePath, strRecvPath);
        }
        else if(qFileInfo.isDir())
        {
            copyDir(strShareFilePath, strRecvPath);
        }
        PDU *retPdu = mkPDU(0);
        retPdu->uiMsgLen = ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND;
        strcpy(retPdu->caData, "copy file ok");
        write((char*)retPdu, retPdu->uiPDULen);
        free(retPdu);
        retPdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_MOVE_FILE_REQUEST:   //移动文件请求
    {
        char caFileName[32] = {'\0'};   //要移动的文件名称
        int srcLen = 0;     //源文件路径的长度
        int destLen = 0;    //目标文件路径的长度
        // 从协议数据单元的caData字段中解析出源文件路径的长度、目标文件路径的长度和要移动的文件名称
        sscanf(pdu->caData, "%d %d %s", &srcLen, &destLen, caFileName);

        char *pSrcPath = new char[srcLen + 1];          //源文件路径
        char *pDestPath = new char[destLen +1 + 32];    //目标文件路径，预留32字节的空间用于拼接文件名称
       //将源文件路径和目标文件路径的内存区域初始化为全0，确保字符串以'\0'结尾
        memset(pSrcPath, '\0', srcLen + 1);
        memset(pDestPath, '\0', destLen +1 + 32);

        memcpy(pSrcPath, pdu->caMsg,  srcLen);
        memcpy(pDestPath, (char*)(pdu->caMsg) + (srcLen + 1),  destLen);

        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
        QFileInfo fileInfo(pDestPath);
        if(fileInfo.isDir())    // 如果目标目录是目录才能移动
        {
            // 拼接目标文件路径，格式为：目标目录路径 + "/" + 文件名称
            strcat(pDestPath, "/");
            strcat(pDestPath, caFileName);

            bool ret = QFile::rename(pSrcPath, pDestPath);
            if(ret)
            {
                strcpy(respdu->caData, MOVE_FILE_OK);
            }
            else
            {
                strcpy(respdu->caData, COMMON_ERR);
            }
        }
        else if(fileInfo.isFile())   // 如果目标目录是文件夹，那么不能移动
        {
            strcpy(respdu->caData, MOVE_FILE_FAIL);
        }
        // 其他文件类型目前不做处理
        else
        {
            strcpy(respdu->caData, MOVE_FILE_FAIL);
        }

        write((char*)respdu, respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    default:
        break;
    }
}

void MyTcpSocket::clientOffline()
{
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this); //发送用户离线信号，参数是离线的客户端套接字对象指针
}

void MyTcpSocket::sendFileDataToClient()
{
    char *pBuffer = new char[4096];
    qint64 ret = 0; //保存从文件中读取的内容
    while(true)
    {
        //从文件中读取最多4096字节的数据到pBuffer中，返回实际读取的字节数，
        //如果返回0，代表文件已经读取结束了，如果返回-1，代表读取文件出错了
        ret = m_file.read(pBuffer, 4096);
        if(ret > 0 && ret <= 4096)  // 如果读取到了文件中的数据，则发送给客户端
        {
            write(pBuffer, ret);
        }
        else if(ret == 0) // 如果文件读取结束，就结束循环
        {
            break;
        }
        else     // 如果ret < 0 或者 ret > 4096,则代表读取错误了
        {
            qDebug() << "下载文件失败：读取文件内容失败";
            break;
        }
    }
    m_file.close(); //关闭文件
    delete []pBuffer;
    pBuffer = NULL;
    m_pTimer->stop();   //停止定时器，防止重复发送文件数据
}

PDU * MyTcpSocket::getDirFilePDU(QString curPath)
{
    QDir dir(curPath);
    // 获取当前路径下的所有文件和文件夹信息，过滤掉"."和".."这两个特殊目录
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    int iFileCount = fileInfoList.size();
    PDU *retPdu = mkPDU(sizeof(FileInfo) * iFileCount);
    FileInfo *pFileInfo = NULL;
    for(int i = 0; i < iFileCount; i++)
    {
        // 获取返回PDU中第i块FileInfo的内存区域，向里面塞入数据
        pFileInfo = (FileInfo *)retPdu->caMsg + i;
        strcpy(pFileInfo->caFileName, fileInfoList[i].fileName().toStdString().c_str());
        if(fileInfoList[i].isDir())
        {
            pFileInfo->iFileType = 0;
        }
        else if(fileInfoList[i].isFile())
        {
            pFileInfo->iFileType = 1;
        }
        else
        {
            pFileInfo->iFileType = 2;
        }
    }
    return retPdu;
}


void MyTcpSocket::copyDir(QString sourceDir, QString targetDir)
{
    QDir dir;
    dir.mkdir(targetDir);//创建目标文件夹，防止文件夹不存在
    dir.setPath(sourceDir);
    QFileInfoList fileInfoList = dir.entryInfoList();
    QString sourceTemp;
    QString targetTemp;
    for(int i = 0; i < fileInfoList.size(); i++)
    {
        sourceTemp = sourceDir + "/" + fileInfoList.at(i).fileName();
        targetTemp = targetDir + "/" + fileInfoList.at(i).fileName();
        if(fileInfoList.at(i).isFile())
        {
            QFile::copy(sourceTemp, targetTemp);
        }
        else if(fileInfoList.at(i).isDir())
        {
            // 不复制 . 和 ..目录
            if(QString(".") == fileInfoList.at(i).fileName()
                || QString("..") == fileInfoList.at(i).fileName())
            {
                continue;
            }
            copyDir(sourceTemp, targetTemp);
        }
    }
}

