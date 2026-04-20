#include "modconflictwidget.h"
#include "ui_modconflictwidget.h"

#include "gamemanager.h"
#include "../utils/imageloader.h"

ModConflictWidget::ModConflictWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ModConflictWidget)
{
    ui->setupUi(this);

    hide();

    m_layout = qobject_cast<QVBoxLayout*>(ui->scrollWidget_left->layout());

    ui->widget_container->setCardSizeMode(ModCard::SizeMode::Conflict);

    connect(ui->pushButton_close, &QPushButton::clicked, this, &ModConflictWidget::deleteLater);


    // 当布局发生变化时，检查剩余的卡片
    // connect(ui->widget_container, &CardContainer::updatedLayout, this, [=](){
    //     QList<ModCard *> modCardList = ui->widget_container->modCardList();

    //     if(!modCardList.isEmpty() || !m_currentCard) return;

    //     // 全部被禁用，则删除当前选中模组卡片
    //     m_currentCard->deleteLater();
    //     m_cardList.removeAll(m_currentCard);
    //     if(!m_cardList.isEmpty()){
    //         emit m_cardList.first()->clicked();
    //     }
    // });
}

ModConflictWidget::~ModConflictWidget()
{
    delete ui;
}

/**
* @author   XiaoAn
* @brief    清空冲突模组
* @date     2026-04-17
**/
void ModConflictWidget::clearConflictMod()
{
    // 清空
    foreach (ModCard *modCard, m_cardList) {
        modCard->deleteLater();
    }
    m_cardList.clear();

    m_conflictModMap.clear();

    ui->widget_container->clearModCard();
}

/**
* @author   XiaoAn
* @brief    检测全部的冲突模组
* @date     2026-04-18
**/
void ModConflictWidget::detectConflictMod()
{
    QString gamePath = GameManager::getInstance()->gamePath();
    QList<ModInfo> allModInfoList = SqliteObj::getInstance()->getModInfoList(GameManager::ModLocalDir);
    // 解析所有vpk文件
    QList<VpkFileParser> vpkFileList;
    foreach (const ModInfo &modInfo, allModInfoList) {
        QString filePath = QString("%1/%2/%3.vpk").arg(gamePath, modInfo.relative_path, modInfo.original_name);
        vpkFileList.append(VpkFileParser(filePath));
    }

    QList<QPair<int, int>> conflictPairs = VpkFileParser::detectConflicts(vpkFileList);
    foreach (const auto &pair, conflictPairs) {
        ModInfo firModInfo = allModInfoList[pair.first];
        ModInfo secModInfo = allModInfoList[pair.second];
        if(m_conflictModMap.contains(firModInfo)){
            m_conflictModMap[firModInfo].append(secModInfo);
        }else{
            m_conflictModMap.insert(firModInfo, QList<ModInfo>() << secModInfo);
        }
    }

    for(auto it = m_conflictModMap.begin(); it != m_conflictModMap.end(); ++it)
    {
        appendModCard(it.key());
    }

    if(!m_cardList.isEmpty()){
        emit m_cardList.first()->clicked();
    }
}

/**
* @author   XiaoAn
* @brief    检测指定模组与其他模组的冲突
* @date     2026-04-18
**/
void ModConflictWidget::detectConflictMod(const QList<ModInfo> &modInfoList)
{
    QString gamePath = GameManager::getInstance()->gamePath();
    QList<ModInfo> allModInfoList = SqliteObj::getInstance()->getModInfoList(GameManager::ModLocalDir);

    // 移除重复的模组
    foreach (const ModInfo &modInfo, modInfoList) {
        allModInfoList.removeAll(modInfo);
    }

    // 解析所有vpk文件
    QList<VpkFileParser> vpkFileList;
    foreach (const ModInfo &modInfo, allModInfoList) {
        if(modInfoList.contains(modInfo)) continue;
        QString filePath = QString("%1/%2/%3.vpk").arg(gamePath, modInfo.relative_path, modInfo.original_name);
        vpkFileList.append(VpkFileParser(filePath));
    }

    foreach (const ModInfo &modInfo, modInfoList) {
        QString filePath = QString("%1/%2/%3.vpk").arg(gamePath, modInfo.relative_path, modInfo.original_name);
        VpkFileParser vpkFile(filePath);
        // 检测单个模组与其他模组的冲突
        QList<int> conflicts = VpkFileParser::detectConflicts(vpkFile, vpkFileList);
        if(conflicts.isEmpty()) continue;

        QList<ModInfo> conflictModInfoList;
        foreach (int idx, conflicts) {
            conflictModInfoList.append(allModInfoList[idx]);
        }
        m_conflictModMap.insert(modInfo, conflictModInfoList);
    }
    for(auto it = m_conflictModMap.begin(); it != m_conflictModMap.end(); ++it)
    {
        appendModCard(it.key());
    }

    if(!m_cardList.isEmpty()){
        emit m_cardList.first()->clicked();
    }
}

