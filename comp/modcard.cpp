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
}

ModCard::~ModCard()
{
    delete ui;
}

ModCard::ModCard(const ModInfo &modInfo, QWidget *parent)
    : ModCard(parent)
{
    m_modInfo = modInfo;

    // 备注名称
    if(modInfo.custom_name.isEmpty()){
        ui->label_name->setText(modInfo.original_name);
    }else{
        ui->label_name->setText(modInfo.custom_name);
    }

    QString baseDir = ModManager::getInstance()->gamePath() + modInfo.relative_path;

    // 加载图片
    loadImage(baseDir + "/" + modInfo.original_name + ".jpg");

    // 文件大小
    QFile modFile(baseDir + "/" + modInfo.original_name + ".vpk");
    if(modFile.exists()){
        QString sizeStr = ModManager::getFileSizeWithUnit(modFile.size());
        ui->label_size->setText(sizeStr);
    }else {
        ui->label_size->setText("0kb");
    }


    // 获取所有分类列表
    m_categoryList = SqliteObj::getInstance()->getCategoryList();
    // 获取当前Mod的分类列表
    QList<CategoryInfo> modCategoryList = SqliteObj::getInstance()->getModCategorys(modInfo.id);
    foreach (const auto &modCategory, modCategoryList) {
        foreach (const auto &category, m_categoryList) {
            if(category == modCategory){
                m_categoryList.removeOne(category);
                break;
            }
        }
    }

    // 添加到下拉框中
    foreach (const auto &category, m_categoryList) {
        ui->comboBox_categorys->addItem(category.name);
    }

    // 备注信息
    connect(ui->pushButton_remark, &QPushButton::clicked, this, &ModCard::remarkMod);


    if(modInfo.relative_path == ModManager::ModTrashDir){
        ui->pushButton_move->setText("启用");
    }else{
        ui->pushButton_move->setText("禁用");
    }
    // 禁用
    connect(ui->pushButton_move, &QPushButton::clicked, this, &ModCard::moveMod);

    // 确认设置分类
    connect(ui->pushButton_affirm, &QPushButton::clicked, this, &ModCard::classifyMod);

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
    QPushButton *categoryDelBtn = new QPushButton("移除分类",this);

    connect(categoryDelBtn, &QPushButton::clicked, this, [=](){
        bool ret = SqliteObj::getInstance()->removeModCategory(m_modInfo.id, category.id);
        if(ret){
            this->deleteLater();
        }else{
            QMessageBox::warning(this, "错误", "移除分类失败！", QMessageBox::Ok);
        }
    });
}

/**
* @author   XiaoAn
* @brief    加载Mod图片
* @date     2026-02-25
**/
void ModCard::loadImage(const QString &imgPath)
{

    QFile imgFile(imgPath);
    if(!imgFile.exists(imgPath)){
        qWarning() << "文件不存在" << imgPath;
        return;
    }

    QPixmap pixmap(imgPath);;

    // 缩放图片到QLabel大小（保持宽高比，不拉伸变形）
    QPixmap scaledPixmap = pixmap.scaled(
        ui->label_img->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    if (scaledPixmap.isNull()) {
        qWarning() << "图片加载失败：" << imgPath;
        return;
    }

    ui->label_img->setPixmap(scaledPixmap);
    ui->label_img->setAlignment(Qt::AlignCenter);
}

/**
* @author   XiaoAn
* @brief    给Mod备注
* @date     2026-02-27
**/
void ModCard::remarkMod()
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
* @brief    移动Mod
* @date     2026-02-27
**/
void ModCard::moveMod()
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
        deleteLater();
    }
}

/**
* @author   XiaoAn
* @brief    对该Mod进行分类
* @date     2026-02-26
**/
void ModCard::classifyMod()
{
    int categoryIdx = ui->comboBox_categorys->currentIndex();

    bool ret = SqliteObj::getInstance()->setModCategory(m_modInfo.id, m_categoryList.at(categoryIdx).id);

    if(!ret){
        QMessageBox::warning(this, "错误", "分类设置失败!", QMessageBox::Ok);
    }else {
        QMessageBox::information(this, "提示", "分类设置成功!", QMessageBox::Ok);
        ui->comboBox_categorys->removeItem(categoryIdx);
    }
}

