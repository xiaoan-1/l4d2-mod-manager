#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QProcess>
#include <QPlainTextEdit>
#include <QDebug>

#include "comp/categorydialog.h"
#include "comp/categorybutton.h"
#include "comp/modconflictdetector.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 全局互斥选中按钮
    m_buttonGroup.addButton(ui->pushButton_workshop);
    m_buttonGroup.addButton(ui->pushButton_localMod);
    m_buttonGroup.addButton(ui->pushButton_trashMod);
    m_buttonGroup.addButton(ui->pushButton_conflict);
    m_buttonGroup.setExclusive(true);

    // 设置菜单
    m_settingMenu = new QMenu(this);
    m_settingMenu->setWindowFlags(Qt::Popup);
    m_settingMenu->addAction("设置游戏路径", this, [=](){
        QString gamePath = QFileDialog::getExistingDirectory(this, "选择游戏根路径", QDir::homePath(), QFileDialog::ShowDirsOnly);
        if(gamePath.isEmpty() || !GameManager::getInstance()->setGamePath(gamePath)) return;
    });
    m_settingMenu->addAction("打开游戏路径", this, [=](){
        QString gamePath = GameManager::getInstance()->gamePath();
        if(QDir(gamePath).exists()){
            QDesktopServices::openUrl(QUrl::fromLocalFile(gamePath));
        }
    });
    m_settingMenu->addAction("设置启动参数", this, &MainWindow::showGameParamDilaog);;

    // 筛选面板
    m_ckListWidget = new CheckBoxListWidget(this);
    connect(m_ckListWidget, &CheckBoxListWidget::optionCheckStateChanged, this, [=](QString categoryName, bool checked){
        ui->cardContainer->filterCard(categoryName, checked);
    });
    // 默认添加一个未分类选项
    m_ckListWidget->addOption("未分类");

    // 启动游戏
    connect(ui->pushButton_startGame, &QPushButton::clicked, this, &MainWindow::startGame);

    // 打开创意工坊网页
    connect(ui->pushButton_openworkshop, &QPushButton::clicked, this, [=](){
        QDesktopServices::openUrl(QUrl("https://steamcommunity.com/app/550/workshop/"));
    });

    // 弹出设置菜单
    connect(ui->pushButton_setting, &QPushButton::clicked, this, [=](){
        // 获取按钮在屏幕上的绝对位置
        QPoint globalBtnPos = ui->pushButton_setting->mapToGlobal(QPoint(0, 0));
        m_settingMenu->move(globalBtnPos.x(), globalBtnPos.y() + ui->pushButton_setting->height());
        m_settingMenu->show();
    });

    // 加载工坊Mod文件卡片
    connect(ui->pushButton_workshop, &QPushButton::clicked, this, [=](){
        QList<ModInfo> modInfoList = GameManager::getInstance()->scanDirModInfo(GameManager::WorkshopDir);
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(modInfoList);
        // 重置过滤
        m_ckListWidget->resetOptionsChecked(true);
        refreshModCount();
    });

    // 加载本地Mod文件卡片
    connect(ui->pushButton_localMod, &QPushButton::clicked, this, [=](){
        QList<ModInfo> modInfoList = GameManager::getInstance()->scanDirModInfo(GameManager::ModLocalDir);
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(modInfoList);
        // 重置过滤
        m_ckListWidget->resetOptionsChecked(true);
        refreshModCount();
    });

    // 加载禁用Mod文件卡片
    connect(ui->pushButton_trashMod, &QPushButton::clicked, this, [=](){
        QList<ModInfo> modInfoList = GameManager::getInstance()->scanDirModInfo(GameManager::ModTrashDir);
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(modInfoList);
        // 重置过滤
        m_ckListWidget->resetOptionsChecked(true);
        refreshModCount();
    });

    // 加载冲突Mod文件卡片
    connect(ui->pushButton_conflict, &QPushButton::clicked, this, &MainWindow::checkConflictMod);

    // 添加自定义分类到导航栏
    QList<CategoryInfo> categoryList = SqliteObj::getInstance()->getCategoryList();
    foreach (const auto &category, categoryList) {
        addCategory(category);
    }

    // 手动添加自定义分类
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
            addCategory(SqliteObj::getInstance()->getCategoryInfo(categoryName));
        }
    });

    // 移至本地
    connect(ui->pushButton_moveLocal, &QPushButton::clicked, this, &MainWindow::moveToLocal);

    // 自动整理mod
    connect(ui->pushButton_autoOrganize, &QPushButton::clicked, this, &MainWindow::autoOrganizeMod);

    // 搜索Mod
    connect(ui->lineEdit_search, &QLineEdit::textChanged, ui->cardContainer, &CardContainer::slot_searchCard);

    // 打开筛选面板
    connect(ui->pushButton_categoryFilter, &QPushButton::clicked, this, [=](){
        // 获取按钮在屏幕上的绝对位置
        QPoint globalBtnPos = ui->pushButton_categoryFilter->mapToGlobal(QPoint(0, 0));
        m_ckListWidget->move(globalBtnPos.x(), globalBtnPos.y() + ui->pushButton_categoryFilter->height());
        m_ckListWidget->show();
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
* @brief    添加分类信息
* @date     2026-02-26
**/
void MainWindow::addCategory(const CategoryInfo &category)
{
    // 1、生成分类按钮
    CategoryButton *btn = new CategoryButton(category, this);
    m_buttonGroup.addButton(btn->coreButton());
    QVBoxLayout *vboxLayout = qobject_cast<QVBoxLayout*>(ui->categoryList->layout());
    vboxLayout->insertWidget(vboxLayout->count() - 1, btn);

    // 绑定点击事件
    connect(btn, &CategoryButton::clicked, this, [=](){
        // 查询该分类的Mod信息
        QList<ModInfo> modInfoList = SqliteObj::getInstance()->getModInfoList(category.id);
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(modInfoList, category);
        // 重置过滤
        m_ckListWidget->resetOptionsChecked(true);
        refreshModCount();
    });

    // 重命名分类
    connect(btn, &CategoryButton::renamed, this, [=](QString oldName, QString newName){
        m_ckListWidget->renameOption(oldName, newName);
        ui->cardContainer->updateModCard();
    });

    // 删除分类
    connect(btn, &CategoryButton::deleted, this, [=](){
        // 移除按钮
        m_buttonGroup.removeButton(btn->coreButton());
        // 移除分类筛选中的分类
        m_ckListWidget->removeOption(category.name);
        btn->deleteLater();

        // 刷新Mod信息
        ui->cardContainer->updateModCard();
    });

    // 2、同步添加筛选项
    m_ckListWidget->addOption(category.name);

    // 3、刷新Mod卡片信息, 增加分类选项
    ui->cardContainer->updateModCard();
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
        if(modInfo.relative_path == GameManager::ModTrashDir){
            disabledCount++;
        }
        QList<CategoryInfo> catList = SqliteObj::getInstance()->getModCategorys(modInfo.id);
        catList.isEmpty() ? unclassifiedCount++ : classified++;

        QFile file(GameManager::getInstance()->gamePath() + modInfo.relative_path + "/" + modInfo.original_name + ".vpk");
        if(file.exists() && modInfo.relative_path != GameManager::ModTrashDir){
            totalOccupied += file.size();
        }
    }
    ui->label_unclassified->setText(QString::number(unclassifiedCount));
    ui->label_classified->setText(QString::number(classified));
    ui->label_disabled->setText(QString::number(disabledCount));
    ui->label_occupied->setText(GameManager::getFileSizeWithUnit(totalOccupied));
}

/**
* @author   XiaoAn
* @brief    设置游戏启动参数
* @date     2026-03-02
**/
void MainWindow::showGameParamDilaog()
{
    // 创建对话框
    QDialog dialog(this);
    dialog.setWindowTitle("输入信息");
    dialog.setModal(true);
    dialog.resize(300, 150);

    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 添加提示文本
    QLabel *label_tip = new QLabel("请输入内容：", &dialog);
    layout->addWidget(label_tip);

    // 添加输入框
    QPlainTextEdit *textEdit = new QPlainTextEdit(&dialog);
    QString gameParam = GameManager::getInstance()->gameParam();
    if(gameParam.isEmpty()){
        textEdit->setPlaceholderText("输入启动参数......");
    }else{
        textEdit->setPlainText(gameParam);
    }

    layout->addWidget(textEdit);

    // 添加按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    layout->addWidget(buttonBox);

    // 连接按钮信号
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString inputText = textEdit->toPlainText();
        GameManager::getInstance()->setGameParam(inputText);
    }
}

