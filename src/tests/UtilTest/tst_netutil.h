#pragma once

#include <QFileInfo>
#include <QSignalSpy>

#include <googletest.h>

#include <task/taskzonedownloader.h>
#include <util/fileutil.h>
#include <util/net/arearange.h>
#include <util/net/dirrange.h>
#include <util/net/iprange.h>
#include <util/net/netformatutil.h>
#include <util/net/netutil.h>
#include <util/net/portrange.h>
#include <util/net/protorange.h>

class NetUtilTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void NetUtilTest::SetUp()
{
    NetUtil::windowsSockInit();
}

void NetUtilTest::TearDown()
{
    NetUtil::windowsSockCleanup();
}

TEST_F(NetUtilTest, ip4Text)
{
    const QString ip4Str("172.16.0.1");

    ASSERT_EQ(NetFormatUtil::ip4ToText(NetFormatUtil::textToIp4(ip4Str)), ip4Str);
}

TEST_F(NetUtilTest, ip6Text)
{
    const QString ip6Str("::1");

    ASSERT_EQ(NetFormatUtil::ip6ToText(NetFormatUtil::textToIp6(ip6Str)), ip6Str);
}

TEST_F(NetUtilTest, ip6Bytes01)
{
    const ip6_addr_t ip = NetFormatUtil::textToIp6("ff02::1:3");

    ASSERT_EQ(ip.addr32[0], 0x2ff);
    ASSERT_EQ(ip.addr32[1], 0);
    ASSERT_EQ(ip.addr32[2], 0);
    ASSERT_EQ(ip.addr32[3], 0x3000100);
}

TEST_F(NetUtilTest, ip6Bytes02)
{
    const ip6_addr_t ip = NetFormatUtil::textToIp6("fe80::e58c:84f8:a156:2a23");

    ASSERT_EQ(ip.addr32[0], 0x80fe);
    ASSERT_EQ(ip.addr32[1], 0);
    ASSERT_EQ(ip.addr32[2], 0xf8848ce5);
    ASSERT_EQ(ip.addr32[3], 0x232a56a1);
}

TEST_F(NetUtilTest, ip6Mask01)
{
    const ip6_addr_t ip = NetUtil::applyIp6Mask(NetFormatUtil::textToIp6("::2"), 126);

    ASSERT_EQ(ip.addr32[0], 0);
    ASSERT_EQ(ip.addr32[1], 0);
    ASSERT_EQ(ip.addr32[2], 0);
    ASSERT_EQ(ip.addr32[3], 0x03000000);
}

TEST_F(NetUtilTest, ip4Ranges)
{
    IpRange ipRange;

    ASSERT_FALSE(ipRange.fromText("172.16.0.0/33"));
    ASSERT_FALSE(ipRange.fromText("172.16.0.255/-16"));
    ASSERT_FALSE(ipRange.fromText("10.0.0.32 - 10.0.0.24"));
    ASSERT_EQ(ipRange.errorLineNo(), 1);

    ASSERT_TRUE(ipRange.fromText("172.16.0.1/32"));
    ASSERT_EQ(ipRange.toText(), QString("172.16.0.1\n"));

    ASSERT_TRUE(ipRange.fromText("172.16.0.1/0"));
    ASSERT_EQ(ipRange.toText(), QString("172.16.0.1-255.255.255.255\n"));

    // Simple range
    {
        ASSERT_TRUE(ipRange.fromText("127.0.0.1\n"
                                     "172.16.0.0/20\n"
                                     "192.168.0.0 - 192.168.255.255\n"));
        ASSERT_EQ(ipRange.errorLineNo(), 0);
        ASSERT_EQ(ipRange.pair4Size(), 2);
        ASSERT_EQ(ipRange.ip4Size(), 1);

        const Ip4Pair &ipPair1 = ipRange.pair4At(0);
        ASSERT_EQ(ipPair1.from, NetFormatUtil::textToIp4("172.16.0.0"));
        ASSERT_EQ(ipPair1.to, NetFormatUtil::textToIp4("172.16.15.255"));

        const Ip4Pair &ipPair2 = ipRange.pair4At(1);
        ASSERT_EQ(ipPair2.from, NetFormatUtil::textToIp4("192.168.0.0"));
        ASSERT_EQ(ipPair2.to, NetFormatUtil::textToIp4("192.168.255.255"));

        ASSERT_EQ(ipRange.ip4At(0), NetFormatUtil::textToIp4("127.0.0.1"));
    }

    // Merge ranges
    {
        ASSERT_TRUE(ipRange.fromText("10.0.0.0 - 10.0.0.255\n"
                                     "10.0.0.64 - 10.0.0.128\n"
                                     "10.0.0.128 - 10.0.2.0\n"));
        ASSERT_EQ(ipRange.ip4Size(), 0);
        ASSERT_EQ(ipRange.pair4Size(), 1);

        const Ip4Pair &ipPair1 = ipRange.pair4At(0);
        ASSERT_EQ(ipPair1.from, NetFormatUtil::textToIp4("10.0.0.0"));
        ASSERT_EQ(ipPair1.to, NetFormatUtil::textToIp4("10.0.2.0"));
    }
}

