#include <QtGui/QApplication>
#include "dialog.h"
#include <QTextCodec> //���������ʾ��Ҫ

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //���������ʾ
    QTextCodec *codec = QTextCodec::codecForName("GB2312");
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);

    Dialog w;
    w.show();

    return a.exec();
}
