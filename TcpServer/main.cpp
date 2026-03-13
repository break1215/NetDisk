#include "tcpserver.h"
#include "opedb.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    OpeDB::getInstance().init(); // 初始化数据库连接

    TcpServer w;
    w.setWindowTitle("服务端");
    w.show();
    return a.exec();
}

