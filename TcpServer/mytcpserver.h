#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QTcpServer>
#include<QList>
#include "mytcpsocket.h"

class MyTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    MyTcpServer();

    static MyTcpServer &getInstance();

    void incomingConnection(qintptr socketDescriptor); //当有新的客户端连接时，QTcpServer会调用这个函数，参数是新连接的套接字描述符

    void resend(const char *pername, PDU *pdu); //转发消息给指定用户的函数，参数是目标用户名和协议数据单元指针

public slots:
    void deleteSocket(MyTcpSocket *myTcpSocket); //删除套接字对象的槽函数，参数是离线的客户端套接字对象指针

private:
    QList<MyTcpSocket*> m_tcpSocketlist;
};

#endif // MYTCPSERVER_H
