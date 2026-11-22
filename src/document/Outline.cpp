// Outline.cpp
#include "document/Outline.h"

namespace vela {

QVector<OutlineEntry> Outline::fromMarkdown(const QString& md)
{
    QVector<OutlineEntry> out;
    int pos = 0;
    for (qsizetype i = 0; i < md.size(); ) {
        if (md[i] != QLatin1Char('#')) { ++i; ++pos; continue; }
        int hashes = 0;
        while (i + hashes < md.size() && md[i + hashes] == QLatin1Char('#')) ++hashes;
        if (hashes > 6) { ++i; ++pos; continue; }
        if (i + hashes >= md.size() || md[i + hashes] != QLatin1Char(' ')) {
            ++i; ++pos; continue;
        }
        int titleStart = i + hashes + 1;
        int titleEnd   = md.indexOf(QLatin1Char('\n'), titleStart);
        if (titleEnd < 0) titleEnd = md.size();
        OutlineEntry e;
        e.title   = md.mid(titleStart, titleEnd - titleStart).trimmed();
        e.level   = qBound(1, hashes, 6);
        e.charPos = pos;
        out.append(e);
        i = titleEnd;
        pos = titleEnd;
    }
    return out;
}

} // namespace vela
