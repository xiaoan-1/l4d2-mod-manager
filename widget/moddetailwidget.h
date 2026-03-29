#ifndef MODDETAILWIDGET_H
#define MODDETAILWIDGET_H

#include <QWidget>

#include "sqliteobj.h"

namespace Ui {
class ModDetailWidget;
}

class ModDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ModDetailWidget(QWidget *parent = nullptr);
    ~ModDetailWidget();

    void setModInfo(const ModInfo &modInfo);

private:
    Ui::ModDetailWidget *ui;

    ModInfo m_modInfo;

    // 所有分类信息
    QList<CategoryInfo> m_categoryList;

    // 已分类的分类信息
    QList<CategoryInfo> m_classifiedList;
};

#endif // MODDETAILWIDGET_H
