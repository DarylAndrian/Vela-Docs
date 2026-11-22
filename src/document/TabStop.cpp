// ════════════════════════════════════════════════════════════════════════════
//  TabStop.cpp — v0.4.0
// ════════════════════════════════════════════════════════════════════════════
#include "document/TabStop.h"

namespace vela {

QString TabStop::toDocxXml() const
{
    QString out;
    out += QStringLiteral("<w:tab w:val=\"%1\" w:pos=\"%2\"")
               .arg(typeToString(m_type))
               .arg(qRound(m_pos * 1440.0));   // inches → twentieths of a point
    if (m_leader != None) {
        out += QStringLiteral(" w:leader=\"%1\"").arg(leaderToString(m_leader));
    }
    out += QStringLiteral("/>");
    return out;
}

TabStop::Type TabStop::typeFromString(const QString& s)
{
    if (s == QLatin1String("center"))   return Center;
    if (s == QLatin1String("right"))    return Right;
    if (s == QLatin1String("decimal"))  return Decimal;
    if (s == QLatin1String("bar"))      return Bar;
    return Left;
}

TabStop::Leader TabStop::leaderFromString(const QString& s)
{
    if (s == QLatin1String("dot"))        return Dot;
    if (s == QLatin1String("hyphen"))     return Hyphen;
    if (s == QLatin1String("underscore")) return Underscore;
    return None;
}

QString TabStop::typeToString(TabStop::Type t)
{
    switch (t) {
        case Left:    return QStringLiteral("left");
        case Center:  return QStringLiteral("center");
        case Right:   return QStringLiteral("right");
        case Decimal: return QStringLiteral("decimal");
        case Bar:     return QStringLiteral("bar");
    }
    return QStringLiteral("left");
}

QString TabStop::leaderToString(TabStop::Leader l)
{
    switch (l) {
        case None:       return QString();
        case Dot:        return QStringLiteral("dot");
        case Hyphen:     return QStringLiteral("hyphen");
        case Underscore: return QStringLiteral("underscore");
    }
    return QString();
}

} // namespace vela