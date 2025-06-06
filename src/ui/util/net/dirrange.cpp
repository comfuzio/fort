#include "dirrange.h"

#include <util/conf/confdata.h>

DirRange::DirRange(QObject *parent) : TextRange(parent) { }

bool DirRange::isEmpty() const
{
    return !(isIn() || isOut());
}

void DirRange::clear()
{
    TextRange::clear();

    m_isIn = false;
    m_isOut = false;
}

void DirRange::toList(QStringList &list) const
{
    if (isIn()) {
        list << "IN";
    }
    if (isOut()) {
        list << "OUT";
    }
}

TextRange::ParseError DirRange::parseText(const QString &text)
{
    if (text == "IN") {
        m_isIn = true;
    } else if (text == "OUT") {
        m_isOut = true;
    } else {
        return ErrorBadText;
    }

    return ErrorOk;
}

void DirRange::write(ConfData &confData) const
{
    confData.writeDirRange(*this);
}
