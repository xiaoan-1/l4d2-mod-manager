#include "categorybutton.h"
#include "ui_categorybutton.h"

#include <QDrag>
#include <QMimeData>
#include <QMessageBox>

CategoryButton::CategoryButton(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CategoryButton)
{
    ui->setupUi(this);

    // 菜单
    m_menu = new QMenu(this);
    m_menu->setWindowFlags(Qt::Popup);
    m_menu->addAction("重命名", this, &CategoryButton::renameCategory);
    m_menu->addAction("删除分类", this, &CategoryButton::deleteCategory);

    // 弹出菜单
    connect(ui->pushButton_menu, &QPushButton::clicked, this, [=](){
        // 获取按钮在屏幕上的绝对位置
        QPoint globalBtnPos = ui->pushButton_menu->mapToGlobal(QPoint(0, 0));
        m_menu->move(globalBtnPos.x(), globalBtnPos.y() + ui->pushButton_menu->height());
        m_menu->show();
    });

    connect(ui->pushButton_name, &QPushButton::clicked, this, &CategoryButton::clicked);
}

CategoryButton::~CategoryButton()
{
    delete ui;
}

CategoryButton::CategoryButton(const CategoryInfo &category, QWidget *parent)
    : CategoryButton(parent)
{
    m_category = category;
    ui->pushButton_name->setText(category.name);
}

/**
* @author   XiaoAn
* @brief    核心按钮
* @date     2026-03-08
**/
QPushButton *CategoryButton::coreButton() const
{
    return ui->pushButton_name;
}

/**
* @author   XiaoAn
* @brief    设置分类排序序号
* @date     2026-04-08
**/
void CategoryButton::setCategorySort(int sort)
{
    bool ret = SqliteObj::getInstance()->updateCategorySort(m_category.id, sort);
    if(ret){
        m_category.sort = sort;
    }
}

/**
* @author   XiaoAn
* @brief    重命名分类
* @date     2026-03-08
**/
void CategoryButton::renameCategory()
{
    if(m_lineEdit){
        return;
    }
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setGeometry(ui->pushButton_name->geometry());
    m_lineEdit->setAlignment(Qt::AlignCenter);
    m_lineEdit->setText(ui->pushButton_name->text());
    m_lineEdit->raise();
    m_lineEdit->setFocus();
    m_lineEdit->show();

    connect(m_lineEdit, &QLineEdit::editingFinished, this, [=](){
        QString newName = m_lineEdit->text();
        if(SqliteObj::getInstance()->updateCategoryName(m_category.id, newName)){
            emit renamed(ui->pushButton_name->text(), newName);
            ui->pushButton_name->setText(newName);
        }else{
            QMessageBox::warning(this, "错误", "修改失败!", QMessageBox::Ok);
        }
        m_lineEdit->deleteLater();
        m_lineEdit = nullptr;
    });

}

/**
* @author   XiaoAn
* @brief    删除分类
* @date     2026-03-08
**/
void CategoryButton::deleteCategory()
{
    QList<ModInfo> modInfoList = SqliteObj::getInstance()->getModInfoList(m_category.id);

    if(!modInfoList.isEmpty()){
        int opt = QMessageBox::question(this, "确认删除", "该分类存在Mod文件信息，是否清空!", QMessageBox::Ok | QMessageBox::Cancel);
        if(opt == QMessageBox::Cancel){
            return;
        }
    }

    // 删除分类
    bool ret = SqliteObj::getInstance()->removeCategory(m_category.id);
    if(ret){
        emit deleted();
    }else{
        QMessageBox::warning(this, "错误", "删除失败!", QMessageBox::Ok);
    }
}

/**
* @author   XiaoAn
* @brief    鼠标移动启动拖拽效果
* @date     2026-04-08
**/
void CategoryButton::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) return;

    // 计算移动距离，超过阈值才启动拖拽
    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    // 1. 创建数据容器
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("custom/type", QByteArray::number((qlonglong)this));

    // 2. 创建拖拽对象
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // 3. 设置视觉反馈
    QPixmap pixmap = grab();
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

    // 4. 执行拖拽效果
    drag->exec(Qt::MoveAction | Qt::CopyAction);
}


