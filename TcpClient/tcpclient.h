#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QFile>
#include <QTcpSocket>
#include "protocol.h"
#include "opewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class TcpClient;
}
QT_END_NAMESPACE

class TcpClient : public QWidget
{
    Q_OBJECT

public:
    TcpClient(QWidget *parent = nullptr);
    ~TcpClient();
    void loadConfig();

    static TcpClient& getInstance(); //单例模式，获取Tcpclient实例的静态成员函数
    QTcpSocket& getTcpSocket(); // 获取TCP套接字的成员函数
    QString loginName(); // 获取登录用户名
    QString curPath();  // 获取当前路径
    void setCurPath(QString strPath);   // 设置当前路径

public slots:
    void showConnect();
    void recvMsg();


private slots:
    // void on_pushButton_clicked();

    void on_login_pb_clicked();

    void on_regist_pb_clicked();

    void on_cancel_pb_clicked();

private:
    Ui::TcpClient *ui;
    QString m_strIP;
    quint16 m_usPort;

    QTcpSocket m_tcpSocket; //TCP套接字对象，用于与服务器进行通信
    QString m_strLoginName; //保存登录的用户名
    QString m_strCurPath;   // 当前路径

};
#endif // TCPCLIENT_H
