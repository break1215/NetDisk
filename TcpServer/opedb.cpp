#include "opedb.h"
#include<QMessageBox>
#include<QDebug>

OpeDB::OpeDB(QObject *parent)
    : QObject{parent}
{
    //使用SQLite数据库驱动程序创建一个数据库连接对象，并将其赋值给成员变量m_db
    m_db = QSqlDatabase::addDatabase("QSQLITE");
}

OpeDB::~OpeDB()
{
    m_db.close();
}

OpeDB &OpeDB::getInstance()
{
    static OpeDB instance;
    return instance;
}

void OpeDB::init()
{
    m_db.setHostName("localhost");
    m_db.setDatabaseName("D:\\QT\\NetDisk\\TcpServer\\cloud.db");
    m_db.open();
    if(m_db.open())
    {
        QSqlQuery query;

        query.exec("select * from usrInfo");
        while(query.next())
        {
            QString name = query.value(1).toString();
            QString passwd = query.value(2).toString();
            QString data=QString("%1 %2 %3").arg(query.value(0).toString()).arg(query.value(1).toString()).arg(query.value(2).toString());
            qDebug() << data;
            qDebug() << "用户名: " << name << " 密码: " << passwd;
        }
    }
    else
    {
        QMessageBox::critical(NULL, "Database Error", "打开数据库失败");
    }
}



bool OpeDB::handleRegist(const char *name, const char *pwd)
{
    if(NULL == name || NULL == pwd)
    {
        qDebug() << "用户名或密码为空";
        return false;
    }
    QString data=QString("insert into usrInfo(name,pwd) values(\'%1\',\'%2\')").arg(name).arg(pwd);
    // qDebug() << data;
    QSqlQuery query;
    return query.exec(data);
}

bool OpeDB::handleLogin(const char *name, const char *pwd)
{
    if(NULL == name || NULL == pwd)
    {
        qDebug() << "用户名或密码不能为空";
        return false;
    }
    QString data=QString("select * from usrInfo where name=\'%1\' and pwd = \'%2\' and online=0 ").arg(name).arg(pwd);
    QSqlQuery query;
    query.exec(data);

    if(query.next())
    {
        data=QString("update usrInfo set online=1 where name=\'%1\' and pwd = \'%2\' ").arg(name).arg(pwd);
        QSqlQuery query;
        query.exec(data);
        return true;
    }
    else
    {
        qDebug() << "登录失败：用户名或密码错误，或者用户已登录";
        return false;
    }
    return query.next();
}

void OpeDB::handleOffline(const char *name)
{
    if(NULL == name )
    {
        qDebug() << "用户名为空";
        return ;
    }
    QString data=QString("update usrInfo set online=0 where name=\'%1\' ").arg(name);
    QSqlQuery query;
    query.exec(data);
}

QStringList OpeDB::handleAllOnline()
{
    QString data=QString("select name from usrInfo where online=1 ");
    QSqlQuery query;
    query.exec(data);

    QStringList result;
    result.clear();
    while(query.next()) //遍历查询结果，将在线用户的用户名添加到结果列表中
    {
        result.append( query.value(0).toString());  //将查询结果中的用户名添加到结果列表中
    }
    return result;
}

int OpeDB::handleSearchUsr(const char *name)
{
    if(NULL == name)
    {
        return -1;
    }
    QString data=QString("select online from usrInfo where name=\'%1\' ").arg(name);
    QSqlQuery query;
    query.exec(data);
    if(query.next())
    {
        int ret=query.value(0).toInt();
        return ret;    //ret为1时用户在线，为0时不在线
    }
    else
    {
        return -1;   //用户不存在
    }
}

int OpeDB::handleAddfriendCheck(const char *friendName, const char *loginName)
{
    if(NULL == friendName || NULL == loginName)
    {
        // 输入内容格式错误
        return -1;
    }
    QString strSql = QString("select id from usrInfo where name = '%1'").arg(friendName);
    QSqlQuery query;
    query.exec(strSql);
    if(query.next())
    {
        int friendId = query.value(0).toInt();
        strSql = QString("select id from usrInfo where name = '%1'").arg(loginName);
        query.exec(strSql);
        query.next();
        int loginId = query.value(0).toInt();
        strSql = QString("select * from friend where id = '%1' and friendId = '%2'").arg(loginId).arg(friendId);
        query.exec(strSql);
        if(query.next())
        {
            // 如果已经是好友了
            return 0;
        }
        else
        {
            strSql = QString("select online from usrInfo where id = '%1'").arg(friendId);
            query.exec(strSql);
            query.next();
            int online = query.value(0).toInt();
            if(online == 1)
            {
                // 对方在线中
                return 1;
            }
            else if(online == 0)
            {
                // 如果对方离线中
                return 2;
            }
        }
    }
    else
    {
        // 用户不存在
        return 3;
    }
}

void OpeDB::handleAddfriend(const char *friendName, const char *loginName)
{
    // 获取好友id
    QString strSql = QString("select id from usrInfo where name = '%1'").arg(friendName);
    QSqlQuery query;
    query.exec(strSql);

    qDebug() << "正在查找的好友名：" << QString::fromUtf8(friendName);
    if(!query.next()) {
        qDebug() << "好友不存在: " << friendName;
        return; // 好友不存在，直接返回
    }
    int friendId = query.value(0).toInt();
    // 获取登录用户id
    strSql = QString("select id from usrInfo where name = '%1'").arg(loginName);
    query.exec(strSql);
    if (!query.next())
    {
        qDebug() << "登录用户不存在: " << loginName;
        return; // 登录用户不存在，直接返回
    }
    int loginId = query.value(0).toInt();
    // 新增好友
    strSql = QString("insert into friend (id,friendId) values (%1,%2), (%2,%1)").arg(friendId).arg(loginId);
    query.exec(strSql);
}

QStringList OpeDB:: handleFlushFriend(const char *name)
{
    QStringList strFriendList;
    strFriendList.clear();

    if(NULL == name)
    {
        return strFriendList;
    }

    QString strSql = QString("select id from usrInfo where name = '%1'").arg(name);
    QSqlQuery query;
    query.exec(strSql);

    qDebug() << "用户名：" << QString::fromUtf8(name);

    if (!query.next()) {
        qDebug() << "用户不存在: " << name;
        return strFriendList; // 用户不存在，直接返回空列表
    }
    int loginId = query.value(0).toInt();

    strSql = QString("select friendId from friend where id = %1").arg(loginId);
    query.exec(strSql);
    while(query.next())
    {
        int friendId = query.value(0).toInt();
        strSql = QString("select name from usrInfo where id = %1 and online = 1").arg(friendId);
        QSqlQuery nameQuery;
        nameQuery.exec(strSql);
        if(nameQuery.next())
        {
            strFriendList.append(nameQuery.value(0).toString());
        }
    }
    return strFriendList;
}

void OpeDB::handleDeletefriend(const char *friendName, const char *loginName)
{
    if(NULL == friendName || NULL == loginName)
    {
        return ;
    }
    QString strSql = QString("select id from usrInfo where name = '%1'").arg(friendName);
    QSqlQuery query;
    query.exec(strSql);
    query.next();
    int friendId = query.value(0).toInt();
    strSql = QString("select id from usrInfo where name = '%1'").arg(loginName);
    query.exec(strSql);
    query.next();
    int userId = query.value(0).toInt();
    strSql = QString("delete from friend where (id = '%1' and friendId = '%2') or (id = '%2' and friendId = '%1')").arg(userId).arg(friendId);
    query.exec(strSql);
}