/**
* @author   XiaoAn
* @brief    启动游戏
* @date     2026-03-02
**/
void MainWindow::startGame()
{
    QString gameParm = GameManager::getInstance()->gameParam();
    QString steamUrl = QString("steam://rungameid/550//%1").arg(gameParm);

    bool success = QDesktopServices::openUrl(QUrl(steamUrl));

    if(!success){
        QMessageBox::warning(this, "错误", "启动失败!", QMessageBox::Ok);
    }
}

/**
* @author   XiaoAn
* @brief    工坊Mod文件移动至本地
* @date     2026-03-08
**/
void MainWindow::moveToLocal()
{
    QString gamePath = GameManager::getInstance()->gamePath();

    QList<ModInfo> modInfoList = SqliteObj::getInstance()->getModInfoList();
    foreach (const ModInfo &modInfo, modInfoList) {
        // 工坊的Mod移入本地目录
        if(modInfo.relative_path == GameManager::WorkshopDir){
            QString sourFilePath = QString("%1/%2/%3").arg(gamePath).arg(modInfo.relative_path).arg(modInfo.original_name);
            QString destFilePath = QString("%1/%2/%3").arg(gamePath).arg(GameManager::ModLocalDir).arg(modInfo.original_name);
            QFile::rename(sourFilePath + ".jpg", destFilePath + ".jpg");
            if(QFile::rename(sourFilePath + ".vpk", destFilePath + ".vpk")){
                if(!SqliteObj::getInstance()->updateModRelativePath(modInfo.id, GameManager::ModLocalDir)){
                    // 数据修改失败，则移回文件
                    QFile::rename(destFilePath + ".vpk", sourFilePath + ".vpk");
                    QFile::rename(destFilePath + ".jpg", sourFilePath + ".jpg");
                }
            }
        }
    }
    QMessageBox::information(this, "提示", "已将工坊Mod文件移至本地，请在创意工坊中取消订阅", QMessageBox::Ok);
    QDesktopServices::openUrl(QUrl("https://steamcommunity.com/app/550/workshop/"));
}

