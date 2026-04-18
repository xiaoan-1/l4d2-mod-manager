#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QMenu>

#include "sqliteobj.h"
#include "gamemanager.h"
#include "comp/modcard.h"
#include "comp/checkboxlistwidget.h"
#include "comp/paramcheckwidget.h"
#include "widget/cardcontainer.h"
#include "widget/modconflictwidget.h"


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
    void initWidget();

    // 添加分类信息
    void addCategory(const CategoryInfo &category);

    // 刷新Mod统计信息
    void refreshModCount();

    // 设置游戏启动参数
    void showGameParamDilaog();

    // 启动游戏
    void startGame();

    // 导入mod到本地
    void importMod();

    // 工坊Mod文件移动至本地
    void moveToLocal();

    // 检测冲突Mod
    void checkConflictMod();

private:
    Ui::MainWindow *ui;

    // 设置菜单
    QMenu *m_settingMenu;

    // 操作菜单
    QMenu *m_operationMenu;

    // 参数选择面板
    ParamCheckWidget *m_paramCheckWidget;

    // 分类筛选列表
    CheckBoxListWidget *m_ckListWidget;

    // 互斥选择按钮
    QButtonGroup m_buttonGroup;

    // 是否显示禁用Mod
    bool m_disableModVisiable = true;

    // 冲突模组面板
    ModConflictWidget *m_modConflictWidget;
};
#endif // MAINWINDOW_H
