#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDebug>

#include "comp/categorydialog.h"
#include "comp/categorybutton.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 全局互斥选中按钮
    m_buttonGroup.addButton(ui->pushButton_workshop);
    m_buttonGroup.addButton(ui->pushButton_localMod);
    m_buttonGroup.addButton(ui->pushButton_trashMod);
    m_buttonGroup.setExclusive(true);

    // 设置菜单
    m_settingMenu = new QMenu(this);
    m_settingMenu->setWindowFlags(Qt::Popup);
    m_settingMenu->addAction("设置路径", this, [=](){
        QString gamePath = QFileDialog::getExistingDirectory(this, "选择游戏根路径", QDir::homePath(), QFileDialog::ShowDirsOnly);
        if(gamePath.isEmpty() || !ModManager::getInstance()->setGamePath(gamePath)) return;
    });
    m_settingMenu->addAction("打开路径", this, [=](){
        QString gamePath = ModManager::getInstance()->gamePath();
        if(QDir(gamePath).exists()){
            QDesktopServices::openUrl(QUrl::fromLocalFile(gamePath));
        }
    });

    // 弹出设置菜单
    connect(ui->pushButton_setting, &QPushButton::clicked, this, [=](){
        // 获取按钮在屏幕上的绝对位置
        QPoint globalBtnPos = ui->pushButton_setting->mapToGlobal(QPoint(0, 0));
        m_settingMenu->move(globalBtnPos.x(), globalBtnPos.y() + ui->pushButton_setting->height());
        m_settingMenu->show();
    });

    // 启动游戏
    connect(ui->pushButton_startGame, &QPushButton::clicked, this, [=](){
        QString gamePath = ModManager::getInstance()->gamePath();
        if(gamePath.isEmpty()){
            QMessageBox::warning(this, "警告", "请先设置游戏路径", QMessageBox::Ok);
            return;
        }
    });

    // 添加自定义分类
    QList<CategoryInfo> categoryList = SqliteObj::getInstance()->getCategoryList();
    foreach (const auto &category, categoryList) {
        addCategoryButton(category);
    }

    // 添加自定义分类
    connect(ui->pushButton_addCategory, &QPushButton::clicked, this, [=](){
        CategoryDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString categoryName = dialog.getCategory();
            if(categoryName.isEmpty()) return;
            // 插入数据库
            if (!SqliteObj::getInstance()->appendCategory(categoryName)) {
                QMessageBox::warning(this, "错误", "分类添加失败！", QMessageBox::Ok);
                return;
            }
            addCategoryButton(SqliteObj::getInstance()->getCategoryInfo(categoryName));
        }
    });

    // 加载工坊Mod文件信息
    connect(ui->pushButton_workshop, &QPushButton::clicked, this, [=](){
        QList<ModInfo> modInfoList = ModManager::getInstance()->scanDirModInfo(ModManager::WorkshopDir);
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(modInfoList);
    });

    // 加载本地Mod文件信息
    connect(ui->pushButton_localMod, &QPushButton::clicked, this, [=](){
        QList<ModInfo> modInfoList = ModManager::getInstance()->scanDirModInfo(ModManager::ModLocalDir);
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(modInfoList);
    });

    // 加载禁用Mod文件信息
    connect(ui->pushButton_trashMod, &QPushButton::clicked, this, [=](){
        QList<ModInfo> modInfoList = ModManager::getInstance()->scanDirModInfo(ModManager::ModTrashDir);
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(modInfoList);
    });

    // 默认刷新
    emit ui->pushButton_workshop->clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
* @author   XiaoAn
* @brief    添加分类按钮
* @date     2026-02-26
**/
void MainWindow::addCategoryButton(const CategoryInfo &category)
{
    // 插入成功，动态生成分类按钮
    CategoryButton *btn = new CategoryButton(category.name, this);
    m_buttonGroup.addButton(btn->coreButton());
    QVBoxLayout *vboxLayout = qobject_cast<QVBoxLayout*>(ui->categoryList->layout());
    vboxLayout->insertWidget(vboxLayout->count() - 1, btn);

    // 绑定点击事件
    connect(btn, &CategoryButton::clicked, this, [=](){
        // 查询该分类的Mod信息
        QList<ModInfo> modInfoList = SqliteObj::getInstance()->getModInfoList(category.id);
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(modInfoList);
    });

    connect(btn, &CategoryButton::deleteCategory, this, [=](){

        QList<ModInfo> modInfoList = SqliteObj::getInstance()->getModInfoList(category.id);

        if(!modInfoList.isEmpty()){
            int opt = QMessageBox::question(this, "确认删除", "该分类存在Mod文件信息，是否清空!", QMessageBox::Ok | QMessageBox::Cancel);
            if(opt == QMessageBox::Cancel){
                return;
            }
        }

        // 删除分类
        bool ret = SqliteObj::getInstance()->removeCategory(category.id);
        if(ret){
            // 移除按钮
            m_buttonGroup.removeButton(btn->coreButton());
            btn->deleteLater();

            // 默认刷新
            ui->pushButton_workshop->click();
        }else{
            QMessageBox::warning(this, "错误", "删除失败!", QMessageBox::Ok);
        }
    });
}



/**
* @author   XiaoAn
* @brief    刷新Mod统计信息
* @date     2026-02-27
**/
void MainWindow::refreshModCount()
{
    // Mod总数
    QList<ModInfo> modInfoList = SqliteObj::getInstance()->getModInfoList();
    ui->label_totalNum->setText(QString::number( modInfoList.size() ));

    // 未分类、已分类、已禁用、总占用
    int unclassifiedCount = 0, classified = 0, disabledCount = 0;
    quint64 totalOccupied = 0;
    foreach (const auto &modInfo, modInfoList) {
        if(modInfo.relative_path == ModManager::ModTrashDir){
            disabledCount++;
        }
        QList<CategoryInfo> catList = SqliteObj::getInstance()->getModCategorys(modInfo.id);
        catList.isEmpty() ? unclassifiedCount++ : classified++;

        QFile file(ModManager::getInstance()->gamePath() + modInfo.relative_path + "/" + modInfo.original_name + ".vpk");
        if(file.exists() && modInfo.relative_path != ModManager::ModTrashDir){
            totalOccupied += file.size();
        }
    }
    ui->label_unclassified->setText(QString::number(unclassifiedCount));
    ui->label_classified->setText(QString::number(classified));
    ui->label_disabled->setText(QString::number(disabledCount));
    ui->label_occupied->setText(ModManager::getFileSizeWithUnit(totalOccupied));
}
