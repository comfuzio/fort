#include "axistickerspeed.h"

#include <util/formatutil.h>

double AxisTickerSpeed::getTickStep(const QCPRange &range)
{
    const double exactStep = range.size() / tickCount();
    constexpr int tickStep = 2;

    return qPow(tickStep, qFloor(qLn(exactStep) / qLn(tickStep) + 0.5));
}

QString AxisTickerSpeed::getTickLabel(
        double tick, const QLocale &locale, QChar formatChar, int precision)
{
    Q_UNUSED(locale);
    Q_UNUSED(formatChar);
    Q_UNUSED(precision);

    return FormatUtil::formatSpeed(qint64(tick), unitFormat());
}
