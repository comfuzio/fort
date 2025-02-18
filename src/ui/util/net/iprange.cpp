#include "iprange.h"

#include <common/fortconf.h>

#include <util/conf/confdata.h>
#include <util/stringutil.h>

#include "netformatutil.h"
#include "netutil.h"

namespace {

inline bool checkIp4MaskBitsCount(const int nbits)
{
    return (nbits >= 0 && nbits <= 32);
}

inline bool checkIp6MaskBitsCount(const int nbits)
{
    return (nbits >= 0 && nbits <= 128);
}

inline bool compareLessIp6(const ip6_addr_t &l, const ip6_addr_t &r)
{
    return fort_ip6_cmp(&l, &r) < 0;
}

inline void sortIp6Array(ip6_arr_t &array)
{
    std::sort(array.begin(), array.end(), compareLessIp6);
}

void sortIp6PairArray(ip6_arr_t &fromArray, ip6_arr_t &toArray)
{
    Q_ASSERT(fromArray.size() == toArray.size());

    const int arraySize = fromArray.size();

    ip6_pair_arr_t pairArray;
    pairArray.reserve(arraySize);

    for (int i = 0; i < arraySize; ++i) {
        pairArray.append(Ip6Pair { fromArray[i], toArray[i] });
    }

    std::sort(pairArray.begin(), pairArray.end(),
            [](const Ip6Pair &l, const Ip6Pair &r) { return compareLessIp6(l.from, r.from); });

    for (int i = 0; i < arraySize; ++i) {
        const Ip6Pair &pair = pairArray[i];
        fromArray[i] = pair.from;
        toArray[i] = pair.to;
    }
}

}

IpRange::IpRange(QObject *parent) : ValueRange(parent) { }

bool IpRange::isEmpty() const
{
    return ip4Size() == 0 && pair4Size() == 0 && ip6Size() == 0 && pair6Size() == 0;
}

bool IpRange::checkSize() const
{
    return (ip4Size() + pair4Size()) < FORT_CONF_IP_MAX
            && (ip6Size() + pair6Size()) < FORT_CONF_IP_MAX;
}

int IpRange::sizeToWrite() const
{
    return FORT_CONF_ADDR_LIST_SIZE(ip4Size(), pair4Size(), ip6Size(), pair6Size());
}

void IpRange::clear()
{
    ValueRange::clear();

    m_ip4Array.clear();
    m_pair4FromArray.clear();
    m_pair4ToArray.clear();

    m_ip6Array.clear();
    m_pair6FromArray.clear();
    m_pair6ToArray.clear();
}

void IpRange::toList(QStringList &list) const
{
    for (int i = 0, n = ip4Size(); i < n; ++i) {
        const ip4_t ip = ip4At(i);

        list << NetFormatUtil::ip4ToText(ip);
    }

    for (int i = 0, n = pair4Size(); i < n; ++i) {
        const Ip4Pair ip = pair4At(i);

        list << QString("%1-%2").arg(
                NetFormatUtil::ip4ToText(ip.from), NetFormatUtil::ip4ToText(ip.to));
    }

    for (int i = 0, n = ip6Size(); i < n; ++i) {
        const ip6_addr_t ip = ip6At(i);

        list << NetFormatUtil::ip6ToText(ip);
    }

    for (int i = 0, n = pair6Size(); i < n; ++i) {
        const Ip6Pair ip = pair6At(i);

        list << QString("%1-%2").arg(
                NetFormatUtil::ip6ToText(ip.from), NetFormatUtil::ip6ToText(ip.to));
    }
}

bool IpRange::fromList(const StringViewList &list, bool sort)
{
    clear();

    ip4range_map_t ip4RangeMap;
    int pair4Size = 0;

    int lineNo = 0;
    for (const auto &line : list) {
        ++lineNo;

        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        if (parseIpLine(line, ip4RangeMap, pair4Size) != ErrorOk) {
            appendErrorDetails(QString("line='%1'").arg(line));
            setErrorLineNo(lineNo);
            return false;
        }
    }

    fillRangeArrays<ip4_t>({
            .rangeMap = ip4RangeMap,
            .valuesArray = m_ip4Array,
            .pairFromArray = m_pair4FromArray,
            .pairToArray = m_pair4ToArray,
            .pairSize = pair4Size,
    });

    if (sort) {
        sortIp6Array(m_ip6Array);
        sortIp6PairArray(m_pair6FromArray, m_pair6ToArray);
    }

    return true;
}

