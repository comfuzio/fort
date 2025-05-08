#ifndef APPSTATMODEL_H
#define APPSTATMODEL_H

#include <sqlite/sqlite_types.h>

#include <util/model/tablesqlmodel.h>

#include "appstatcolumn.h"

class AppInfoCache;
class StatManager;

struct AppStatRow : TableRow
{
    qint64 appId;

    qint64 downloadBytes = 0;
    qint64 uploadBytes = 0;

    QString appPath;
};

class AppStatModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit AppStatModel(QObject *parent = nullptr);

    StatManager *statManager() const;
    AppInfoCache *appInfoCache() const;
    SqliteDb *sqliteDb() const override;

    void initialize();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const AppStatRow &appStatRowAt(int row) const;

    static QString columnName(const AppStatColumn column);

public slots:
    void remove(int row = -1);

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_appStatRow; }

    QString sqlBase() const override;
    QString sqlOrderColumn() const override;

private:
    QVariant headerDataDisplay(int section, int role) const;
    QVariant headerDataDecoration(int section) const;

    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataDecoration(const QModelIndex &index) const;

private:
    mutable AppStatRow m_appStatRow;
};

#endif // APPSTATMODEL_H
