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

#include <windows.h>
#include <tlhelp32.h>
#include "comp/categorydialog.h"
#include "comp/categorybutton.h"
#include "utils/modconflictdetector.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("L4D2-Mod管理工具");

    initWidget();

    // 全局互斥选中按钮
    m_buttonGroup.addButton(ui->pushButton_workshop);
    m_buttonGroup.addButton(ui->pushButton_localMod);
    m_buttonGroup.addButton(ui->pushButton_trashMod);
    m_buttonGroup.addButton(ui->pushButton_conflict);
    m_buttonGroup.setExclusive(true);

    // 共同行为
    foreach (QAbstractButton *btn, m_buttonGroup.buttons()) {
        connect(btn, &QAbstractButton::clicked, this, [=](){
            // 重置搜索、分类过滤、刷新统计信息
            ui->lineEdit_search->clear();
            m_ckListWidget->resetOptionsChecked(true);
            refreshModCount();
        });
    }

    // 导入vpk文件到本地
    connect(ui->pushButton_importMod, &QPushButton::clicked, this, &MainWindow::importMod);

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

    // 打开创意工坊网页
    connect(ui->pushButton_openworkshop, &QPushButton::clicked, this, [=](){
        QDesktopServices::openUrl(QUrl("https://steamcommunity.com/app/550/workshop/"));
    });

    // 启动游戏
    connect(ui->pushButton_startGame, &QPushButton::clicked, this, &MainWindow::startGame);

    // 加载工坊Mod文件卡片
    connect(ui->pushButton_workshop, &QPushButton::clicked, this, [=](){
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(GameManager::getInstance()->scanDirModInfo(GameManager::WorkshopDir));
    });

    // 加载本地Mod文件卡片
    connect(ui->pushButton_localMod, &QPushButton::clicked, this, [=](){
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(GameManager::getInstance()->scanDirModInfo(GameManager::ModLocalDir));
    });

    // 加载禁用Mod文件卡片
    connect(ui->pushButton_trashMod, &QPushButton::clicked, this, [=](){
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(GameManager::getInstance()->scanDirModInfo(GameManager::ModTrashDir));
    });

    // 加载冲突Mod文件卡片
    connect(ui->pushButton_conflict, &QPushButton::clicked, this, &MainWindow::checkConflictMod);

    // 搜索Mod
    connect(ui->lineEdit_search, &QLineEdit::textChanged, ui->cardContainer, &CardContainer::slot_searchCard);

    // 默认刷新
    emit ui->pushButton_workshop->clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
* @author   XiaoAn
* @brief    初始化组件
* @date     2026-03-29
**/
void MainWindow::initWidget()
{
    m_paramCheckWidget = new ParamCheckWidget(this);

    // 操作菜单
    m_operationMenu = new QMenu(this);
    m_operationMenu->setWindowFlags(Qt::Popup);
    m_operationMenu->addAction("工坊移至本地", this, &MainWindow::moveToLocal);
    m_operationMenu->addAction("安装dvxk补丁", this, [=](){
        bool isSuccess = GameManager::copyDirectory(
            QCoreApplication::applicationDirPath() + "/patch/dxvk-2.7.1", GameManager::getInstance()->gamePath());
        if(isSuccess){
            // 启用参数
            QList<QPair<QString, QString>> paramList;
            paramList << qMakePair("-vulkan", "") << qMakePair("-d3d9ex", "") << qMakePair("-high", "") << qMakePair("-heapsize", "7500000");
            m_paramCheckWidget->checkParam(paramList);
            QMessageBox::information(this, "提示", "安装成功!", QMessageBox::Ok);
        }else{
            QMessageBox::warning(this, "提示", "安装失败!", QMessageBox::Ok);
        }
    });
    m_operationMenu->addAction("安装L4N平台", this, [=](){
        bool isSuccess = GameManager::copyDirectory(
            QCoreApplication::applicationDirPath() + "/patch/L4N_v2.1.6", GameManager::getInstance()->gamePath());
        if(isSuccess){
            QMessageBox::information(this, "提示", "安装成功!", QMessageBox::Ok);
        }else{
            QMessageBox::warning(this, "提示", "安装失败!", QMessageBox::Ok);
        }
    });

    // 弹出操作菜单
    connect(ui->pushButton_moreOperation, &QPushButton::clicked, this, [=](){
        // 获取按钮在屏幕上的绝对位置
        QPoint globalBtnPos = ui->pushButton_moreOperation->mapToGlobal(QPoint(0, 0));
        m_operationMenu->move(globalBtnPos.x(), globalBtnPos.y() + ui->pushButton_moreOperation->height());
        m_operationMenu->show();
    });

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
    m_settingMenu->addAction("设置启动参数", m_paramCheckWidget, &ParamCheckWidget::showCenter);

    // 弹出设置菜单
    connect(ui->pushButton_setting, &QPushButton::clicked, this, [=](){
        // 获取按钮在屏幕上的绝对位置
        QPoint globalBtnPos = ui->pushButton_setting->mapToGlobal(QPoint(0, 0));
        m_settingMenu->move(globalBtnPos.x(), globalBtnPos.y() + ui->pushButton_setting->height());
        m_settingMenu->show();
    });

    // 筛选面板
    m_ckListWidget = new CheckBoxListWidget(this);
    // 默认添加一个未分类选项
    m_ckListWidget->addOption("未分类");
    connect(m_ckListWidget, &CheckBoxListWidget::optionCheckStateChanged, ui->cardContainer, &CardContainer::slot_setCategoryFilter);

    // 打开筛选面板
    connect(ui->pushButton_categoryFilter, &QPushButton::clicked, this, [=](){
        // 获取按钮在屏幕上的绝对位置
        QPoint globalBtnPos = ui->pushButton_categoryFilter->mapToGlobal(QPoint(0, 0));
        m_ckListWidget->move(globalBtnPos.x(), globalBtnPos.y() + ui->pushButton_categoryFilter->height());
        m_ckListWidget->show();
    });


}

