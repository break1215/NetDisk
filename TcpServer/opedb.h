#ifndef OPEDB_H
#define OPEDB_H

#include <QObject>
#include<QSqlDatabase>
#include<QSqlQuery>
#include<QStringList>

class OpeDB : public QObject
{
    Q_OBJECT
public:
    explicit OpeDB(QObject *parent = nullptr);
    ~OpeDB();

    static OpeDB& getInstance() ;  //单例模式，获取OpeDB实例的静态成员函数

    void init(); //初始化数据库连接
    bool handleRegist(const char *name, const char *pwd); //处理注册请求
    bool handleLogin(const char *name, const char *pwd); //处理登录请求
    void handleOffline(const char *name); //处理用户离线请求
    QStringList handleAllOnline(); //处理在线用户请求，返回在线用户列表
    int handleSearchUsr(const char *name); //处理搜索用户请求，返回搜索结果

    int handleAddfriendCheck(const char *friendName, const char *loginName); // 添加好友--校验
    void handleAddfriend(const char *friendName, const char *loginName);    // 添加好友操作

    QStringList handleFlushFriend(const char *name); //处理刷新好友请求，返回好友列表
    void handleDeletefriend(const char *friendName, const char *loginName); // 删除好友操作

signals:

public slots:
private:
    QSqlDatabase m_db; //数据库对象
};

#endif // OPEDB_H
