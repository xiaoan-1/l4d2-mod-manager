#include "checkboxlistwidget.h"
#include "ui_checkboxlistwidget.h"



CheckBoxListWidget::CheckBoxListWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CheckBoxListWidget)
{
    ui->setupUi(this);

    hide();
    setWindowFlags(Qt::Popup);

    connect(ui->pushButton_reset, &QPushButton::clicked, this, [=](){
        resetOptionsChecked(true);
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

    // 创建容器
    QWidget *w = new QWidget();
    QHBoxLayout *hLayout = new QHBoxLayout();
    w->setProperty("option", option);
    m_widgetList.append(w);
    w->setLayout(hLayout);

    QLabel *label = new QLabel(option);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    QCheckBox *checkBox = new QCheckBox();
    checkBox->setChecked(true);
    connect(checkBox, &QCheckBox::checkStateChanged, this, [=](bool checked){
        optionCheckStateChanged(option, checked);
    });

    m_options.insert(option, checkBox);

    hLayout->addWidget(label);
    hLayout->addWidget(checkBox);

    vboxLayout->insertWidget(vboxLayout->count() - 1, w);
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
    foreach (QWidget *w, m_widgetList) {
        if(w->property("option") == option){
            w->deleteLater();
            m_widgetList.removeAll(w);
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
    for (auto it = m_options.begin(); it != m_options.end(); ++it) {
        if(it.key() == option){
            it.value()->setChecked(checked);
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
    for (auto it = m_options.begin(); it != m_options.end(); ++it) {
        it.value()->setChecked(checked);
    }
}
