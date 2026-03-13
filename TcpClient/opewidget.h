#ifndef OPEWIDGET_H
#define OPEWIDGET_H

#include <QWidget>
#include<QListWidget>

#include"friend.h"
#include"book.h"
#include<QStackedWidget>

class OpeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OpeWidget(QWidget *parent = 0);
    static OpeWidget &getInstance(); //获取单例对象的静态成员函数
    Friend *getFriend() ; //获取好友窗口的成员函数
    Book *getBook() ; //获取图书窗口的成员函数

signals:

public slots:

private:
    QListWidget *m_pListW; //显示在线用户的列表控件
    Friend *m_pFriend;   //好友窗口
    Book *m_pBook;   //图书窗口

    QStackedWidget *m_pSW; //堆叠窗口，用于显示好友窗口和图书窗口
};

#endif // OPEWIDGET_H
