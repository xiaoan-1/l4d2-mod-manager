#include "modcard.h"
#include "ui_modcard.h"

#include <QImage>
#include <QBuffer>
#include <QImageReader>
#include <QImageIOHandler>
#include <QMessageBox>
#include <QLineEdit>

#include "../gamemanager.h"
#include "../utils/vpkfileparser.h"

ModCard::ModCard(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ModCard)
{
    ui->setupUi(this);
}

ModCard::~ModCard()
{
    delete ui;
}

ModCard::ModCard(const ModInfo &modInfo, QWidget *parent, SizeMode sizeMode)
    : ModCard(parent)
{
    m_modInfo = modInfo;

    setSizeMode(sizeMode);

    updateModInfo();

    // 删除按钮
    m_removeButton = new QPushButton(this);
    m_removeButton->setStyleSheet("padding: 0px;");
    m_removeButton->hide();

    // 给下拉框安装过滤器，忽略滚轮事件保证容器可以滚动
    ui->comboBox_categorys->installEventFilter(this);

    // 给图片安装过滤，保证图片自适应大小
    ui->label_img->installEventFilter(this);

    // 备注
    connect(ui->pushButton_remark, &QPushButton::clicked, this, &ModCard::remark);

    // 转移
    connect(ui->pushButton_move, &QPushButton::clicked, this, &ModCard::transfer);

    // 分类
    connect(ui->pushButton_affirm, &QPushButton::clicked, this, &ModCard::classify);

    // 按钮行为
    connect(m_removeButton, &QPushButton::clicked, this, &ModCard::remove);
}

/**
* @author   XiaoAn
* @brief    设置大小模式
* @date     2026-04-20
**/
void ModCard::setSizeMode(SizeMode sizeMode)
{
    if(sizeMode == SizeMode::Conflict){
        ui->label_s->hide();
        ui->label_size->hide();
        ui->pushButton_remark->hide();
        ui->pushButton_affirm->hide();
        ui->comboBox_categorys->hide();
    }
}

/**
* @author   XiaoAn
* @brief    设置当前Mod分类
* @date     2026-02-26
**/
void ModCard::setCurrentCategory(const CategoryInfo &category)
{
    if(category.id == -1){
        return;
    }
    m_category = category;
}

/**
* @author   XiaoAn
* @brief    是否存在分类
* @date     2026-03-04
**/
bool ModCard::hasCategory(const QString &catName)
{
    foreach (const auto &category, m_classifiedList) {
        if(category.name == catName){
            return true;
        }
    }
    return false;
}

/**
* @author   XiaoAn
* @brief    图片大小
* @date     2026-03-14
**/
QSize ModCard::getImageSize()
{
    return ui->label_img->size();
}


/**
* @author   XiaoAn
* @brief    刷新Mod信息
* @date     2026-03-02
**/
void ModCard::updateModInfo()
{
    QString filePath = QString("%1/%2/%3.vpk").
                       arg(GameManager::getInstance()->gamePath(), m_modInfo.relative_path, m_modInfo.original_name);

    QMap<QString, QString> addonInfoMap = VpkFileParser(filePath).getAddonInfo();
    // 备注名称
    QString addontitle = addonInfoMap.value("addontitle");
    if(!m_modInfo.custom_name.isEmpty()){
        ui->label_name->setText(m_modInfo.custom_name);
        ui->label_name->setToolTip(m_modInfo.custom_name);
    }else if(!addontitle.isEmpty()){
        ui->label_name->setText(addontitle);
        ui->label_name->setToolTip(addontitle);
    }else{
        ui->label_name->setText(m_modInfo.original_name);
        ui->label_name->setToolTip(m_modInfo.original_name);
    }

    // 文件大小
    QFile modFile(filePath);
    if(modFile.exists()){
        QString sizeStr = GameManager::getFileSizeWithUnit(modFile.size());
        ui->label_size->setText(sizeStr);
    }else {
        ui->label_size->setText("0kb");
    }

    // 获取所有分类列表
    QList<CategoryInfo> categoryList = SqliteObj::getInstance()->getCategoryList();
    // 获取当前Mod的分类列表
    m_classifiedList = SqliteObj::getInstance()->getModCategorys(m_modInfo.id);

    // 添加该Mod未分类信息到下拉框中
    ui->comboBox_categorys->clear();
    foreach (const auto &category, categoryList) {
        if(!hasCategory(category.name)){
            ui->comboBox_categorys->addItem(category.name);
        }
    }

    if(m_modInfo.relative_path == GameManager::ModTrashDir){
        ui->pushButton_move->setText("启用");
        ui->pushButton_move->setIcon(QIcon(":/resources/enable.png"));
        ui->pushButton_move->setStyleSheet("#pushButton_move:hover{background:#28a745;}");
        ui->centralwidget->setStyleSheet("#centralwidget{border: 0px;border-radius: 12px;background-color:#1c2027;}");
    }else{
        ui->pushButton_move->setText("禁用");
        ui->pushButton_move->setIcon(QIcon(":/resources/disable.png"));
        ui->pushButton_move->setStyleSheet("#pushButton_move:hover{background:#dc3545;}");
        ui->centralwidget->setStyleSheet("#centralwidget{border: 0px;border-radius: 12px;background-color:#465975;}");
    }
}


