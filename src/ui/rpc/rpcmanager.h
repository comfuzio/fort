#ifndef RPCMANAGER_H
#define RPCMANAGER_H

#include <QObject>
#include <QVariant>

#include <control/control.h>
#include <util/ioc/iocservice.h>

struct ProcessCommandArgs;
class ControlWorker;

class RpcManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit RpcManager(QObject *parent = nullptr);

    Control::Command resultCommand() const { return m_resultCommand; }
    QVariantList resultArgs() const { return m_resultArgs; }

    ControlWorker *client() const { return m_client; }

    void setUp() override;
    void tearDown() override;

    bool waitResult();
    void sendResult(ControlWorker *w, bool ok, const QVariantList &args = {});

    bool invokeOnServer(Control::Command cmd, const QVariantList &args = {});
    bool doOnServer(
            Control::Command cmd, const QVariantList &args = {}, QVariantList *resArgs = nullptr);

    void invokeOnClients(Control::Command cmd, const QVariantList &args = {});

    bool processCommandRpc(const ProcessCommandArgs &p, ProcessCommandResult &r);

    template<typename F>
    constexpr static F *getProcessFunc(Control::Command command, F *const funcList[], int minIndex,
            int maxIndex, F *defaultFunc = nullptr)
    {
        if (command < minIndex || command > maxIndex)
            return defaultFunc;

        const int funcIndex = command - minIndex;
        return funcList[funcIndex];
    }

private:
    void setupServerSignals();

    void setupClient();
    void closeClient();

    bool checkClientValidated(ControlWorker *w) const;
    void initClientOnServer(ControlWorker *w) const;

    bool processManagerRpc(const ProcessCommandArgs &p, ProcessCommandResult &r);

private:
    Control::Command m_resultCommand = Control::CommandNone;
    QVariantList m_resultArgs;

    ControlWorker *m_client = nullptr;
};

#endif // RPCMANAGER_H
