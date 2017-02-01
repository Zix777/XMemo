#include "XMemo.h"
#include <QApplication>
#include <QCheckBox>
#include <QStringList>
#include <QtAlgorithms>
#include <QToolBar>
#include <QDebug>
#include <QHeaderView>
#include <QPalette>
#include "DbOperator.h"
#include "Settings.h"

XMemo::XMemo(QWidget *parent)
    : QMainWindow(parent)
{    
    memosTableWidget = new QTableWidget(0, 2);
    setCentralWidget(memosTableWidget);
    QStringList tableWidgetHeaders;
    tableWidgetHeaders<<tr("show/hide")<<tr("Memo");
    memosTableWidget->setHorizontalHeaderLabels(tableWidgetHeaders);
    memosTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows); //设置选择行为，以行为单位
    memosTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    init();

    createTrayIcon();

    setWindowTitle("XMemo");
    setWindowIcon(QIcon(":/image/icon.png"));

    memosTableWidget->setFrameShape(QFrame::NoFrame); //设置边框
    //memosTableWidget->setShowGrid(false); //设置不显示格子线
    memosTableWidget->setFocusPolicy(Qt::NoFocus); //去除选中虚线框
    memosTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); //设置不可编辑

    //设置表头字体加粗
    QFont font = memosTableWidget->horizontalHeader()->font();
    font.setBold(true);
    memosTableWidget->horizontalHeader()->setFont(font);
    memosTableWidget->horizontalHeader()->setSectionsClickable(false);
    memosTableWidget->horizontalHeader()->setStretchLastSection(true); //设置充满表宽度
    memosTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    memosTableWidget->verticalHeader()->setVisible(false); //设置垂直头不可见

    memosTableWidget->setAlternatingRowColors(true);
}

XMemo::~XMemo()
{
    // TODO
}

void XMemo::init()
{
    createManagePanel();

    Settings::getInstance().load();
    if(Settings::getInstance().isAutoHide())
        hide();

    DbOperator::getInstance().init();
    DbOperator::getInstance().read(memosList);

    for(const auto i: memosList)
    {
        addMemo(i);

        if(i->isVisible())
        {
            createMemoWidget(i, false);
        }
    }
}

void XMemo::createManagePanel()
{
    quitAction = new QAction(tr("Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    newAction = new QAction(this);
    newAction->setIcon(QIcon(":/image/widget/new.png"));
    connect(newAction, &QAction::triggered, this, &XMemo::onNewMemoTriggered);

    deleteMemosAction = new QAction(this);
    deleteMemosAction->setIcon(QIcon(":/image/widget/delete.png"));
    connect(deleteMemosAction, &QAction::triggered, this, &XMemo::onDeleteMemosTriggered);

    hideMemosAction = new QAction(this);
    hideMemosAction->setIcon(QIcon(":/image/widget/Invisible.png"));
    connect(hideMemosAction, &QAction::triggered, this, &XMemo::onHideMemosTriggered);

    showMemosAction = new QAction(this);
    showMemosAction->setIcon(QIcon(":/image/widget/Visiblet.png"));
    connect(showMemosAction, &QAction::triggered, this, &XMemo::onShowMemosTriggered);

    QToolBar *toolBar = addToolBar(tr("&Tools"));
    toolBar->setMovable(false);
    toolBar->addAction(newAction);
    toolBar->addAction(deleteMemosAction);
    toolBar->addAction(hideMemosAction);
    toolBar->addAction(showMemosAction);
    // TODO
}

void XMemo::deleteMemos(QList<int> selectedRows)
{
    qSort(selectedRows.begin(), selectedRows.end(), [](int a, int b){
        return a > b;
    });
    for(const auto i : selectedRows)
    {
        QObject *checkBoxObj = static_cast<QObject *>(memosTableWidget->cellWidget(i, 0));
        MemoInfo *memoInfo = visibilityCheckBoxHashMap.value(checkBoxObj);
        DbOperator::getInstance().remove(*memoInfo);
        delete memoInfo;

        visibilityCheckBoxHashMap.remove(static_cast<QObject *>(memosTableWidget->cellWidget(i, 0)));
        tableWidgetItemHashMap.remove(memoInfo->getId());
        delete memosTableWidget->cellWidget(i, 0);
        memosTableWidget->removeRow(i);
    }
}

void XMemo::hideMemos(QList<int> selectedRows)
{
    for(const auto i : selectedRows)
    {
        static_cast<QCheckBox *>(memosTableWidget->cellWidget(i, 0))->setChecked(false);
        QObject *checkBoxObj = static_cast<QObject *>(memosTableWidget->cellWidget(i, 0));
        MemoInfo *memoInfo = visibilityCheckBoxHashMap.value(checkBoxObj);
        setMemoVisibility(memoInfo, false);
    }
}

void XMemo::showMemos(QList<int> selectedRows)
{
    for(const auto i : selectedRows)
    {
        static_cast<QCheckBox *>(memosTableWidget->cellWidget(i, 0))->setChecked(true);
        QObject *checkBoxObj = static_cast<QObject *>(memosTableWidget->cellWidget(i, 0));
        MemoInfo *memoInfo = visibilityCheckBoxHashMap.value(checkBoxObj);
        setMemoVisibility(memoInfo, true);
    }
}

void XMemo::setMemoVisibility(MemoInfo *memoInfo, bool visibility)
{
    if(visibility)
    {
        if(memoInfo->getMemoWidget() == nullptr)
            createMemoWidget(memoInfo, false);
    }
    else
    {
        disconnect(memoInfo->getMemoWidget(), &MemoWidget::closeMemo, this, &XMemo::onMemoWidgetClosed);
        memoInfo->removeWidget();
        tableWidgetItemHashMap.value(memoInfo->getId()).checkBox->setChecked(false);
    }
    DbOperator::getInstance().modifyVisibility(*memoInfo);
}

void XMemo::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(QIcon(":/image/icon.png"), this);
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(quitAction);

    trayIcon->show();
    trayIcon->setToolTip(tr("XMemo"));
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, &QSystemTrayIcon::activated, this, &XMemo::onTrayIconClicked);
}