IpRange::ParseError IpRange::parseIpLine(
        const QStringView line, ip4range_map_t &ip4RangeMap, int &pair4Size)
{
    static const QRegularExpression ipRe(R"(^\[?([A-Fa-f\d:.]+)\]?\s*([\/-]?)\s*(\S*))");

    const auto match = StringUtil::match(ipRe, line);
    if (!match.hasMatch()) {
        setErrorMessage(tr("Bad format"));
        return ErrorBadFormat;
    }

    const auto ip = match.captured(1);
    const auto sepStr = match.capturedView(2);
    const auto mask = match.captured(3);

    if (sepStr.isEmpty() != mask.isEmpty()) {
        setErrorMessage(tr("Bad mask"));
        setErrorDetails(QString("ip='%1' sep='%2' mask='%3'").arg(ip, sepStr, mask));
        return ErrorBadMaskFormat;
    }

    const char maskSep = sepStr.isEmpty() ? '\0' : sepStr.at(0).toLatin1();
    const bool isIPv6 = ip.contains(':');

    return isIPv6 ? parseIp6Address(ip, mask, maskSep)
                  : parseIp4Address(ip, mask, ip4RangeMap, pair4Size, maskSep);
}

IpRange::ParseError IpRange::parseIp4Address(const QStringView ip, const QStringView mask,
        ip4range_map_t &ip4RangeMap, int &pair4Size, char maskSep)
{
    ip4_t from, to = 0;

    bool ok;
    from = NetFormatUtil::textToIp4(ip, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad IP address"));
        setErrorDetails(QString("IPv4 ip='%1'").arg(ip));
        return ErrorBadAddress;
    }

    const ParseError err = parseIp4AddressMask(mask, from, to, maskSep);
    if (err != ErrorOk)
        return err;

    ip4RangeMap.insert(from, to);

    if (from != to) {
        ++pair4Size;
    }

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp4AddressMask(
        const QStringView mask, ip4_t &from, ip4_t &to, char maskSep)
{
    switch (maskSep) {
    case '-': // e.g. "127.0.0.0-127.255.255.255"
        return parseIp4AddressMaskFull(mask, from, to);
    case '/':
    case '\0': // e.g. "127.0.0.0/24", "127.0.0.0"
        return parseIp4AddressMaskPrefix(mask, from, to);
    default:
        return ErrorOk;
    }
}

IpRange::ParseError IpRange::parseIp4AddressMaskFull(const QStringView mask, ip4_t &from, ip4_t &to)
{
    bool ok;
    to = NetFormatUtil::textToIp4(mask, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad second IP address"));
        setErrorDetails(QString("IPv4 ip='%1'").arg(mask));
        return ErrorBadAddress2;
    }

    if (from > to) {
        setErrorMessage(tr("Bad range"));
        setErrorDetails(QString("IPv4 from='%1' to='%2'").arg(from, to));
        return ErrorBadRange;
    }

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp4AddressMaskPrefix(
        const QStringView mask, ip4_t &from, ip4_t &to)
{
    bool ok = true;
    const int nbits = mask.isEmpty() ? emptyNetMask() : mask.toInt(&ok);

    if (!ok || !checkIp4MaskBitsCount(nbits)) {
        setErrorMessage(tr("Bad mask"));
        setErrorDetails(QString("IPv4 mask='%1' nbits='%2'").arg(mask, QString::number(nbits)));
        return ErrorBadMask;
    }

    to = NetUtil::applyIp4Mask(from, nbits);

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp6Address(
        const QStringView ip, const QStringView &mask, char maskSep)
{
    bool hasMask = false;
    ip6_addr_t from, to;

    bool ok;
    from = NetFormatUtil::textToIp6(ip, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad IP address"));
        setErrorDetails(QString("IPv6 ip='%1'").arg(ip));
        return ErrorBadAddress;
    }

    const ParseError err = parseIp6AddressMask(mask, from, to, hasMask, maskSep);
    if (err != ErrorOk)
        return err;

    if (hasMask) {
        m_pair6FromArray.append(from);
        m_pair6ToArray.append(to);
    } else {
        m_ip6Array.append(from);
    }

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp6AddressMask(
        const QStringView mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask, char maskSep)
{
    hasMask = false;

    switch (maskSep) {
    case '-': // e.g. "::1 - ::2"
        return parseIp6AddressMaskFull(mask, to, hasMask);
    case '/': // e.g. "::1/24", "::1"
        return parseIp6AddressMaskPrefix(mask, from, to, hasMask);
    default:
        return ErrorOk;
    }
}

IpRange::ParseError IpRange::parseIp6AddressMaskFull(
        const QStringView mask, ip6_addr_t &to, bool &hasMask)
{
    bool ok;
    to = NetFormatUtil::textToIp6(mask, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad second IP address"));
        setErrorDetails(QString("IPv6 ip='%1'").arg(mask));
        return ErrorBadAddress2;
    }

    hasMask = true;

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp6AddressMaskPrefix(
        const QStringView mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask)
{
    bool ok;
    const int nbits = mask.toInt(&ok);

    if (!ok || !checkIp6MaskBitsCount(nbits)) {
        setErrorMessage(tr("Bad mask"));
        setErrorDetails(QString("IPv6 mask='%1' nbits='%2'").arg(mask, QString::number(nbits)));
        return ErrorBadMask;
    }

    if (nbits == 128)
        return ErrorOk;

    to = NetUtil::applyIp6Mask(from, nbits);

    hasMask = true;

    return ErrorOk;
}

void IpRange::write(ConfData &confData) const
{
    confData.writeAddressList(*this);
}
