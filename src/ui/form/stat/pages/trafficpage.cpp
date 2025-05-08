#include "trafficpage.h"

#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QTabBar>
#include <QTimeEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include <appinfo/appinfocache.h>
#include <conf/confmanager.h>
#include <form/controls/appinforow.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/stat/statisticscontroller.h>
#include <form/stat/statisticswindow.h>
#include <manager/windowmanager.h>
#include <model/appstatmodel.h>
#include <model/traflistmodel.h>
#include <user/iniuser.h>
#include <util/iconcache.h>

namespace {

constexpr int APP_LIST_HEADER_VERSION = 1;

}

TrafficPage::TrafficPage(StatisticsController *ctrl, QWidget *parent) :
    StatBasePage(ctrl, parent),
    m_appStatModel(new AppStatModel(this)),
    m_trafListModel(new TrafListModel(this))
{
    setupUi();

    appStatModel()->initialize();
    trafListModel()->initialize();
}

AppInfoCache *TrafficPage::appInfoCache() const
{
    return appStatModel()->appInfoCache();
}

void TrafficPage::onSaveWindowState(IniUser *ini)
{
    auto header = m_appListView->horizontalHeader();
    ini->setStatAppListHeader(header->saveState());
    ini->setStatAppListHeaderVersion(APP_LIST_HEADER_VERSION);

    ini->setStatWindowTrafSplit(m_splitter->saveState());
}

void TrafficPage::onRestoreWindowState(IniUser *ini)
{
    if (ini->statAppListHeaderVersion() == APP_LIST_HEADER_VERSION) {
        auto header = m_appListView->horizontalHeader();
        header->restoreState(ini->statAppListHeader());
    }

    m_splitter->restoreState(ini->statWindowTrafSplit());
}

void TrafficPage::onRetranslateUi()
{
    m_btEdit->setText(tr("Edit"));
    m_actAddProgram->setText(tr("Add Program"));
    m_actRemoveApp->setText(tr("Remove Application"));
    m_actResetTotal->setText(tr("Reset Total"));
    m_actClearAll->setText(tr("Clear All"));

    m_btRefresh->setText(tr("Refresh"));

    m_traphUnits->setText(tr("Units:"));
    retranslateTrafUnitNames();

    retranslateTabBar();

    m_appInfoRow->retranslateUi();
}

void TrafficPage::retranslateTrafUnitNames()
{
    const QStringList list = { tr("Adaptive"), tr("Bytes"), "KB", "MB", "GB", "TB", "PB" };

    ControlUtil::setComboBoxTexts(m_comboTrafUnit, list);

    updateTrafUnit();
    updateTableTrafUnit();
}

void TrafficPage::retranslateTabBar()
{
    const QStringList list = { tr("Hourly"), tr("Daily"), tr("Monthly"), tr("Total") };

    int index = 0;
    for (const auto &v : list) {
        m_tabBar->setTabText(index++, v);
    }
}

void TrafficPage::setupUi()
{
    // Header
    auto header = setupHeader();

    // App List
    setupAppListView();
    setupAppListHeader();

    // Tab Bar
    setupTabBar();

    auto trafLayout = ControlUtil::createVLayout();
    trafLayout->addWidget(m_tabBar);

    // Traf Table
    setupTableTraf();
    setupTableTrafHeader();
    trafLayout->addWidget(m_tableTraf);

    auto trafWidget = new QWidget();
    trafWidget->setLayout(trafLayout);

    m_splitter = new QSplitter();
    m_splitter->addWidget(m_appListView);
    m_splitter->addWidget(trafWidget);

    // App Info Row
    setupAppInfoRow();

    // Actions on app list view's current changed
    setupAppListViewChanged();

    // Layout
    auto layout = new QVBoxLayout();
    layout->addLayout(header);
    layout->addWidget(m_splitter, 1);
    layout->addWidget(m_appInfoRow);

    this->setLayout(layout);
}

QLayout *TrafficPage::setupHeader()
{
    setupClearMenu();
    setupRefresh();
    setupTrafUnits();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btEdit, ControlUtil::createVSeparator(), m_btRefresh,
                    /*stretch*/ nullptr, m_traphUnits, m_comboTrafUnit });

    return layout;
}

void TrafficPage::setupClearMenu()
{
    auto menu = ControlUtil::createMenu(this);

    m_actAddProgram = menu->addAction(IconCache::icon(":/icons/application.png"), QString());
    m_actAddProgram->setShortcut(Qt::Key_Insert);

    m_actRemoveApp = menu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveApp->setShortcut(Qt::Key_Delete);

    m_actResetTotal = menu->addAction(QString());
    m_actClearAll = menu->addAction(IconCache::icon(":/icons/broom.png"), QString());

    connect(m_actAddProgram, &QAction::triggered, this, [&] {
        const auto appPath = appListCurrentPath();
        if (!appPath.isEmpty()) {
            windowManager()->openProgramEditForm(appPath, windowManager()->statWindow());
        }
    });
    connect(m_actRemoveApp, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox([&] { appStatModel()->remove(appListCurrentIndex()); },
                tr("Are you sure to remove statistics for selected application?"));
    });
    connect(m_actResetTotal, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] { ctrl()->resetAppTotals(); }, tr("Are you sure to reset total statistics?"));
    });
    connect(m_actClearAll, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] {
                    m_appListView->clearSelection();
                    ctrl()->clearTraffic();
                },
                tr("Are you sure to clear all statistics?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(menu);
}

