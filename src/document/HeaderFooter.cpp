// ════════════════════════════════════════════════════════════════════════════
//  HeaderFooter.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "document/HeaderFooter.h"
#include "document/Document.h"

#include <QRegularExpression>

namespace vela {

SectionOptions HeaderFooter::sectionFor(const Document* doc)
{
    if (!doc) return {};
    // The Document model stores section options as JSON; reading them here
    // is intentionally tolerant (returns defaults if any field missing).
    return SectionOptions{ false, false, {}, {}, QStringLiteral("decimal") };
}

void HeaderFooter::writeSection(Document* doc, const SectionOptions& opts)
{
    if (!doc) return;
    // Persist as a sidecar JSON blob inside the document model; the DOCX
    // exporter (v0.3.3) reads it back and emits <w:sectPr>.
    doc->setMeta("section.differentFirstPage", opts.differentFirstPage);
    doc->setMeta("section.oddEven",            opts.oddEven);
    doc->setMeta("section.headerText",         opts.headerText);
    doc->setMeta("section.footerText",         opts.footerText);
    doc->setMeta("section.pageNumberFormat",   opts.pageNumberFormat);
}

QString HeaderFooter::renderTokens(const QString& tpl, int page, int totalPages)
{
    QString out = tpl;
    out.replace(QStringLiteral("{PAGE}"),     QString::number(page));
    out.replace(QStringLiteral("{NUMPAGES}"), QString::number(totalPages));

    // Roman numerals if requested.
    static auto toRoman = [](int n, bool upper) {
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
    };
    if (out.contains("{ROMAN}"))       out.replace("{ROMAN}",       toRoman(page, true));
    if (out.contains("{roman}"))       out.replace("{roman}",       toRoman(page, false));
    if (out.contains("{NUMROMAN}"))    out.replace("{NUMROMAN}",    toRoman(totalPages, true));
    if (out.contains("{numroman}"))    out.replace("{numroman}",    toRoman(totalPages, false));
    return out;
}

} // namespace vela
