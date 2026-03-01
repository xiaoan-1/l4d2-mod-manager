#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QMenu>

#include "sqliteobj.h"
#include "modmanager.h"
#include "comp/modcard.h"
#include "cardcontainer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // 添加分类按钮
    void addCategoryButton(const CategoryInfo &category);

    // 刷新Mod卡片列表
    // void refreshModCards(const QList<ModInfo> &modInfoList, const CategoryInfo &category = CategoryInfo());

    // 刷新Mod统计信息
    void refreshModCount();

private:
    Ui::MainWindow *ui;

    // 设置菜单
    QMenu *m_settingMenu;

    // 互斥选择按钮
    QButtonGroup m_buttonGroup;

    QList<ModCard*> m_modCardList;
};
#endif // MAINWINDOW_H
