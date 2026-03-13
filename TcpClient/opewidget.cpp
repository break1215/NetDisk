#include "opewidget.h"

OpeWidget::OpeWidget(QWidget *parent)
    : QWidget{parent}
{
    m_pListW=new QListWidget(this);
    m_pListW->addItem("好友");
    m_pListW->addItem("图书");

    m_pFriend=new Friend;
    m_pBook=new Book;

    m_pSW=new QStackedWidget(this);
    m_pSW->addWidget(m_pFriend);
    m_pSW->addWidget(m_pBook);

    QHBoxLayout *pMain=new QHBoxLayout(this);
    pMain->addWidget(m_pListW);
    pMain->addWidget(m_pSW);

    setLayout(pMain);

    //当列表控件的当前行发生变化时，触发currentRowChanged(int)信号，
    //调用堆叠窗口的setCurrentIndex(int)槽函数来切换显示的窗口
    connect(m_pListW,SIGNAL(currentRowChanged(int)),m_pSW,SLOT(setCurrentIndex(int)));

}

OpeWidget &OpeWidget::getInstance()
{
    static OpeWidget instance; //静态局部变量，保证只创建一个实例
    return instance;
}

Friend *OpeWidget::getFriend()
{
    return m_pFriend;
}

Book *OpeWidget::getBook()
{
    return m_pBook;
}


