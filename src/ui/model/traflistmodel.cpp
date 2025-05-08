#include "traflistmodel.h"

#include <QLocale>

#include <stat/statmanager.h>
#include <stat/statsql.h>
#include <util/dateutil.h>
#include <util/formatutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

namespace {

bool checkTrafType(TrafListModel::TrafType type)
{
    if (type >= TrafListModel::TrafHourly && type <= TrafListModel::TrafTotal)
        return true;

    Q_UNREACHABLE();
    return false;
}

static const char *const sqlSelectMinTrafApps[] = {
    StatSql::sqlSelectMinTrafAppHour,
    StatSql::sqlSelectMinTrafAppDay,
    StatSql::sqlSelectMinTrafAppMonth,
    StatSql::sqlSelectMinTrafAppTotal,
};

static const char *const sqlSelectMinTrafs[] = {
    StatSql::sqlSelectMinTrafHour,
    StatSql::sqlSelectMinTrafDay,
    StatSql::sqlSelectMinTrafMonth,
    StatSql::sqlSelectMinTrafTotal,
};

const char *getSqlMinTrafTime(TrafListModel::TrafType type, qint64 appId)
{
    if (!checkTrafType(type))
        return nullptr;

    return (appId != 0 ? sqlSelectMinTrafApps : sqlSelectMinTrafs)[type];
}

static const char *const sqlSelectTrafApps[] = {
    StatSql::sqlSelectTrafAppHour,
    StatSql::sqlSelectTrafAppDay,
    StatSql::sqlSelectTrafAppMonth,
    StatSql::sqlSelectTrafAppTotal,
};

static const char *const sqlSelectTrafs[] = {
    StatSql::sqlSelectTrafHour,
    StatSql::sqlSelectTrafDay,
    StatSql::sqlSelectTrafMonth,
    StatSql::sqlSelectTrafTotal,
};

const char *getSqlSelectTraffic(TrafListModel::TrafType type, qint64 appId)
{
    if (!checkTrafType(type))
        return nullptr;

    return (appId != 0 ? sqlSelectTrafApps : sqlSelectTrafs)[type];
}

}

TrafListModel::TrafListModel(QObject *parent) : TableItemModel(parent) { }

StatManager *TrafListModel::statManager() const
{
    return IoC<StatManager>();
}

void TrafListModel::initialize()
{
    connect(statManager(), &StatManager::trafficCleared, this, &TrafListModel::resetTraf);
    connect(statManager(), &StatManager::appTrafTotalsResetted, this, &TrafListModel::resetTraf);
}

int TrafListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_trafCount;
}

int TrafListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return int(TrafListColumn::Count);
}

QVariant TrafListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return headerDataDisplay(section);

    // Icon
    case Qt::DecorationRole:
        return headerDataDecoration(section);
    }

    return {};
}

QVariant TrafListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index);
    }

    return {};
}

QVariant TrafListModel::headerDataDisplay(int section) const
{
    switch (TrafListColumn(section)) {
    case TrafListColumn::Date:
        return tr("Date");
    case TrafListColumn::Download:
        return tr("Download");
    case TrafListColumn::Upload:
        return tr("Upload");
    case TrafListColumn::Sum:
        return tr("Sum");
    }

    return {};
}

QVariant TrafListModel::headerDataDecoration(int section) const
{
    switch (TrafListColumn(section)) {
    case TrafListColumn::Download:
        return IconCache::icon(":/icons/green_down.png");
    case TrafListColumn::Upload:
        return IconCache::icon(":/icons/blue_up.png");
    }

    return {};
}

QVariant TrafListModel::dataDisplay(const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    updateRowCache(row);

    switch (TrafListColumn(column)) {
    case TrafListColumn::Date:
        return formatTrafTime(m_trafRow.trafTime);
    case TrafListColumn::Download:
        return formatTrafUnit(m_trafRow.inBytes);
    case TrafListColumn::Upload:
        return formatTrafUnit(m_trafRow.outBytes);
    case TrafListColumn::Sum:
        return formatTrafUnit(m_trafRow.inBytes + m_trafRow.outBytes);
    }

    return {};
}

void TrafListModel::resetTraf()
{
    const char *sqlMinTrafTime = getSqlMinTrafTime(type(), m_appId);

    beginResetModel();

    m_minTrafTime = statManager()->getTrafficTime(sqlMinTrafTime, m_appId);

    m_maxTrafTime = getMaxTrafTime(type());

    m_isEmpty = (m_minTrafTime == 0);

    if (m_minTrafTime == 0) {
        m_minTrafTime = m_maxTrafTime;
    }

    m_trafCount = getTrafCount(type(), m_minTrafTime, m_maxTrafTime);

    invalidateRowCache();

    endResetModel();
}

void TrafListModel::reset()
{
    if (m_isEmpty) {
        resetTraf();
    } else {
        TableItemModel::reset();
    }
}

bool TrafListModel::updateTableRow(const QVariantHash & /*vars*/, int row) const
{
    m_trafRow.trafTime = getTrafTime(row);

    const char *sqlSelectTraffic = getSqlSelectTraffic(type(), m_appId);

    statManager()->getTraffic(
            sqlSelectTraffic, m_trafRow.trafTime, m_trafRow.inBytes, m_trafRow.outBytes, m_appId);

    return true;
}

QString TrafListModel::formatTrafUnit(qint64 bytes) const
{
    if (bytes == 0) {
        return "0";
    }

    const int trafPrec = (unit() == UnitBytes) ? 0 : 2;

    if (unit() == UnitAdaptive) {
        return FormatUtil::formatDataSize(bytes, trafPrec);
    }

    const int power = unit() - 1;

    return FormatUtil::formatSize(bytes, power, trafPrec);
}

QString TrafListModel::formatTrafTime(qint32 trafTime) const
{
    const qint64 unixTime = DateUtil::toUnixTime(trafTime);

    switch (type()) {
    case TrafHourly:
        return DateUtil::formatHour(unixTime);
    case TrafDaily:
        return DateUtil::formatDay(unixTime);
    case TrafMonthly:
        return DateUtil::formatMonth(unixTime);
    case TrafTotal:
        return DateUtil::formatHour(unixTime);
    }
    return QString();
}

qint32 TrafListModel::getTrafTime(int row) const
{
    switch (type()) {
    case TrafHourly:
        return m_maxTrafTime - row;
    case TrafDaily:
        return m_maxTrafTime - row * 24;
    case TrafMonthly:
        return DateUtil::addUnixMonths(m_maxTrafTime, -row);
    case TrafTotal:
        return m_minTrafTime;
    }
    return 0;
}

qint32 TrafListModel::getTrafCount(TrafType type, qint32 minTrafTime, qint32 maxTrafTime)
{
    if (type == TrafTotal)
        return 1;

    const qint32 hours = maxTrafTime - minTrafTime + 1;
    if (type == TrafHourly)
        return hours;

    const qint32 days = hours / 24 + 1;
    if (type == TrafDaily)
        return days;

    const qint32 months = days / 30 + 1;
    if (type == TrafMonthly)
        return months;

    Q_UNREACHABLE();
    return 0;
}

qint32 TrafListModel::getMaxTrafTime(TrafType type)
{
    const qint64 unixTime = DateUtil::getUnixTime();

    switch (type) {
    case TrafTotal:
        Q_FALLTHROUGH();
    case TrafHourly:
        return DateUtil::getUnixHour(unixTime);
    case TrafDaily:
        return DateUtil::getUnixDay(unixTime);
    case TrafMonthly:
        return DateUtil::getUnixMonth(unixTime);
    }

    Q_UNREACHABLE();
    return 0;
}
