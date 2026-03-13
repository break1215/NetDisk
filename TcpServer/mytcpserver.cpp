#include "mytcpserver.h"
#include<QDebug>
MyTcpServer::MyTcpServer()
{

}

MyTcpServer &MyTcpServer::getInstance()
{
    static MyTcpServer instance;
    return instance;
}

void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug()<<"new client connect";
    MyTcpSocket *ptcpSocket=new MyTcpSocket;
    ptcpSocket->setSocketDescriptor(socketDescriptor); //将套接字描述符与套接字对象关联起来，这样就可以通过套接字对象来操作这个连接了
    m_tcpSocketlist.append(ptcpSocket);

    //当客户端离线时，触发offline(MyTcpSocket*)信号，调用deleteSocket(MyTcpSocket*)槽函数删除套接字对象
    connect(ptcpSocket,SIGNAL(offline(MyTcpSocket*)),this,SLOT(deleteSocket(MyTcpSocket*)));


}

void MyTcpServer::resend(const char *pername, PDU *pdu)
{
    if(pername == NULL || pdu == NULL)
    {
        return;
    }
    QString strName=QString(pername);   //
    for(int i=0;i<m_tcpSocketlist.size();i++)  //遍历在线用户列表，找到目标用户的套接字对象
    {
        if(strName==m_tcpSocketlist.at(i)->getName())
        {
            m_tcpSocketlist.at(i)->write((char*)pdu,pdu->uiPDULen); //把协议数据单元发送给目标用户
            break;
        }
    }
}

void MyTcpServer::deleteSocket(MyTcpSocket *myTcpSocket)
{
    QList<MyTcpSocket*>::iterator iter=m_tcpSocketlist.begin();
    for(;iter != m_tcpSocketlist.end();iter++)
    {
        if(myTcpSocket == *iter)
        {
            // 删除套接字对象，deleteLater()是QObject的一个函数，
            //它会在事件循环的适当时机删除对象，避免直接delete可能导致的程序崩溃
            (*iter)->deleteLater();
            m_tcpSocketlist.erase(iter); //从列表中删除套接字对象
            break;
        }
    }
    for(int i=0;i<m_tcpSocketlist.size();i++)
    {
        qDebug()<<"在线用户:"<<m_tcpSocketlist.at(i)->getName();
    }
}
