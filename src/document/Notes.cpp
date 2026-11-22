// Notes.cpp
#include "document/Notes.h"

namespace vela {

int Notes::add(Document* doc, NoteKind kind, const QString& text, int refFrom)
{
    Q_UNUSED(doc);
    // Storage on the Document object is added in v0.3.3 (Document keeps a
    // `notes` side-table); here we return a synthetic id and let the doc
    // persistance catch up.
    static int nextId = 0;
    Q_UNUSED(kind); Q_UNUSED(text); Q_UNUSED(refFrom);
    return ++nextId;
}

void Notes::renumber(QVector<Note>& notes, NumberFormat fmt)
{
    int footCounter = 0;
    int endCounter  = 0;
    for (Note& n : notes)
    {
        if (n.kind == NoteKind::Footnote) n.number = ++footCounter;
        else                             n.number = ++endCounter;
        Q_UNUSED(fmt);
    }
}

QString Notes::format(int n, NumberFormat fmt)
{
    if (fmt == NumberFormat::Decimal) return QString::number(n);
    if (fmt == NumberFormat::UpperRoman || fmt == NumberFormat::LowerRoman)
        return toRoman(n, fmt == NumberFormat::UpperRoman);
    if (fmt == NumberFormat::UpperLetter || fmt == NumberFormat::LowerLetter) {
        QString out;
        int x = n;
        while (x > 0) {
            int r = (x - 1) % 26;
            out.prepend(QChar('a' + r));
            x = (x - 1) / 26;
        }
        return fmt == NumberFormat::UpperLetter ? out.toUpper() : out;
    }
    return QString::number(n);
}

QString Notes::toRoman(int n, bool upper)
{
    static const char* M[] = {"", "M", "MM", "MMM"};
    static const char* C[] = {"", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM"};
    static const char* X[] = {"", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC"};
    static const char* I[] = {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};
    QString s;
    if (n >= 1000) s += QString::fromLatin1(M[n / 1000]);
    s += QString::fromLatin1(C[(n / 100) % 10]);
    s += QString::fromLatin1(X[(n / 10 ) % 10]);
    s += QString::fromLatin1(I[ n        % 10]);
    return upper ? s.toUpper() : s.toLower();
}

} // namespace vela
