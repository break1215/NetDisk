#include "tcpclient.h"
#include <QApplication>
#include"sharefile.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // ShareFile w;
    // w.show();

    TcpClient &w = TcpClient::getInstance();
    w.setWindowTitle("客户端");
    w.show();

    return a.exec();
}
