#ifndef FRIEND_H
#define FRIEND_H

#include <QWidget>
#include<QTextEdit>
#include<QListWidget>
#include<QLineEdit>
#include<QPushButton>
#include<QVBoxLayout>
#include<QHBoxLayout>
#include"online.h"


class Friend : public QWidget
{
    Q_OBJECT
public:
    explicit Friend(QWidget *parent = nullptr);
    void showAllOnlineUsr(PDU *pdu); //显示所有在线用户
    void updateFriendList(PDU *pdu);   //更新好友列表
    void updateGroupMsg(PDU *pdu);  //更新群聊消息

    QString m_strSearchName;    //搜索的用户名
    QListWidget *getFriendList();   //获取好友列表


signals:

public slots:
    void showOnline();  // 显示在线用户窗口的槽函数
    void searchUsr();  // 搜索用户的槽函数
    void flushFriend(); // 刷新好友列表的槽函数
    void deleteFriend();  // 删除好友的槽函数
    void privateChat(); // 私聊的槽函数
    void groupChat();   // 群聊的槽函数

private:
    QTextEdit *m_pShowMsgTE;    //显示消息的文本编辑框
    QListWidget *m_pFriendListWidget;   //好友列表
    QLineEdit *m_pInputMsgLE;   //内容输入框

    QPushButton *m_pDelFriendPB;    //删除用户按钮
    QPushButton *m_pFlushFriendPB;  //刷新好友用户按钮
    QPushButton *m_pShowOnlineUserPB;   //查看在线用户
    QPushButton *m_pSearchUserPB;   //搜素用户
    QPushButton *m_pMsgSendPB;  //发送消息按钮
    QPushButton *m_pPrivateChatPB;  //私聊按钮

    Online *m_pOnline;    //在线用户窗口


};

#endif // FRIEND_H
