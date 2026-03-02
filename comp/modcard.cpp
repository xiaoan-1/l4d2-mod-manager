#include "modcard.h"
#include "ui_modcard.h"

#include <QImage>
#include <QBuffer>
#include <QImageReader>
#include <QImageIOHandler>
#include <QMessageBox>
#include <QLineEdit>

#include "../modmanager.h"

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

    // 可选性
    m_checkBox = new QCheckBox(this);
    m_checkBox->move(0, 0);
    m_checkBox->hide();
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

    // 可移除分类
    QPushButton *categoryDelBtn = new QPushButton("移除分类",this);
    categoryDelBtn->move(width() - categoryDelBtn->width(), 0);
    connect(categoryDelBtn, &QPushButton::clicked, this, [=](){
        bool ret = SqliteObj::getInstance()->removeModCategory(m_modInfo.id, category.id);
        if(ret){
            // 由父容器来销毁
            emit destroyCard(m_modInfo.id);
        }else{
            QMessageBox::warning(this, "错误", "移除分类失败！", QMessageBox::Ok);
        }
    });
    categoryDelBtn->show();
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

    QString baseDir = ModManager::getInstance()->gamePath() + m_modInfo.relative_path;

    // 文件大小
    QFile modFile(baseDir + "/" + m_modInfo.original_name + ".vpk");
    if(modFile.exists()){
        QString sizeStr = ModManager::getFileSizeWithUnit(modFile.size());
        ui->label_size->setText(sizeStr);
    }else {
        ui->label_size->setText("0kb");
    }


    // 获取所有分类列表
    m_categoryList = SqliteObj::getInstance()->getCategoryList();
    // 获取当前Mod的分类列表
    QList<CategoryInfo> modCategoryList = SqliteObj::getInstance()->getModCategorys(m_modInfo.id);
    foreach (const auto &modCategory, modCategoryList) {
        foreach (const auto &category, m_categoryList) {
            if(category == modCategory){
                m_categoryList.removeOne(category);
                break;
            }
        }
    }

    // 添加到下拉框中
    ui->comboBox_categorys->clear();
    foreach (const auto &category, m_categoryList) {
        ui->comboBox_categorys->addItem(category.name);
    }

    if(m_modInfo.relative_path == ModManager::ModTrashDir){
        ui->pushButton_move->setText("启用");
    }else{
        ui->pushButton_move->setText("禁用");
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
    QString gamePath = ModManager::getInstance()->gamePath();
    if(m_modInfo.relative_path == ModManager::ModTrashDir){
        // 移回本地
        QFile modFile(gamePath + m_modInfo.relative_path + "/" + m_modInfo.original_name + ".vpk");
        modFile.rename(gamePath + ModManager::ModLocalDir + "/" + m_modInfo.original_name + ".vpk");

        QFile imgFile(gamePath + m_modInfo.relative_path + "/" + m_modInfo.original_name + ".jpg");
        imgFile.rename(gamePath + ModManager::ModLocalDir + "/" + m_modInfo.original_name + ".jpg");

        // 修改记录
        SqliteObj::getInstance()->updateModRelativePath(m_modInfo.id, ModManager::ModLocalDir);
        ui->pushButton_move->setText("禁用");
    }else{
        // 移到回收站
        QFile modFile(gamePath + m_modInfo.relative_path + "/" + m_modInfo.original_name + ".vpk");
        modFile.rename(gamePath + ModManager::ModTrashDir + "/" + m_modInfo.original_name + ".vpk");

        QFile imgFile(gamePath + m_modInfo.relative_path + "/" + m_modInfo.original_name + ".jpg");
        imgFile.rename(gamePath + ModManager::ModTrashDir + "/" + m_modInfo.original_name + ".jpg");


        SqliteObj::getInstance()->updateModRelativePath(m_modInfo.id, ModManager::ModTrashDir);
        ui->pushButton_move->setText("启用");
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
        // 同步删除分类
        m_categoryList.removeAt(categoryIdx);
        ui->comboBox_categorys->removeItem(categoryIdx);
    }
}

/**
* @author   XiaoAn
* @brief    隐藏信号
* @date     2026-03-01
**/
void ModCard::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    emit visiableChanged(false);
}

/**
* @author   XiaoAn
* @brief    显示信号
* @date     2026-03-01
**/
void ModCard::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    emit visiableChanged(true);
}

