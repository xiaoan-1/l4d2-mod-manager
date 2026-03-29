#include "moddetailwidget.h"
#include "ui_moddetailwidget.h"

#include "gamemanager.h"

ModDetailWidget::ModDetailWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ModDetailWidget)
{
    ui->setupUi(this);
}

ModDetailWidget::~ModDetailWidget()
{
    delete ui;
}

/**
* @author   XiaoAn
* @brief    设置Mod信息
* @date     2026-03-28
**/
void ModDetailWidget::setModInfo(const ModInfo &modInfo)
{
    m_modInfo = modInfo;

    // 备注名称
    if(m_modInfo.custom_name.isEmpty()){
        ui->label_name->setText(m_modInfo.original_name);
        ui->label_name->setToolTip(m_modInfo.original_name);
    }else{
        ui->label_name->setText(m_modInfo.custom_name);
        ui->label_name->setToolTip(m_modInfo.custom_name);
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
        if(!m_classifiedList.contains(category)){
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
