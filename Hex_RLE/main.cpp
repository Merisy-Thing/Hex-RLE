#include <QtGui/QApplication>
#include "dialog.h"
#include <QTextCodec> //解决中文显示需要

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //解决中文显示
    QTextCodec *codec = QTextCodec::codecForName("GB2312");
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);

    Dialog w;
    w.show();

    return a.exec();
}