/**
* @author   XiaoAn
* @brief
* @date     2026-04-18
**/
QList<ModInfo> ModConflictWidget::conflictModList()
{
    return m_conflictModMap.keys();
}

/**
* @author   XiaoAn
* @brief    添加模组到左侧列表
* @date     2026-04-18
**/
void ModConflictWidget::appendModCard(const ModInfo &modInfo)
{
    // 左侧冲突模组
    ModCard *modCard = new ModCard(modInfo, this, ModCard::SizeMode::Conflict);
    modCard->setFixedSize(220, 200);
    m_layout->insertWidget(m_layout->count() - 1, modCard);
    m_cardList.append(modCard);

    // 点击后在右侧展示多个冲突模组
    connect(modCard, &ModCard::clicked, this, [=](){
        m_currentCard = modCard;
        ui->widget_container->clearModCard();
        ui->widget_container->appendModCard(m_conflictModMap.value(modInfo));

        // 右侧模组被禁用后，删除左侧相同模组
        QList<ModCard *> modCardList = ui->widget_container->modCardList();
        foreach (ModCard *modCardR, modCardList) {
            connect(modCardR, &ModCard::toggleDisabled, this, [=](bool isDisabled){
                if(!isDisabled) return;
                removeConflictMod(modCardR->modInfo());
            });
        }
    });

    // 销毁模组卡片
    connect(modCard, &ModCard::toggleDisabled, this, [=](bool isDisabled){
        if(!isDisabled) return;
        removeConflictMod(modCard->modInfo());
    });

    // 当模组卡片大小变化时提交图像加载任务
    connect(modCard, &ModCard::imgResize, this, [=](){
        ImageLoader::Task task;
        task.isCover = true;
        task.id = modInfo.id;
        task.imagePath = QString("%1/%2/%3.jpg").arg(
            GameManager::getInstance()->gamePath(), modInfo.relative_path , modInfo.original_name);
        task.targetSize = modCard->getImageSize();
        task.modCardPtr = modCard;
        ImageLoader::getInstance()->addTask(task);
    });
}

/**
* @author   XiaoAn
* @brief    移除冲突的模组
* @date     2026-04-20
**/
void ModConflictWidget::removeConflictMod(const ModInfo &modInfo)
{
    // 移除模组信息映射的所有该模组信息
    QMutableMapIterator<ModInfo, QList<ModInfo>> it(m_conflictModMap);
    while (it.hasNext()) {
        it.next();

        // 从当前列表中移除所有冲突模组
        it.value().removeAll(modInfo);

        // 如果列表变空 或者 键等于 n，则删除整个条目
        if (it.value().isEmpty() || it.key() == modInfo) {
            it.remove();
        }
    }

    // 移除模组卡片控件
    QList<ModCard *> modCardList = ui->widget_container->modCardList();

    if(modCardList.isEmpty() && m_currentCard){
        m_currentCard->deleteLater();
        m_currentCard = nullptr;
    }

    foreach (ModCard *modCard, m_cardList) {
        if(modCard->modInfo().id != modInfo.id) continue;

        if(modCard == m_currentCard){
            m_currentCard = nullptr;
        }

        modCard->deleteLater();
        m_cardList.removeAll(modCard);
        // 只可能存在一个
        break;
    }

    // 初始化选中
    if(m_currentCard == nullptr && !m_cardList.isEmpty()){
        m_currentCard = m_cardList.first();
        emit m_currentCard->clicked();
    }
}