/**
* @author   XiaoAn
* @brief    自动整理Mod
* @date     2026-03-07
**/
void MainWindow::autoOrganizeMod()
{
    QMessageBox::information(this, "提示", "待实现", QMessageBox::Ok);
}

/**
* @author   XiaoAn
* @brief    检测冲突Mod
* @date     2026-03-11
**/
void MainWindow::checkConflictMod()
{
    QList<ModInfo> modInfoList = GameManager::getInstance()->scanDirModInfo(GameManager::ModLocalDir);

    QString gamePath = GameManager::getInstance()->gamePath();

    QList<ModInfo> conflictModList;
    for (int i = 0; i < modInfoList.size(); ++i) {
        for (int j = i + 1; j < modInfoList.size(); ++j) {
            ModInfo modInfo1 = modInfoList.at(i);
            ModInfo modInfo2 = modInfoList.at(j);
            QString filePath1 = QString("%1/%2/%3.vpk").arg(gamePath)
                                    .arg(modInfo1.relative_path).arg(modInfo1.original_name);

            QString filePath2 = QString("%1/%2/%3.vpk").arg(gamePath)
                                    .arg(modInfo2.relative_path).arg(modInfo2.original_name);

            if(ModConflictDetector::checkConflict(filePath1, filePath2)){
                if(!conflictModList.contains(modInfo1)){
                    conflictModList.append(modInfo1);
                }
                if(!conflictModList.contains(modInfo2)){
                    conflictModList.append(modInfo2);
                }
            }
        }
    }

    ui->cardContainer->clearModCard();
    ui->cardContainer->appendModCard(conflictModList);
}