/**
* @author   XiaoAn
* @brief    加载Mod图片
* @date     2026-02-25
**/
void ModCard::loadImage(const QImage &image)
{
    if (image.isNull()) return;
    ui->label_img->setPixmap(QPixmap::fromImage(image));
}

/**
* @author   XiaoAn
* @brief    设置图片加载失败信息
* @date     2026-04-16
**/
void ModCard::setImageErrorText(const QString &errorStr)
{
    ui->label_img->setText(errorStr);
}



/**
* @author   XiaoAn
* @brief    备注
* @date     2026-02-27
**/
void ModCard::remark()
{
    if(m_remarkEdit){
        QString newName = m_remarkEdit->text();
        // 修改数据库Mod自定义名称
        bool ret = SqliteObj::getInstance()->updateModCustomName(m_modInfo.id, newName);
        if(ret){
            m_modInfo.custom_name = newName;
        }else{
            QMessageBox::warning(this, "错误", "备注失败！！", QMessageBox::Ok);
        }
        updateModInfo();
        m_remarkEdit->deleteLater();
        m_remarkEdit = nullptr;
        ui->pushButton_remark->setText("备注");
    }else{
        m_remarkEdit = new QLineEdit(this);
        m_remarkEdit->setGeometry(ui->label_name->geometry());
        m_remarkEdit->setText(ui->label_name->text());
        m_remarkEdit->show();
        m_remarkEdit->raise();
        ui->pushButton_remark->setText("确认");
    }
}

/**
* @author   XiaoAn
* @brief    移动模组（启用和禁用）
* @date     2026-02-27
**/
void ModCard::transfer()
{
    bool isDisable = false;
    QString destRelativePath, btnText;
    QString gamePath = GameManager::getInstance()->gamePath();
    if(m_modInfo.relative_path == GameManager::ModTrashDir){
        // 启用模组
        btnText = "禁用";
        destRelativePath = GameManager::ModLocalDir;
        ui->centralwidget->setStyleSheet("#centralwidget{border: 0px;border-radius: 12px;background-color:#465975;}");
    }else{
        // 禁用模组
        btnText = "启用";
        destRelativePath = GameManager::ModTrashDir;
        ui->centralwidget->setStyleSheet("#centralwidget{border: 0px;border-radius: 12px;background-color:#1c2027;}");
        isDisable = true;
    }

    QString sourFilePath = QString("%1/%2/%3").arg(gamePath, m_modInfo.relative_path, m_modInfo.original_name);
    QString destFilePath = QString("%1/%2/%3").arg(gamePath, destRelativePath, m_modInfo.original_name);
    QFile::rename(sourFilePath + ".jpg", destFilePath + ".jpg");

    if(QFile::rename(sourFilePath + ".vpk", destFilePath + ".vpk")){
        // 修改记录
        if(SqliteObj::getInstance()->updateModRelativePath(m_modInfo.id, destRelativePath)){
            m_modInfo.relative_path = destRelativePath;
            ui->pushButton_move->setText(btnText);
            emit toggleDisabled(isDisable);
        }else{
            // 移回回收文件夹
            QFile::rename(destFilePath + ".vpk", sourFilePath + ".vpk");
            QFile::rename(destFilePath + ".vpk", sourFilePath + ".vpk");
            QMessageBox::warning(this, "错误", "数据库修改路径失败，文件已还原位置", QMessageBox::Ok);
        }
    }else{
        QMessageBox::warning(this, "错误", "文件移动失败，可能存在同名文件", QMessageBox::Ok);
    }
}

