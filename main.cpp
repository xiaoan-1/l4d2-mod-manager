#include "mainwindow.h"

#include <QApplication>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 从资源文件中加载字体
    QString appDir = QCoreApplication::applicationDirPath();
    QFontDatabase::addApplicationFont(appDir + "/font/MapleMono-NF-CN-Bold.ttf");
    int fontId = QFontDatabase::addApplicationFont(appDir + "/font/MapleMono-NF-CN-Regular.ttf");
    if (fontId != -1) {
        // 获取加载的字体家族名称
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            QFont customFont(fontFamilies.first());
            customFont.setHintingPreference(QFont::PreferNoHinting);
            customFont.setPointSize(11);
            a.setFont(customFont);
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
