#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include<QTcpSocket>
#include "protocol.h"
#include "opedb.h"
#include<QDir>
#include<QFile>
#include<QTimer>

class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT;
public:
    MyTcpSocket();
    QString getName();
    void copyDir(QString sourceDir, QString targetDir);

signals:
    void offline(MyTcpSocket *myTcpSocket); //用户离线信号，参数是离线的客户端套接字对象指针

public slots:
    void recvMsg(); //接收消息的槽函数
    void clientOffline();   //处理用户离线的槽函数
    void sendFileDataToClient();    //定时器超时，发送文件数据给客户端的槽函数



private:
    QString m_strName; //保存登录的用户名
    QFile m_file;//上传的文件
    qint64 m_iTotal;//文件总大小
    qint64 m_iRecved;//已接受到的数据大小
    bool m_bUpload; //正在上传文件的状态
    qint64 m_iCount;//计数
    QTimer *m_pTimer;

    PDU *getDirFilePDU(QString curPath); //将指定路径文件夹中的所有文件写入PDU中，并返回这个PDU

};

#endif // MYTCPSOCKET_H
