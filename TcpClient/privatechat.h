#ifndef PRIVATECHAT_H
#define PRIVATECHAT_H

#include <QWidget>
#include "protocol.h"

namespace Ui {
class PrivateChat;
}

class PrivateChat : public QWidget
{
    Q_OBJECT

public:
    explicit PrivateChat(QWidget *parent = nullptr);
    ~PrivateChat();

    static PrivateChat& getInstance(); //单例模式，获取PrivateChat实例的静态成员函数

    void setChatName(QString &strName); //设置聊天对象的名称
    void updateMsg(const PDU *pdu); //更新消息显示的函数

private slots:
    void on_sendMsg_pb_clicked();

private:
    Ui::PrivateChat *ui;
    QString m_strChatName; //聊天对象的名称
    QString m_strLoginName; //登录者的名称
};

#endif // PRIVATECHAT_H