/**
* @author   XiaoAn
* @brief    添加分类信息
* @date     2026-02-26
**/
void MainWindow::addCategory(const CategoryInfo &category)
{
    CategoryButton *btn = ui->widget_category->appendCategory(category);
    m_buttonGroup.addButton(btn->coreButton());

    // 绑定点击事件
    connect(btn, &CategoryButton::clicked, this, [=](){
        // 查询该分类的Mod信息
        QList<ModInfo> modInfoList = SqliteObj::getInstance()->getModInfoList(category.id);
        ui->cardContainer->clearModCard();
        ui->cardContainer->appendModCard(modInfoList, category);
        // 重置搜索、分类过滤、刷新统计信息
        ui->lineEdit_search->clear();
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

bool isGameRunning(const QString &exeName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    bool found = false;

    if (Process32First(hSnapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName.toStdWString().c_str()) == 0) {
                found = true;
                break;
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return found;
}

/**
* @author   XiaoAn
* @brief    启动游戏
* @date     2026-03-02
**/
void MainWindow::startGame()
{
    if(isGameRunning("left4dead2.exe")){
        QMessageBox::warning(this, "错误", "游戏已启动!", QMessageBox::Ok);
        return;
    }

    QString gameParm = GameManager::getInstance()->gameParam();
    QString steamUrl = QString("steam://rungameid/550//%1").arg(gameParm);

    bool success = QDesktopServices::openUrl(QUrl(steamUrl));

    if(!success){
        QMessageBox::warning(this, "错误", "启动失败!", QMessageBox::Ok);
    }
}

/**
* @author   XiaoAn
* @brief    导入mod
* @date     2026-03-29
**/
void MainWindow::importMod()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择vpk文件", QDir::homePath(), "(*.vpk)");
    if(filePath.isEmpty()) return;

    QFileInfo fileInfo(filePath);
    QString gamePath = GameManager::getInstance()->gamePath();
    if(!QFile(filePath).rename(QString("%1/%2/%3").arg(gamePath, GameManager::ModLocalDir, fileInfo.fileName()))){
        QMessageBox::warning(this, "错误", "导入失败!", QMessageBox::Ok);
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
            QString sourFilePath = QString("%1/%2/%3").arg(gamePath, modInfo.relative_path, modInfo.original_name);
            QString destFilePath = QString("%1/%2/%3").arg(gamePath, GameManager::ModLocalDir, modInfo.original_name);
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
            QString filePath1 = QString("%1/%2/%3.vpk").arg(gamePath, modInfo1.relative_path, modInfo1.original_name);

            QString filePath2 = QString("%1/%2/%3.vpk").arg(gamePath, modInfo2.relative_path, modInfo2.original_name);

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
