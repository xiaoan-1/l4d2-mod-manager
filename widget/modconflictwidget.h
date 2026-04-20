#ifndef MODCONFLICTWIDGET_H
#define MODCONFLICTWIDGET_H

#include <QWidget>
#include <QLayout>
#include <QPushButton>

#include "header.h"
#include "../comp/modcard.h"
#include "../utils/vpkfileparser.h"

namespace Ui {
class ModConflictWidget;
}

class ModConflictWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ModConflictWidget(QWidget *parent = nullptr);
    ~ModConflictWidget();

    // 清除冲突的模组列表
    void clearConflictMod();

    // 检测冲突
    void detectConflictMod();

    void detectConflictMod(const QList<ModInfo> &modInfoList);


    QList<ModInfo> conflictModList();

private:
    void appendModCard(const ModInfo &modInfo);

    void removeConflictMod(const ModInfo &modInfo);

private:
    Ui::ModConflictWidget *ui;

    QVBoxLayout* m_layout;

    QMap<ModInfo, QList<ModInfo>> m_conflictModMap;

    QList<ModCard*> m_cardList;

    ModCard *m_currentCard = nullptr;
};

#endif // MODCONFLICTWIDGET_H