void TrafficPage::setupRefresh()
{
    m_btRefresh = ControlUtil::createFlatToolButton(
            ":/icons/arrow_refresh_small.png", [&] { trafListModel()->reset(); });
}

void TrafficPage::setupTrafUnits()
{
    m_traphUnits = ControlUtil::createLabel();

    m_comboTrafUnit = ControlUtil::createComboBox(QStringList(), [&](int index) {
        if (iniUser()->statTrafUnit() == index)
            return;

        iniUser()->setStatTrafUnit(index);
        updateTableTrafUnit();

        confManager()->saveIniUser();
    });
    m_comboTrafUnit->setMinimumWidth(100);
}

void TrafficPage::setupAppListView()
{
    m_appListView = new TableView();
    m_appListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_appListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_appListView->setSortingEnabled(true);
    m_appListView->setModel(appStatModel());

    m_appListView->setMenu(m_btEdit->menu());

    connect(m_appListView, &TableView::doubleClicked, m_actAddProgram, &QAction::trigger);
}

void TrafficPage::setupAppListHeader()
{
    auto header = m_appListView->horizontalHeader();

    header->setSectionResizeMode(int(AppStatColumn::Program), QHeaderView::Interactive);
    header->setSectionResizeMode(int(AppStatColumn::Download), QHeaderView::Stretch);
    header->setSectionResizeMode(int(AppStatColumn::Upload), QHeaderView::Stretch);

    header->resizeSection(int(AppStatColumn::Program), 240);

    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(int(AppStatColumn::Program), Qt::AscendingOrder);
}

void TrafficPage::setupTabBar()
{
    m_tabBar = new QTabBar();
    m_tabBar->setShape(QTabBar::RoundedNorth);

    for (int n = 4; --n >= 0;) {
        m_tabBar->addTab(QString());
    }
}

void TrafficPage::setupTableTraf()
{
    m_tableTraf = new TableView();
    m_tableTraf->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableTraf->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_tableTraf->setModel(trafListModel());

    const auto resetTableTraf = [&] {
        const auto trafType = TrafListModel::TrafType(m_tabBar->currentIndex());

        trafListModel()->setType(trafType);

        const auto &appStatRow = appStatModel()->appStatRowAt(appListCurrentIndex());
        trafListModel()->setAppId(appStatRow.appId);

        trafListModel()->resetTraf();
    };

    resetTableTraf();

    connect(m_tabBar, &QTabBar::currentChanged, this, resetTableTraf);
    connect(m_appListView, &TableView::currentIndexChanged, this, resetTableTraf);
}

void TrafficPage::setupTableTrafHeader()
{
    auto header = m_tableTraf->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);

    const auto refreshTableTrafHeader = [&] {
        auto hh = m_tableTraf->horizontalHeader();
        hh->resizeSection(0, qBound(150, qRound(hh->width() * 0.3f), 180));
    };

    refreshTableTrafHeader();

    connect(header, &QHeaderView::geometriesChanged, this, refreshTableTrafHeader);
}

void TrafficPage::setupAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        m_appInfoRow->refreshAppInfoVersion(appListCurrentPath(), appInfoCache());
    };

    refreshAppInfoVersion();

    connect(m_appListView, &TableView::currentIndexChanged, this, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, refreshAppInfoVersion);
}

void TrafficPage::setupAppListViewChanged()
{
    const auto refreshAppListViewChanged = [&] {
        const bool appSelected = (appListCurrentIndex() > 0);
        m_actAddProgram->setEnabled(appSelected);
        m_actRemoveApp->setEnabled(appSelected);
        m_appInfoRow->setVisible(appSelected);
    };

    refreshAppListViewChanged();

    connect(m_appListView, &TableView::currentIndexChanged, this, refreshAppListViewChanged);
}

void TrafficPage::updateTrafUnit()
{
    m_comboTrafUnit->setCurrentIndex(iniUser()->statTrafUnit());
}

void TrafficPage::updateTableTrafUnit()
{
    const auto trafUnit = static_cast<TrafListModel::TrafUnit>(iniUser()->statTrafUnit());

    if (trafListModel()->unit() != trafUnit) {
        trafListModel()->setUnit(trafUnit);
        trafListModel()->refresh();
    }
}

int TrafficPage::appListCurrentIndex() const
{
    return m_appListView->currentRow();
}

QString TrafficPage::appListCurrentPath() const
{
    const auto &appStatRow = appStatModel()->appStatRowAt(appListCurrentIndex());

    return appStatRow.appPath;
}