/**
* @author   XiaoAn
* @brief    分类
* @date     2026-02-26
**/
void ModCard::classify()
{
    CategoryInfo categoryInfo = SqliteObj::getInstance()->getCategoryInfo(ui->comboBox_categorys->currentText());
    bool ret = SqliteObj::getInstance()->setModCategory(m_modInfo.id, categoryInfo.id);

    if(!ret){
        QMessageBox::warning(this, "错误", "分类设置失败!", QMessageBox::Ok);
        return;
    }
    m_classifiedList.append(categoryInfo);
    QMessageBox::information(this, "提示", "分类设置成功!", QMessageBox::Ok);
    ui->comboBox_categorys->removeItem(ui->comboBox_categorys->currentIndex());
    emit classified();
}

/**
* @author   XiaoAn
* @brief    移除分类或者删除模组文件
* @date     2026-04-20
**/
void ModCard::remove()
{
    if(m_category.id != -1){
        // 在分类中该按钮为移除分类
        bool ret = SqliteObj::getInstance()->removeModCategory(m_modInfo.id, m_category.id);
        if(ret){
            // 由父容器来销毁
            emit removeCategory();
        }else{
            QMessageBox::warning(this, "错误", "移除分类失败！", QMessageBox::Ok);
        }
    }else if(m_modInfo.relative_path == GameManager::ModTrashDir){
        // 如果在回收站，则该按钮为删除按钮
        if(QMessageBox::Ok != QMessageBox::question(this, "提示", "是否删除该模组？", QMessageBox::Ok | QMessageBox::Cancel)){
            return;
        }
        QFile vpkFile(QString("%1/%2/%3.vpk").arg(GameManager::getInstance()->gamePath(),
                                                  m_modInfo.relative_path, m_modInfo.original_name));
        QFile imgFile(QString("%1/%2/%3.jpg").arg(GameManager::getInstance()->gamePath(),
                                                  m_modInfo.relative_path,m_modInfo.original_name));
        if(vpkFile.moveToTrash()){
            imgFile.moveToTrash();
            SqliteObj::getInstance()->removeModInfo(m_modInfo.id);
            QMessageBox::information(this, "提示", "已删除模组至系统回收站", QMessageBox::Ok);
            // 由父容器来销毁
            emit removeModFile();
        }else {
            QMessageBox::warning(this, "错误", "模组删除失败！", QMessageBox::Ok);
        }
    }
}

/**
* @author   XiaoAn
* @brief    鼠标移入
* @date     2026-03-14
**/
void ModCard::enterEvent(QEnterEvent *event)
{
    if(m_category.id != -1){
        m_removeButton->setText("移除分类");
        m_removeButton->setIcon(QIcon(":/resources/remove.png"));
        m_removeButton->move(width() - m_removeButton->width(), 0);
        m_removeButton->show();
    }else if(m_modInfo.relative_path == GameManager::ModTrashDir){
        m_removeButton->setText("删除模组");
        m_removeButton->setIcon(QIcon(":/resources/delete.png"));
        m_removeButton->move(width() - m_removeButton->width(), 0);
        m_removeButton->show();
    }
    QWidget::enterEvent(event);
}

/**
* @author   XiaoAn
* @brief    鼠标移出
* @date     2026-03-14
**/
void ModCard::leaveEvent(QEvent *event)
{
    m_removeButton->hide();
    QWidget::leaveEvent(event);
}

/**
* @author   XiaoAn
* @brief    事件过滤
* @date     2026-03-14
**/
bool ModCard::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->comboBox_categorys && event->type() == QEvent::Wheel) {
        // 忽略滚轮事件，使其传递给父窗口
        event->ignore();
        return true;
    }

    if (obj == ui->label_img) {
        if(event->type() == QEvent::Resize){
            emit imgResize();
        }else if(event->type() == QEvent::MouseButtonPress){
            emit clicked();
        }
    }

    return QWidget::eventFilter(obj, event);
}

