#include "modcard.h"
#include "ui_modcard.h"

#include <QImage>
#include <QBuffer>
#include <QImageReader>
#include <QImageIOHandler>
#include <QMessageBox>
#include <QLineEdit>

#include "../gamemanager.h"

ModCard::ModCard(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ModCard)
{
    ui->setupUi(this);

    show();
    setFixedSize(300, 300);
}

ModCard::~ModCard()
{
    delete ui;
}

ModCard::ModCard(const ModInfo &modInfo, QWidget *parent)
    : ModCard(parent)
{
    m_modInfo = modInfo;
    updateModInfo();

    // 备注
    connect(ui->pushButton_remark, &QPushButton::clicked, this, &ModCard::remark);

    // 转移
    connect(ui->pushButton_move, &QPushButton::clicked, this, &ModCard::transfer);

    // 分类
    connect(ui->pushButton_affirm, &QPushButton::clicked, this, &ModCard::classify);

    // 给下拉框安装过滤器，忽略滚轮事件保证容器可以滚动
    ui->comboBox_categorys->installEventFilter(this);

    // 可选性
    m_checkBox = new QCheckBox(this);
    m_checkBox->move(0, 0);
    m_checkBox->hide();

    // 操作按钮
    m_button = new QPushButton("删除模组", this);
    m_button->setStyleSheet("padding: 0px;");
    m_button->setIcon(QIcon(":/resources/delete.png"));
    m_button->move(width() - m_button->width(), 0);
    m_button->hide();

    // 按钮行为
    connect(m_button, &QPushButton::clicked, this, [=](){
        if(m_category.id != -1){
            // 在分类中该按钮为移除分类
            bool ret = SqliteObj::getInstance()->removeModCategory(m_modInfo.id, m_category.id);
            if(ret){
                // 由父容器来销毁
                emit destroyCard(m_modInfo.id);
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
            if(!vpkFile.moveToTrash()){
                imgFile.moveToTrash();
                QMessageBox::information(this, "提示", "已删除模组至系统回收站", QMessageBox::Ok);
                // 由父容器来销毁
                emit destroyCard(m_modInfo.id);
            }
        }
    });
}

/**
* @author   XiaoAn
* @brief    获取Mod信息
* @date     2026-02-28
**/
ModInfo ModCard::modInfo() const
{
    return m_modInfo;
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
* @brief    加载Mod图片
* @date     2026-02-25
**/
void ModCard::loadImage(const QImage &image)
{
    if (image.isNull()) return;

    // 设置图片
    ui->label_img->setPixmap(QPixmap::fromImage(image));
    ui->label_img->setAlignment(Qt::AlignCenter);
}

/**
* @author   XiaoAn
* @brief    刷新Mod信息
* @date     2026-03-02
**/
void ModCard::updateModInfo()
{
    // 备注名称
    if(m_modInfo.custom_name.isEmpty()){
        ui->label_name->setText(m_modInfo.original_name);
    }else{
        ui->label_name->setText(m_modInfo.custom_name);
    }

    QString baseDir = GameManager::getInstance()->gamePath() + m_modInfo.relative_path;

    // 文件大小
    QFile modFile(baseDir + "/" + m_modInfo.original_name + ".vpk");
    if(modFile.exists()){
        QString sizeStr = GameManager::getFileSizeWithUnit(modFile.size());
        ui->label_size->setText(sizeStr);
    }else {
        ui->label_size->setText("0kb");
    }


    // 获取所有分类列表
    m_categoryList = SqliteObj::getInstance()->getCategoryList();
    // 获取当前Mod的分类列表
    m_classifiedList = SqliteObj::getInstance()->getModCategorys(m_modInfo.id);


    // 添加该Mod未分类信息到下拉框中
    ui->comboBox_categorys->clear();
    foreach (const auto &category, m_categoryList) {
        if(!hasCategory(category.name)){
            ui->comboBox_categorys->addItem(category.name);
        }
    }

    QString styleSheet = ui->centralwidget->styleSheet();
    if(m_modInfo.relative_path == GameManager::ModTrashDir){
        ui->pushButton_move->setText("启用");
        ui->pushButton_move->setIcon(QIcon(":/resources/enable.png"));
        ui->pushButton_move->setStyleSheet("#pushButton_move:hover{background:#28a745;}");
        ui->centralwidget->setStyleSheet("#centralwidget{border: 0px;border-radius: 12px;background-color:#636363;}");
    }else{
        ui->pushButton_move->setText("禁用");
        ui->pushButton_move->setIcon(QIcon(":/resources/disable.png"));
        ui->pushButton_move->setStyleSheet("#pushButton_move:hover{background:#dc3545;}");
        ui->centralwidget->setStyleSheet("#centralwidget{border: 0px;border-radius: 12px;background-color:#395577;}");
    }
}

/**
* @author   XiaoAn
* @brief    设置可选性
* @date     2026-03-02
**/
void ModCard::setCheckable(bool checkable)
{
    m_checkBox->setChecked(false);
    m_checkBox->setCheckable(checkable);
    m_checkBox->setVisible(checkable);
}

/**
* @author   XiaoAn
* @brief    获取可选性
* @date     2026-03-02
**/
bool ModCard::checkable() const
{
    return m_checkBox->isCheckable();
}

/**
* @author   XiaoAn
* @brief    获取选中状态
* @date     2026-03-02
**/
bool ModCard::isChecked() const
{
    return m_checkBox->isChecked();
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
* @brief    备注
* @date     2026-02-27
**/
void ModCard::remark()
{
    QWidget *w = ui->gridLayout->itemAtPosition(0, 0)->widget();

    if(QLabel *label = qobject_cast<QLabel*>(w)){
        QLineEdit *lineEdit = new QLineEdit(this);
        ui->gridLayout->removeWidget(w);
        w->deleteLater();
        ui->gridLayout->addWidget(lineEdit, 0, 0, 1, 2);
        ui->pushButton_remark->setText("确认");
    }else if(QLineEdit *lineEdit = qobject_cast<QLineEdit*>(w)){
        QLabel *label = new QLabel(this);
        label->setStyleSheet("color: rgb(0, 170, 255);font-size: 20px;font-weight: bold;");
        if(!lineEdit->text().isEmpty()){
            label->setText(lineEdit->text());
            // 修改数据库Mod自定义名称
            bool ret = SqliteObj::getInstance()->updateModCustomName(m_modInfo.id, lineEdit->text());
            if(ret){
                m_modInfo.custom_name = lineEdit->text();
            }
        }else if(m_modInfo.custom_name.isEmpty()){
            label->setText(m_modInfo.original_name);
        }else{
            label->setText(m_modInfo.custom_name);
        }
        ui->gridLayout->removeWidget(w);
        w->deleteLater();
        ui->gridLayout->addWidget(label, 0, 0, 1, 2);
        ui->pushButton_remark->setText("备注");
    }

}

/**
* @author   XiaoAn
* @brief    移动
* @date     2026-02-27
**/
void ModCard::transfer()
{
    QString gamePath = GameManager::getInstance()->gamePath();

    QString destRelativePath, btnText;
    if(m_modInfo.relative_path == GameManager::ModTrashDir){
        btnText = "禁用";
        destRelativePath = GameManager::ModLocalDir;
        ui->centralwidget->setStyleSheet("#centralwidget{border: 0px;border-radius: 12px;background-color:#395577;}");
    }else{
        btnText = "启用";
        destRelativePath = GameManager::ModTrashDir;
        ui->centralwidget->setStyleSheet("#centralwidget{border: 0px;border-radius: 12px;background-color:#636363;}");
    }

    QString sourFilePath = QString("%1/%2/%3").arg(gamePath, m_modInfo.relative_path, m_modInfo.original_name);
    QString destFilePath = QString("%1/%2/%3").arg(gamePath, destRelativePath, m_modInfo.original_name);
    QFile::rename(sourFilePath + ".jpg", destFilePath + ".jpg");

    if(QFile::rename(sourFilePath + ".vpk", destFilePath + ".vpk")){
        // 修改记录
        if(SqliteObj::getInstance()->updateModRelativePath(m_modInfo.id, destRelativePath)){
            m_modInfo.relative_path = destRelativePath;
            ui->pushButton_move->setText(btnText);
        }else{
            // 移回回收文件夹
            QFile::rename(destFilePath + ".vpk", sourFilePath + ".vpk");
            QFile::rename(destFilePath + ".vpk", sourFilePath + ".vpk");
            QMessageBox::warning(this, "错误", "数据库修改路径失败，文件已还原位置", QMessageBox::Ok);
        }
    }else{
        QMessageBox::warning(this, "错误", "文件移动失败，可能存在同名文件", QMessageBox::Ok);
    }

    if(m_category.id == -1){
        emit destroyCard(m_modInfo.id);
    }
}

/**
* @author   XiaoAn
* @brief    分类
* @date     2026-02-26
**/
void ModCard::classify()
{
    int categoryIdx = ui->comboBox_categorys->currentIndex();
    int categoryId = m_categoryList.at(categoryIdx).id;
    bool ret = SqliteObj::getInstance()->setModCategory(m_modInfo.id, categoryId);

    if(!ret){
        QMessageBox::warning(this, "错误", "分类设置失败!", QMessageBox::Ok);
    }else {
        QMessageBox::information(this, "提示", "分类设置成功!", QMessageBox::Ok);
        emit classified(m_categoryList.at(categoryIdx).name);
        // 同步删除分类
        m_categoryList.removeAt(categoryIdx);
        ui->comboBox_categorys->removeItem(categoryIdx);
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
        m_button->setText("移除分类");
        m_button->setIcon(QIcon(":/resources/remove.png"));
        m_button->show();
    }else if(m_modInfo.relative_path == GameManager::ModTrashDir){
        m_button->setText("删除模组");
        m_button->setIcon(QIcon(":/resources/delete.png"));
        m_button->show();
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
    m_button->hide();
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
    return QWidget::eventFilter(obj, event);
}

