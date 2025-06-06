#ifndef CONFZONEMANAGERRPC_H
#define CONFZONEMANAGERRPC_H

#include <conf/confzonemanager.h>
#include <control/control_types.h>

class RpcManager;

class ConfZoneManagerRpc : public ConfZoneManager
{
    Q_OBJECT

public:
    explicit ConfZoneManagerRpc(QObject *parent = nullptr);

    bool addOrUpdateZone(Zone &zone) override;
    bool deleteZone(quint8 zoneId) override;
    bool updateZoneName(quint8 zoneId, const QString &zoneName) override;
    bool updateZoneEnabled(quint8 zoneId, bool enabled) override;

    static QVariantList zoneToVarList(const Zone &zone);
    static Zone varListToZone(const QVariantList &v);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);
};

#endif // CONFZONEMANAGERRPC_H
