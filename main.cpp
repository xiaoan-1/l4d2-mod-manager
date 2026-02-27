#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 解决文字不清晰的问题
    QFont font = a.font();
    font.setHintingPreference(QFont::PreferNoHinting);
    a.setFont(font);

    MainWindow w;
    w.show();
    return a.exec();
}
