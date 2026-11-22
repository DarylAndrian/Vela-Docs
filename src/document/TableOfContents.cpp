// TableOfContents.cpp
#include "document/TableOfContents.h"

namespace vela {

QVector<TocEntry> TableOfContents::build(const QString& md, const TocOptions& opts)
{
    QVector<TocEntry> out;
    int pos = 0;
    for (qsizetype i = 0; i < md.size(); ) {
        if (md[i] != QLatin1Char('#')) { ++i; ++pos; continue; }
        int hashes = 0;
        while (i + hashes < md.size() && md[i + hashes] == QLatin1Char('#')) ++hashes;
        if (hashes > opts.maxLevel) { ++i; ++pos; continue; }
        if (i + hashes >= md.size() || md[i + hashes] != QLatin1Char(' ')) {
            ++i; ++pos; continue;
        }
        int titleStart = i + hashes + 1;
        int titleEnd   = md.indexOf(QLatin1Char('\n'), titleStart);
        if (titleEnd < 0) titleEnd = md.size();
        TocEntry e;
        e.text    = md.mid(titleStart, titleEnd - titleStart).trimmed();
        e.level   = hashes;
        e.charPos = pos;
        // Slug: lowercase, spaces -> dashes, strip punctuation.
        QString s = e.text.toLower();
        for (QChar& c : s) if (c.isSpace()) c = QLatin1Char('-');
        s.removeIf([](QChar c){ return !c.isLetterOrNumber() && c != QLatin1Char('-'); });
        e.anchor  = s;
        out.append(e);
        i = titleEnd;
        pos = titleEnd;
    }
    return out;
}

QString TableOfContents::renderMarkdown(const QVector<TocEntry>& entries, const TocOptions& opts)
{
    QString out;
    QTextStream s(&out);
    for (const TocEntry& e : entries) {
        s << QString(2 * e.level, QLatin1Char(' ')) << "- [" << e.text << "](#"
          << e.anchor << ')';
        if (opts.leaderDots) s << " ( p.&nbsp;X )";
        s << '\n';
    }
    return out;
}

} // namespace vela
