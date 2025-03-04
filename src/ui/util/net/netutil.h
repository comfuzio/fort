#ifndef NETUTIL_H
#define NETUTIL_H

#include <QObject>
#include <QString>

#include <common/common_types.h>

class NetUtil
{
public:
    enum ProtocolType : quint8 {
        ProtoAny = 0,
        ProtoTcp = 6,
        ProtoUdp = 17,
    };

    static bool windowsSockInit();
    static void windowsSockCleanup();

    // Get IPv4 address mask
    static int ip4Mask(quint32 ip);

    static quint32 applyIp4Mask(quint32 ip, int nbits);
    static ip6_addr_t applyIp6Mask(ip6_addr_t ip, int nbits);

    static QByteArrayView ip6ToArrayView(const ip6_addr_t &ip);
    static const ip6_addr_t &arrayViewToIp6(const QByteArrayView &buf);

    static QString getHostName(const QString &address);

    static QStringList localIpNetworks();
    static QString localIpNetworksText(int count = -1);

    static QString protocolName(quint8 ipProto);
    static quint8 protocolNumber(const QStringView name, bool &ok);

    static QString serviceName(quint16 port);
    static quint16 serviceToPort(const QStringView name, ProtocolType proto, bool &ok);
};

#endif // NETUTIL_H
