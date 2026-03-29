#include "checkboxlistwidget.h"
#include "ui_checkboxlistwidget.h"



CheckBoxListWidget::CheckBoxListWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CheckBoxListWidget)
{
    ui->setupUi(this);

    hide();
    setWindowFlags(Qt::Popup);

    connect(ui->pushButton_selectAll, &QPushButton::clicked, this, [=](){
        resetOptionsChecked(true);
    });

    connect(ui->pushButton_cancelAll, &QPushButton::clicked, this, [=](){
        resetOptionsChecked(false);
    });
}

CheckBoxListWidget::~CheckBoxListWidget()
{
    delete ui;
}

/**
* @author   XiaoAn
* @brief    添加单个选项
* @date     2026-03-04
**/
void CheckBoxListWidget::addOption(const QString &option)
{
    QVBoxLayout *vboxLayout = qobject_cast<QVBoxLayout*>(ui->scrollWidget->layout());

    QCheckBox *checkBox = new QCheckBox(option);
    checkBox->setChecked(true);
    connect(checkBox, &QCheckBox::checkStateChanged, this, [=](bool checked){
        emit optionCheckStateChanged(checkBox->text(), checked);
    });

    m_checkBoxList.append(checkBox);

    vboxLayout->insertWidget(vboxLayout->count() - 1, checkBox);
}

/**
* @author   XiaoAn
* @brief    添加多个选项
* @date     2026-03-04
**/
void CheckBoxListWidget::addOptions(const QStringList &options)
{
    foreach (const QString &option, options) {
        addOption(option);
    }
}

/**
* @author   XiaoAn
* @brief    移除选项
* @date     2026-03-04
**/
void CheckBoxListWidget::removeOption(const QString &option)
{
    foreach (QCheckBox *checkbox, m_checkBoxList) {
        if(checkbox->text()== option){
            checkbox->deleteLater();
            m_checkBoxList.removeOne(checkbox);
            break;
        }
    }
}

/**
* @author   XiaoAn
* @brief    重命名选项
* @date     2026-03-08
**/
void CheckBoxListWidget::renameOption(const QString &oldName, const QString &newName)
{
    foreach (QCheckBox *checkbox, m_checkBoxList) {
        if(checkbox->text()== oldName){
            checkbox->setText(newName);
            break;
        }
    }
}

/**
* @author   XiaoAn
* @brief    设置选项的选中状态
* @date     2026-03-04
**/
void CheckBoxListWidget::setOptionChecked(const QString &option, const bool &checked)
{
    foreach (QCheckBox *checkbox, m_checkBoxList) {
        if(checkbox->text()== option){
            checkbox->setChecked(checked);
            break;
        }
    }
}

/**
* @author   XiaoAn
* @brief    初始化选中状态
* @date     2026-03-04
**/
void CheckBoxListWidget::resetOptionsChecked(const bool &checked)
{
    foreach (QCheckBox *checkbox, m_checkBoxList) {
        checkbox->setChecked(checked);
    }
}