TEST_F(NetUtilTest, ip6Ranges)
{
    IpRange ipRange;

    ASSERT_FALSE(ipRange.fromText("::1/129"));
    ASSERT_FALSE(ipRange.fromText("::1/-16"));
    ASSERT_EQ(ipRange.errorLineNo(), 1);

    ASSERT_TRUE(ipRange.fromText("::1/128"));
    ASSERT_EQ(ipRange.toText(), QString("::1\n"));

    ASSERT_TRUE(ipRange.fromText("2002::/16"));
    ASSERT_EQ(ipRange.toText(), QString("2002::-2002:ffff:ffff:ffff:ffff:ffff:ffff:ffff\n"));

    ASSERT_TRUE(ipRange.fromText("[::2]/126\n"
                                 "[::1]/126\n"));
    ASSERT_EQ(ipRange.toText(),
            QString("::1-::3\n"
                    "::2-::3\n"));
}

TEST_F(NetUtilTest, portRanges)
{
    PortRange portRange;

    ASSERT_FALSE(portRange.fromText("-16"));
    ASSERT_EQ(portRange.errorLineNo(), 1);

    ASSERT_TRUE(portRange.fromText("1-128"));
    ASSERT_EQ(portRange.toText(), QString("1-128\n"));

    // TCP Prots
    portRange.setProtoTcp(true);

    ASSERT_TRUE(portRange.fromText("http\n"
                                   "https\n"));
    ASSERT_EQ(portRange.toText(),
            QString("80\n"
                    "443\n"));

    ASSERT_TRUE(portRange.fromText("ISO_TSAP-SSL"));
    ASSERT_EQ(portRange.toText(), QString("102-465\n"));
}

TEST_F(NetUtilTest, protocolRanges)
{
    ProtoRange protoRange;

    ASSERT_FALSE(protoRange.fromText("-16"));
    ASSERT_EQ(protoRange.errorLineNo(), 1);

    ASSERT_TRUE(protoRange.fromText("1-128"));
    ASSERT_EQ(protoRange.toText(), QString("1-128\n"));

    ASSERT_TRUE(protoRange.fromText("tcp\n"
                                    "udp\n"));
    ASSERT_EQ(protoRange.toText(),
            QString("TCP\n"
                    "UDP\n"));

    ASSERT_TRUE(protoRange.fromText("HOPOPT-IPV6_FRAG"));
    ASSERT_EQ(protoRange.toText(), QString("0-44\n"));

    ASSERT_TRUE(protoRange.fromText("TP++-A/N"));
    ASSERT_EQ(protoRange.toText(), QString("39-107\n"));

    ASSERT_TRUE(protoRange.fromText("AX.25-RAWSOCKET"));
    ASSERT_EQ(protoRange.toText(), QString("93-255\n"));
}

TEST_F(NetUtilTest, protocolNames)
{
    ProtoRange protoRange;

    ASSERT_TRUE(protoRange.fromText("ICMP\n"
                                    "IGMP\n"
                                    "TCP\n"
                                    "UDP\n"
                                    "ICMPv6\n"
                                    "RAWSOCKET\n"));
}

TEST_F(NetUtilTest, directionRanges)
{
    DirRange dirRange;

    ASSERT_FALSE(dirRange.fromText("1"));
    ASSERT_EQ(dirRange.errorLineNo(), 1);

    ASSERT_TRUE(dirRange.fromText("in"));
    ASSERT_EQ(dirRange.toText(), QString("IN\n"));

    ASSERT_TRUE(dirRange.fromText("out\n"
                                  "in\n"));
    ASSERT_EQ(dirRange.toText(),
            QString("IN\n"
                    "OUT\n"));
}

TEST_F(NetUtilTest, areaRanges)
{
    AreaRange areaRange;

    ASSERT_FALSE(areaRange.fromText("1"));
    ASSERT_EQ(areaRange.errorLineNo(), 1);

    ASSERT_TRUE(areaRange.fromText("lan"));
    ASSERT_EQ(areaRange.toText(), QString("LAN\n"));

    ASSERT_TRUE(areaRange.fromText("inet\n"
                                   "localhost\n"
                                   "lan\n"));
    ASSERT_EQ(areaRange.toText(),
            QString("LOCALHOST\n"
                    "LAN\n"
                    "INET\n"));
}

TEST_F(NetUtilTest, taskTasix)
{
    const QByteArray buf = FileUtil::readFileData(":/data/tasix-mrlg.html");
    ASSERT_FALSE(buf.isEmpty());

    TaskZoneDownloader tasix;
    tasix.setZoneId(1);
    tasix.setSort(true);
    tasix.setEmptyNetMask(24);
    tasix.setPattern("^\\*\\D{2,5}([\\d./-]{7,})");

    QString textChecksum;
    const auto text = QString::fromLatin1(buf);
    const auto list = tasix.parseAddresses(text, textChecksum);
    ASSERT_FALSE(textChecksum.isEmpty());
    ASSERT_FALSE(list.isEmpty());

    const QString cachePath("./zones/");

    tasix.setTextChecksum(textChecksum);
    tasix.setCachePath(cachePath);
    ASSERT_TRUE(tasix.storeAddresses(list));

    const QFileInfo out(cachePath + "tasix-mrlg.txt");
    ASSERT_TRUE(tasix.saveAddressesAsText(out.filePath()));
    ASSERT_GT(out.size(), 0);
}