void XMemo::addMemo(MemoInfo *memoInfo)
{
    int count = memosTableWidget->rowCount();
    memosTableWidget->insertRow(count);
    QCheckBox *visibilityCheckBox = new QCheckBox;
    visibilityCheckBox->resize(BUTTON_WIDTH, BUTTON_HEIGHT);
    connect(visibilityCheckBox, &QCheckBox::clicked, this, &XMemo::onVisibilityCheckBoxClicked);
    QString shownContent = memoInfo->getContent().length()>30?memoInfo->getContent().left(30)+"...":memoInfo->getContent();
    QTableWidgetItem *contentTableWidgetItem = new QTableWidgetItem(shownContent);
    memosTableWidget->setItem(count, 1, contentTableWidgetItem);
    memosTableWidget->setCellWidget(count, 0, visibilityCheckBox);

    visibilityCheckBoxHashMap[static_cast<QObject*>(visibilityCheckBox)]=memoInfo;
    visibilityCheckBox->setChecked(memoInfo->isVisible());

    tableWidgetItemHashMap[memoInfo->getId()] = TableWidgetItemInfo{contentTableWidgetItem, visibilityCheckBox};
}

void XMemo::createMemoWidget(MemoInfo *memoInfo, bool isEditMode)
{
    memoInfo->createWidget(isEditMode);
    connect(memoInfo->getMemoWidget(), &MemoWidget::closeMemo, this, &XMemo::onMemoWidgetClosed);
    connect(memoInfo, &MemoInfo::memoChanged, this, &XMemo::updateMemo);
    connect(memoInfo->getMemoWidget(), &MemoWidget::createMemo, this, &XMemo::onNewMemoTriggered);
}

void XMemo::closeEvent(QCloseEvent *e)
{
    if(trayIcon->isVisible())
    {
        hide();
        e->ignore();
    }
}

QList<int> XMemo::getSelectedRows()
{
    QList<QTableWidgetItem *> selectedItems = memosTableWidget->selectedItems();
    QList<int> selectedRows;
    for(const auto &i : selectedItems)
    {
        int itemRow = memosTableWidget->row(i);
        selectedRows.push_back(itemRow);
    }
    return selectedRows;
}

void XMemo::onTrayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    //托盘点击方式
    switch(reason)
    {
        //单击
        case QSystemTrayIcon::Trigger:
            //双击
        case QSystemTrayIcon::DoubleClick:
            //恢复窗口显示
            this->setWindowState(Qt::WindowActive);
            this->show();
            break;
        default:
            break;
    }
}

void XMemo::onNewMemoTriggered()
{
    MemoInfo *memoInfo = new MemoInfo();
    createMemoWidget(memoInfo, true);
    memosList.push_back(memoInfo);
    DbOperator::getInstance().add(*memoInfo);

    addMemo(memoInfo);
}

void XMemo::onDeleteMemosTriggered()
{
    deleteMemos(getSelectedRows());
}

void XMemo::onHideMemosTriggered()
{
    hideMemos(getSelectedRows());
}

void XMemo::onShowMemosTriggered()
{
    showMemos(getSelectedRows());
}

void XMemo::updateMemo(uint id)
{
    MemoInfo *memoInfo = static_cast<MemoInfo *>(sender());
    tableWidgetItemHashMap.value(id).tableWidgetItem->setText(memoInfo->getContent());
}

void XMemo::onVisibilityCheckBoxClicked(bool checked)
{
    MemoInfo *memoInfo = visibilityCheckBoxHashMap.value(sender());
    setMemoVisibility(memoInfo, checked);
}

void XMemo::onMemoWidgetClosed(MemoInfo *memoInfo)
{
    disconnect(memoInfo->getMemoWidget(), &MemoWidget::closeMemo, this, &XMemo::onMemoWidgetClosed);
    memoInfo->removeWidget();
    tableWidgetItemHashMap.value(memoInfo->getId()).checkBox->setChecked(false);
}