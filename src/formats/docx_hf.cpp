// docx_hf.cpp — v0.3.2 (MAJOR-01) extended in v0.3.3 (IMP-09)
//
// DOCX writer/reader for header/footer parts and section properties.
//
// OOXML summary:
//   word/document.xml      → <w:body>...<w:sectPr>...</w:sectPr>
//   word/header1.xml       → referenced by <w:headerReference r:id="..."/>
//   word/footer1.xml       → referenced by <w:footerReference r:id="..."/>
//   word/_rels/document.xml.rels → adds Target="header1.xml"
//
// This file intentionally stays small — the surface it owns is finite.

#include "formats/docx_hf.h"
#include "util/Logger.h"
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMap>

namespace vela {

namespace {
constexpr auto NS_W = "http://schemas.openxmlformats.org/wordprocessingml/2006/main";
}

QByteArray DocxHF::serializeSectionProps(const SectionOptions& opts)
{
    QString out;
    QXmlStreamWriter w(&out);
    w.setAutoFormatting(false);
    w.writeStartElement(QString::fromLatin1(NS_W), QStringLiteral("sectPr"));

    if (!opts.headerText.isEmpty()) {
        w.writeEmptyElement(QString::fromLatin1(NS_W), QStringLiteral("headerReference"));
        w.writeAttribute(QStringLiteral("r:id"), QStringLiteral("rIdHdr1"));
    }
    if (!opts.footerText.isEmpty()) {
        w.writeEmptyElement(QString::fromLatin1(NS_W), QStringLiteral("footerReference"));
        w.writeAttribute(QStringLiteral("r:id"), QStringLiteral("rIdFtr1"));
    }
    if (opts.differentFirstPage)
        w.writeEmptyElement(QString::fromLatin1(NS_W), QStringLiteral("titlePg"));

    w.writeTextElement(QString::fromLatin1(NS_W), QStringLiteral("pgSz"),
                       QStringLiteral("12240"));   // 8.5" × 11" = 12240 × 15840
    w.writeTextElement(QString::fromLatin1(NS_W), QStringLiteral("pgMar"),
                       QStringLiteral("1440 1440 1440 1440")); // 1" all sides
    w.writeEndElement();
    return out.toUtf8();
}

SectionOptions DocxHF::parseSectionProps(const QByteArray& xml)
{
    SectionOptions o;
    QXmlStreamReader r(xml);
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement()) {
            const auto name = r.name();
            if (name == QStringLiteral("titlePg"))
                o.differentFirstPage = true;
        }
    }
    if (r.hasError())
        Logger::warning(QStringLiteral("DocxHF: parse error %1").arg(r.errorString()));
    return o;
}

} // namespace vela
