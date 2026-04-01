#include "paramcheckwidget.h"
#include "ui_paramcheckwidget.h"

#include <QTableWidgetItem>
#include <QMessageBox>

#include "gamemanager.h"

ParamCheckWidget::ParamCheckWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ParamCheckWidget)
{
    ui->setupUi(this);    
    hide();

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    connect(ui->pushButton_affirm, &QPushButton::clicked, this, &ParamCheckWidget::genGameParamStr);

    // 游戏选项
    appendParamInfo("-novid","", "跳过游戏开场动画");
    appendParamInfo("-insecure", "",
                     "不安全的启动，关闭VAC反作弊，但会被官方和第三方服务器禁止进入");

    appendParamInfo("-console", "", "自动打开控制台");

    appendParamInfo("-nojoy", "",
                     "禁用摇杆支持，如果你不使用手柄来玩游戏，可以启用此选项来优化系统性能");

    appendParamInfo("-noforcemparms","",
                     "禁用鼠标的 Windows 系统快捷键功能，防止鼠标被系统或其他程序干扰。");

    appendParamInfo("-sv_consistency ", "0",
                     "关闭模型一致性检查，这有助于解决一些因模型不一致导致的冲突，适合自定义地图或者MOD使用");

    appendParamInfo("-exec", "autoexec.cfg", "自动加载配置文件");

    // 显示选项
    appendParamInfo("-tickrate ", "60",
                     "解锁客户端的刷新率，提升多人游戏时的体验。数值可以根据需要调整，常见的设置为100或者60");
    appendParamInfo("-refresh ", "120", "锁定屏幕刷新率");
    appendParamInfo("-fps_max ", "0", "帧率上限（0 = 无限制）");

    // 内存性能选项
    appendParamInfo("-lv","", "低血腥度模式，屏幕不会溅血，自动清除尸体");

    appendParamInfo("-high", "", "设置游戏进程的优先级为高");

    appendParamInfo("-processheap ", "",
                     "此选项可以缓解长时间运行后的卡顿问题，特别是在游戏持续运行几小时之后");

    appendParamInfo("-vulkan", "", "启用 Vulkan 渲染（新版支持）");

    appendParamInfo("-heapsize", "7500000", "调整游戏的堆内存大小，750000=512MB［默认数值］最高不建议超过9000000=6G");

    appendParamInfo("-d3d9ex", "", "强制游戏使用 Direct3D 9Ex (扩展版) 渲染模式, 主要用于解决兼容性、闪退或性能异常问题");

    appendParamInfo("-nopreload", "", "禁用资源预加载（减少卡顿）");

    appendParamInfo("-forcenovsync", "", "强制关闭垂直同步");


    // 加载参数配置
    loadParamInfo();
}

ParamCheckWidget::~ParamCheckWidget()
{
    delete ui;
}

/**
* @author   XiaoAn
* @brief    显示在父窗口中间
* @date     2026-04-01
**/
void ParamCheckWidget::showCenter()
{
    move((parentWidget()->width() - width()) / 2, (parentWidget()->height() - height()) / 2);
    show();
}

/**
* @author   XiaoAn
* @brief    启用参数
* @date     2026-04-02
**/
void ParamCheckWidget::checkParam(const QList<QPair<QString, QString>> &paramList)
{
    // 设置选中框和参数编辑框
    foreach (auto pair, paramList) {
        QPair<QCheckBox*, QLineEdit*> widgetPair = m_paramWidgetMap.value(pair.first);
        widgetPair.first->setChecked(true);
        if(!pair.second.isEmpty()){
            widgetPair.second->setText(pair.second);
        }
    }
    // 重新生成启动参数
    genGameParamStr();
}

/**
* @author   XiaoAn
* @brief    加载参数信息
* @date     2026-04-01
**/
void ParamCheckWidget::loadParamInfo()
{
    QString paramStr = GameManager::getInstance()->gameParam();

    QStringList paramList = paramStr.split(' ');

    for (int i = 0; i < paramList.size(); ++i) {
        QString key = paramList.at(i);
        // 参数类型
        if(!key.startsWith('-')){
            continue;
        }

        if(!m_paramWidgetMap.contains(key)){
            continue;
        }

        QPair<QCheckBox*, QLineEdit*> widgetPair = m_paramWidgetMap.value(key);

        // 启用参数
        widgetPair.first->setChecked(true);
        // 设置参数值
        if(i + 1 < paramList.size() && !paramList.at(i + 1).startsWith('-')){
            widgetPair.second->setText(paramList.at(i + 1));
            i++;
        }
    }

}

/**
* @author   XiaoAn
* @brief    添加单个参数
* @date     2026-04-01
**/
void ParamCheckWidget::appendParamInfo(const QString &key, const QString &value, const QString &description)
{
    ui->tableWidget->setRowCount(ui->tableWidget->rowCount() + 1);
    int row = ui->tableWidget->rowCount() - 1;

    QCheckBox *checkBox = new QCheckBox(this);
    checkBox->setStyleSheet("margin-left: 10px;");
    QTableWidgetItem *itemKey = new QTableWidgetItem(key);
    QLineEdit *lineEdit = new QLineEdit(value, this);
    if(value.isEmpty()){
        lineEdit->setDisabled(true);
    }
    QTableWidgetItem *itemInstruction = new QTableWidgetItem(description);

    ui->tableWidget->setCellWidget(row, 0, checkBox);
    ui->tableWidget->setItem(row, 1, itemKey);
    ui->tableWidget->setCellWidget(row, 2, lineEdit);
    ui->tableWidget->setItem(row, 3, itemInstruction);

    m_paramWidgetMap.insert(key, {checkBox, lineEdit});
}

/**
* @author   XiaoAn
* @brief    设置游戏参数
* @date     2026-04-01
**/
void ParamCheckWidget::genGameParamStr()
{
    QStringList paramStrList;
    for (auto it = m_paramWidgetMap.begin(); it != m_paramWidgetMap.end(); ++it) {
        if(!it.value().first->isChecked()) continue;

        paramStrList.append(it.key());
        QString value = it.value().second->text();
        if(!value.isEmpty()){
            paramStrList.append(value);
        }
    }
    GameManager::getInstance()->setGameParam(paramStrList.join(" "));
    QMessageBox::information(this, "提示", "游戏启动参数设置完毕!", QMessageBox::Ok);
    close();
}


