// ════════════════════════════════════════════════════════════════════════════
//  DocxExporter.cpp — OOXML writer
//
//  Consumes genuine ProseMirror / TipTap JSON — the exact shape the editor
//  produces via `editor.getJSON()`:
//
//    {"type":"doc","content":[
//       {"type":"heading","attrs":{"level":1},"content":[
//          {"type":"text","text":"Title","marks":[{"type":"bold"}]}]},
//       {"type":"bulletList","content":[
//          {"type":"listItem","content":[{"type":"paragraph", ...}]}]},
//       {"type":"table","content":[{"type":"tableRow","content":[
//          {"type":"tableCell","attrs":{"colspan":1},"content":[ ... ]}]}]}
//    ]}
//
//  Marks are objects ({"type":"link","attrs":{"href":"…"}}). The legacy
//  bare-string form (["bold"]) is still accepted so DocxImporter round-trips.
//
//  Element order inside <w:pPr>, <w:rPr> and <w:tcPr> is significant — OOXML
//  defines it as a strict sequence and Word refuses files that violate it.
//  The helpers below emit children in schema order only.
//
//  Vela-specific metadata that has no OOXML representation travels in a
//  word/_vela.json part (declared as application/json, ignored by Word) rather
//  than being smuggled into styles.xml as invalid <w:style w:type="page">.
// ════════════════════════════════════════════════════════════════════════════

#include "formats/DocxExporter.h"
#include "util/Logger.h"
#include "util/Zip.h"

#include <QByteArray>
#include <QHash>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QRegularExpression>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <optional>

namespace vela {

DocxExporter::DocxExporter(QObject* parent)
    : QObject(parent)
{
}

namespace {

constexpr int kEmuPerPixel   = 9525;
constexpr int kDefaultTableW = 9360;   // 6.5" usable width at 1" margins, in twips

// Package parts accumulated while walking the document.
struct Package
{
    QHash<QString, QByteArray> entries;   // zip path -> bytes
    QStringList relationships;            // <Relationship …/> lines, id-ordered
    QStringList mediaExtensions;          // image extensions seen (for [Content_Types])
    QHash<QString, QString> headerFooterParts;
    int nextRelId   = 1;
    int nextMedia   = 1;
    int nextDocPrId = 1;                  // unique wp:docPr/@id across the document

    // Numbering is allocated while walking, so numbering.xml is written last.
    // Each list instance needs its OWN abstractNum: sharing one numId makes
    // Word continue a second ordered list from where the first stopped.
    int nextAbstractNumId = 0;
    int nextNumId         = 1;
    QStringList abstractNums;
    QStringList nums;
};

QString escapeXml(const QString& in)
{
    QString out;
    out.reserve(in.size() + 16);
    for (const QChar c : in) {
        switch (c.unicode()) {
        case u'&':  out += QLatin1String("&amp;");  break;
        case u'<':  out += QLatin1String("&lt;");   break;
        case u'>':  out += QLatin1String("&gt;");   break;
        case u'"':  out += QLatin1String("&quot;"); break;
        case u'\'': out += QLatin1String("&apos;"); break;
        default:
            // Drop control characters that are illegal in XML 1.0 outright rather
            // than emitting a document Word refuses to open.
            if (c.unicode() < 0x20 && c != u'\t' && c != u'\n' && c != u'\r') break;
            out += c;
        }
    }
    return out;
}

// CSS length ("16px", "12pt", "1.2rem") -> OOXML half-points.
int cssSizeToHalfPoints(const QString& cssSize, bool& ok)
{
    ok = false;
    QString s = cssSize.trimmed();
    if (s.isEmpty()) return 0;

    if (s.endsWith(QLatin1String("pt"))) {
        s.chop(2);
        const double pt = s.toDouble(&ok);
        return ok ? qMax(1, qRound(pt * 2.0)) : 0;
    }
    if (s.endsWith(QLatin1String("rem")) || s.endsWith(QLatin1String("em"))) {
        while (!s.isEmpty() && !s.endsWith(u'.') && !s[s.size() - 1].isDigit()) s.chop(1);
        const double rem = s.toDouble(&ok);
        return ok ? qMax(1, qRound(rem * 16.0 * 0.75 * 2.0)) : 0;   // 1rem = 16px
    }
    if (s.endsWith(QLatin1String("px"))) s.chop(2);

    const double px = s.toDouble(&ok);
    return ok ? qMax(1, qRound(px * 0.75 * 2.0)) : 0;
}

// CSS length -> twentieths of a point (the unit w:spacing uses).
int cssSizeToTwentieths(const QString& cssSize, bool& ok)
{
    const int halfPoints = cssSizeToHalfPoints(cssSize, ok);
    return ok ? halfPoints * 10 : 0;
}

int twipsFromPx(int px) { return qRound(px * 15.0); }   // 96dpi: 1px = 15 twips

QString sixDigitHex(const QString& cssColor)
{
    QString hex = cssColor.trimmed();
    if (hex.startsWith(u'#')) hex.remove(0, 1);
    if (hex.size() == 3) {   // #abc -> #aabbcc
        QString expanded;
        for (const QChar c : hex) { expanded += c; expanded += c; }
        hex = expanded;
    }
    if (hex.size() != 6) return {};
    for (const QChar c : hex) {
        if (!c.isDigit() && !(c >= u'a' && c <= u'f') && !(c >= u'A' && c <= u'F')) return {};
    }
    return hex.toUpper();
}

QString jcValue(const QString& textAlign)
{
    if (textAlign == QLatin1String("center"))  return QStringLiteral("center");
    if (textAlign == QLatin1String("right"))   return QStringLiteral("right");
    if (textAlign == QLatin1String("justify")) return QStringLiteral("both");
    if (textAlign == QLatin1String("left"))    return QStringLiteral("left");
    return {};
}

// ── Relationships ──────────────────────────────────────────────────────────
QString addRelationship(Package& pkg, const QString& type, const QString& target,
                        bool external = false)
{
    const QString id = QStringLiteral("rId%1").arg(pkg.nextRelId++);
    pkg.relationships += QStringLiteral(
        "  <Relationship Id=\"%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/%2\" Target=\"%3\"%4/>\n")
        .arg(id, type, escapeXml(target),
             external ? QStringLiteral(" TargetMode=\"External\"") : QString());
    return id;
}

// ── Marks ──────────────────────────────────────────────────────────────────
std::optional<QJsonObject> findMark(const QJsonArray& marks, const QString& type)
{
    for (const auto& m : marks) {
        if (m.isObject()) {
            const QJsonObject o = m.toObject();
            if (o.value(QStringLiteral("type")).toString() == type)
                return o.value(QStringLiteral("attrs")).toObject();
        } else if (m.isString() && m.toString() == type) {
            return QJsonObject{};
        }
    }
    return std::nullopt;
}

// Emits <w:rPr> children in CT_RPr schema order:
//   rFonts, b, i, strike, color, sz, szCs, highlight, u, shd
QString runProperties(const QJsonArray& marks)
{
    QString rFonts, sz;
    bool bold = false, italic = false, strike = false;
    QString color, highlight, underline, shading;

    if (findMark(marks, QStringLiteral("bold")))   bold = true;
    if (findMark(marks, QStringLiteral("italic"))) italic = true;
    if (findMark(marks, QStringLiteral("strike"))) strike = true;

    if (findMark(marks, QStringLiteral("code"))) {
        rFonts = QStringLiteral("<w:rFonts w:ascii=\"Consolas\" w:hAnsi=\"Consolas\" w:cs=\"Consolas\"/>");
        shading = QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"F2F2F2\"/>");
    }
    if (const auto u = findMark(marks, QStringLiteral("underline"))) {
        underline = u->value(QStringLiteral("decoration")).toString() == QLatin1String("double")
            ? QStringLiteral("<w:u w:val=\"double\"/>")
            : QStringLiteral("<w:u w:val=\"single\"/>");
    }
    if (const auto h = findMark(marks, QStringLiteral("highlight"))) {
        // Word's w:highlight takes a named colour; fall back to shading for hex.
        const QString raw = h->value(QStringLiteral("color")).toString();
        const QString hex = sixDigitHex(raw);
        if (!hex.isEmpty())
            shading = QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"%1\"/>").arg(hex);
        else if (!raw.isEmpty())
            highlight = QStringLiteral("<w:highlight w:val=\"%1\"/>").arg(escapeXml(raw));
    }
    if (const auto ts = findMark(marks, QStringLiteral("textStyle"))) {
        const QString family = ts->value(QStringLiteral("fontFamily")).toString();
        if (!family.isEmpty()) {
            rFonts = QStringLiteral("<w:rFonts w:ascii=\"%1\" w:hAnsi=\"%1\" w:cs=\"%1\"/>")
                         .arg(escapeXml(family));
        }
        bool ok = false;
        const int halfPoints = cssSizeToHalfPoints(ts->value(QStringLiteral("fontSize")).toString(), ok);
        if (ok && halfPoints > 0)
            sz = QStringLiteral("<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/>").arg(halfPoints);

        const QString hex = sixDigitHex(ts->value(QStringLiteral("color")).toString());
        if (!hex.isEmpty()) color = QStringLiteral("<w:color w:val=\"%1\"/>").arg(hex);
    }

    QString rPr;
    rPr += rFonts;
    if (bold)   rPr += QStringLiteral("<w:b/>");
    if (italic) rPr += QStringLiteral("<w:i/>");
    if (strike) rPr += QStringLiteral("<w:strike/>");
    rPr += color + sz + highlight + underline + shading;
    return rPr.isEmpty() ? QString() : QStringLiteral("<w:rPr>") + rPr + QStringLiteral("</w:rPr>");
}

// Splits text so tabs and newlines become real OOXML siblings of <w:t> rather
// than illegal content inside it.
QString renderTextRun(const QString& text, const QString& rPr)
{
    QString out, pending;
    auto flush = [&]() {
        if (pending.isEmpty()) return;
        out += QStringLiteral("<w:t xml:space=\"preserve\">") + escapeXml(pending)
             + QStringLiteral("</w:t>");
        pending.clear();
    };
    for (const QChar c : text) {
        if      (c == u'\t') { flush(); out += QStringLiteral("<w:tab/>"); }
        else if (c == u'\n') { flush(); out += QStringLiteral("<w:br/>"); }
        else                 { pending += c; }
    }
    flush();
    return out.isEmpty() ? QString() : QStringLiteral("<w:r>") + rPr + out + QStringLiteral("</w:r>");
}

// ── Images ─────────────────────────────────────────────────────────────────
QString renderImage(Package& pkg, const QJsonObject& attrs)
{
    const QString src = attrs.value(QStringLiteral("src")).toString();
    if (src.isEmpty()) return {};

    QString relId;
    int cx = 0, cy = 0;

    if (src.startsWith(QLatin1String("data:"))) {
        const int comma = src.indexOf(u',');
        if (comma < 0) return {};
        const QString meta = src.mid(5, comma - 5);            // image/png;base64
        const QString mime = meta.section(u';', 0, 0);
        QString ext = mime.section(u'/', 1, 1).toLower();
        if (ext.isEmpty()) ext = QStringLiteral("png");
        if (ext == QLatin1String("jpeg") || ext == QLatin1String("jpe")) ext = QStringLiteral("jpg");

        const QByteArray payload = src.mid(comma + 1).toLatin1();
        const QByteArray bytes = meta.contains(QLatin1String("base64"))
            ? QByteArray::fromBase64(payload)
            : QByteArray(QUrl::fromPercentEncoding(payload).toUtf8());
        if (bytes.isEmpty()) return {};

        const QString partName = QStringLiteral("image%1.%2").arg(pkg.nextMedia++).arg(ext);
        pkg.entries.insert(QStringLiteral("word/media/") + partName, bytes);
        if (!pkg.mediaExtensions.contains(ext)) pkg.mediaExtensions += ext;
        relId = addRelationship(pkg, QStringLiteral("image"), QStringLiteral("media/") + partName);

        QImage img;
        if (img.loadFromData(bytes)) {
            cx = img.width()  * kEmuPerPixel;
            cy = img.height() * kEmuPerPixel;
        }
    } else if (src.startsWith(QLatin1String("http://")) || src.startsWith(QLatin1String("https://"))) {
        relId = addRelationship(pkg, QStringLiteral("image"), src, /*external=*/true);
    } else {
        // A bare filesystem path is not portable inside a .docx.
        VE_LOG_WARNING("DocxExporter: skipping image with non-embeddable src");
        return {};
    }

    const int wAttr = attrs.value(QStringLiteral("width")).toInt(0);
    const int hAttr = attrs.value(QStringLiteral("height")).toInt(0);
    if (wAttr > 0) cx = wAttr * kEmuPerPixel;
    if (hAttr > 0) cy = hAttr * kEmuPerPixel;
    if (cx <= 0 || cy <= 0) { cx = 4572000; cy = 3048000; }   // 5" x 3.33"

    const int docPrId = pkg.nextDocPrId++;
    const QString alt  = escapeXml(attrs.value(QStringLiteral("alt")).toString());
    const QString name = escapeXml(QStringLiteral("Picture %1").arg(docPrId));

    return QStringLiteral(
        "<w:r><w:drawing><wp:inline distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\">"
        "<wp:extent cx=\"%1\" cy=\"%2\"/><wp:effectExtent l=\"0\" t=\"0\" r=\"0\" b=\"0\"/>"
        "<wp:docPr id=\"%3\" name=\"%4\" descr=\"%5\"/>"
        "<wp:cNvGraphicFramePr>"
        "<a:graphicFrameLocks xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" noChangeAspect=\"1\"/>"
        "</wp:cNvGraphicFramePr>"
        "<a:graphic xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">"
        "<a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">"
        "<pic:pic xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">"
        "<pic:nvPicPr><pic:cNvPr id=\"%3\" name=\"%4\" descr=\"%5\"/><pic:cNvPicPr/></pic:nvPicPr>"
        "<pic:blipFill><a:blip r:embed=\"%6\"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill>"
        "<pic:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"%1\" cy=\"%2\"/></a:xfrm>"
        "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></pic:spPr>"
        "</pic:pic></a:graphicData></a:graphic></wp:inline></w:drawing></w:r>")
        .arg(cx).arg(cy).arg(docPrId).arg(name, alt, relId);
}

// ── Inline content ─────────────────────────────────────────────────────────
QString renderInlineNode(Package& pkg, const QJsonObject& node)
{
    const QString type = node.value(QStringLiteral("type")).toString();

    if (type == QLatin1String("text")) {
        const QJsonArray marks = node.value(QStringLiteral("marks")).toArray();
        const QString inner = renderTextRun(node.value(QStringLiteral("text")).toString(),
                                            runProperties(marks));
        if (inner.isEmpty()) return {};

        if (const auto link = findMark(marks, QStringLiteral("link"))) {
            const QString href = link->value(QStringLiteral("href")).toString();
            if (!href.isEmpty()) {
                const QString relId = addRelationship(pkg, QStringLiteral("hyperlink"),
                                                      href, /*external=*/true);
                return QStringLiteral("<w:hyperlink r:id=\"%1\">%2</w:hyperlink>").arg(relId, inner);
            }
        }
        return inner;
    }
    if (type == QLatin1String("hardBreak")) return QStringLiteral("<w:r><w:br/></w:r>");
    if (type == QLatin1String("pageBreak")) return QStringLiteral("<w:r><w:br w:type=\"page\"/></w:r>");
    if (type == QLatin1String("image"))     return renderImage(pkg, node.value(QStringLiteral("attrs")).toObject());
    return {};
}

QString renderInlines(Package& pkg, const QJsonArray& content)
{
    QString out;
    for (const auto& c : content) {
        if (c.isObject()) out += renderInlineNode(pkg, c.toObject());
    }
    return out;
}

// ── Paragraph properties ───────────────────────────────────────────────────
QString styleIdForBlock(const QJsonObject& node)
{
    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("heading")) {
        const int level = node.value(QStringLiteral("attrs")).toObject()
                              .value(QStringLiteral("level")).toInt(1);
        return QStringLiteral("Heading%1").arg(qBound(1, level, 6));
    }
    if (type == QLatin1String("title"))    return QStringLiteral("Title");
    if (type == QLatin1String("subtitle")) return QStringLiteral("Subtitle");
    if (type == QLatin1String("blockquote")) return QStringLiteral("Quote");

    const QJsonObject attrs = node.value(QStringLiteral("attrs")).toObject();
    const QString custom = attrs.value(QStringLiteral("customStyle")).toString();
    if (!custom.isEmpty()) return custom;
    const QString legacy = node.value(QStringLiteral("style")).toString();
    if (!legacy.isEmpty()) return legacy;
    return {};
}

// Emits <w:pPr> children in CT_PPr schema order:
//   pStyle, keepNext, numPr, pBdr, shd, spacing, ind, jc, sectPr
QString paragraphProperties(const QJsonObject& node, const QString& numberingXml,
                            const QString& sectionXml = QString())
{
    const QJsonObject attrs = node.value(QStringLiteral("attrs")).toObject();
    const QString type = node.value(QStringLiteral("type")).toString();

    QString pStyle, keepNext, frame, pBdr, shd, spacing, ind, jc;

    const QString styleId = styleIdForBlock(node);
    if (!styleId.isEmpty())
        pStyle = QStringLiteral("<w:pStyle w:val=\"%1\"/>").arg(escapeXml(styleId));

    if (attrs.value(QStringLiteral("keepWithNext")).toBool(false))
        keepNext = QStringLiteral("<w:keepNext/>");

    const auto runs = node.value(QStringLiteral("content")).toArray();
    const auto dropCap = runs.isEmpty() ? QJsonObject{} :
        runs.first().toObject().value(QStringLiteral("_dropCap")).toObject();
    const QString dropType = dropCap.value(QStringLiteral("type")).toString();
    if (dropType == QLatin1String("drop") || dropType == QLatin1String("margin")) {
        frame = QStringLiteral("<w:framePr w:dropCap=\"%1\" w:lines=\"%2\"/>")
                    .arg(dropType).arg(qBound(1, dropCap.value(QStringLiteral("lines")).toInt(3), 10));
    }

    if (type == QLatin1String("codeBlock"))
        shd = QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"F6F6F6\"/>");

    bool okBefore = false, okAfter = false;
    const int before = cssSizeToTwentieths(attrs.value(QStringLiteral("spaceBefore")).toString(), okBefore);
    const int after  = cssSizeToTwentieths(attrs.value(QStringLiteral("spaceAfter")).toString(),  okAfter);
    if (okBefore || okAfter) {
        spacing = QStringLiteral("<w:spacing");
        if (okBefore) spacing += QStringLiteral(" w:before=\"%1\"").arg(before);
        if (okAfter)  spacing += QStringLiteral(" w:after=\"%1\"").arg(after);
        spacing += QStringLiteral("/>");
    }

    const int indentLevel = attrs.value(QStringLiteral("indent")).toInt(0);
    if (indentLevel > 0)
        ind = QStringLiteral("<w:ind w:left=\"%1\"/>").arg(indentLevel * 720);

    const QString align = jcValue(attrs.value(QStringLiteral("textAlign")).toString());
    if (!align.isEmpty())
        jc = QStringLiteral("<w:jc w:val=\"%1\"/>").arg(align);

    const QString pPr = pStyle + keepNext + frame + numberingXml + pBdr + shd + spacing + ind + jc + sectionXml;
    return pPr.isEmpty() ? QString() : QStringLiteral("<w:pPr>") + pPr + QStringLiteral("</w:pPr>");
}

QString renderBlocks(Package& pkg, const QJsonArray& content);

// ── Lists ──────────────────────────────────────────────────────────────────
// Allocates a fresh abstractNum + num pair for one list instance and returns
// the numId to reference from <w:numPr>. `start` honours orderedList.attrs.start.
QString allocateNumbering(Package& pkg, bool ordered, int start)
{
    const int abstractId = pkg.nextAbstractNumId++;
    const int numId      = pkg.nextNumId++;

    QString levels;
    for (int i = 0; i < 9; ++i) {
        const QString numFmt  = ordered ? QStringLiteral("decimal") : QStringLiteral("bullet");
        // U+F0B7 is the Symbol-font bullet Word itself writes.
        const QString lvlText = ordered ? QStringLiteral("%1.").arg(i + 1)
                                        : QString::fromUtf16(u"\uF0B7");
        const QString rPr = ordered
            ? QString()
            : QStringLiteral("<w:rPr><w:rFonts w:ascii=\"Symbol\" w:hAnsi=\"Symbol\" w:hint=\"default\"/></w:rPr>");
        // Only level 0 restarts at the caller's start value.
        const int startValue = (i == 0 && ordered && start > 0) ? start : 1;

        levels += QStringLiteral(
            "<w:lvl w:ilvl=\"%1\"><w:start w:val=\"%2\"/><w:numFmt w:val=\"%3\"/>"
            "<w:lvlText w:val=\"%4\"/><w:lvlJc w:val=\"left\"/>"
            "<w:pPr><w:ind w:left=\"%5\" w:hanging=\"360\"/></w:pPr>%6</w:lvl>")
            .arg(i).arg(startValue).arg(numFmt, escapeXml(lvlText))
            .arg(720 * (i + 1)).arg(rPr);
    }

    pkg.abstractNums += QStringLiteral(
        "<w:abstractNum w:abstractNumId=\"%1\"><w:multiLevelType w:val=\"hybridMultilevel\"/>%2</w:abstractNum>")
        .arg(abstractId).arg(levels);
    pkg.nums += QStringLiteral("<w:num w:numId=\"%1\"><w:abstractNumId w:val=\"%2\"/></w:num>")
        .arg(numId).arg(abstractId);

    return QString::number(numId);
}

QString numPrXml(int depth, const QString& numId)
{
    return QStringLiteral("<w:numPr><w:ilvl w:val=\"%1\"/><w:numId w:val=\"%2\"/></w:numPr>")
        .arg(depth).arg(numId);
}

QString renderList(Package& pkg, const QJsonObject& list, int depth,
                   const QString& inheritedNumId = QString())
{
    const QString type = list.value(QStringLiteral("type")).toString();
    const bool ordered = (type == QLatin1String("orderedList"));
    const bool task    = (type == QLatin1String("taskList"));

    const QJsonObject listAttrs = list.value(QStringLiteral("attrs")).toObject();
    // A nested list continues its parent's numbering at a deeper ilvl rather
    // than starting an independent list.
    const QString numId = inheritedNumId.isEmpty()
        ? allocateNumbering(pkg, ordered, listAttrs.value(QStringLiteral("start")).toInt(1))
        : inheritedNumId;
    const int ilvl = qMin(depth, 8);

    QString out;
    const QJsonArray items = list.value(QStringLiteral("content")).toArray();
    for (const auto& item : items) {
        if (!item.isObject()) continue;
        const QJsonObject li = item.toObject();
        const QJsonArray children = li.value(QStringLiteral("content")).toArray();

        bool wroteAny = false;
        for (const auto& ch : children) {
            if (!ch.isObject()) continue;
            const QJsonObject child = ch.toObject();
            const QString childType = child.value(QStringLiteral("type")).toString();

            if (childType == QLatin1String("bulletList") || childType == QLatin1String("orderedList")
                || childType == QLatin1String("taskList")) {
                out += renderList(pkg, child, depth + 1, numId);
                wroteAny = true;
                continue;
            }

            QString inner = renderInlines(pkg, child.value(QStringLiteral("content")).toArray());
            if (task || childType == QLatin1String("taskItem")) {
                const bool checked = li.value(QStringLiteral("attrs")).toObject()
                                         .value(QStringLiteral("checked")).toBool(false)
                                     || child.value(QStringLiteral("attrs")).toObject()
                                         .value(QStringLiteral("checked")).toBool(false);
                inner = QStringLiteral("<w:r><w:t xml:space=\"preserve\">%1 </w:t></w:r>")
                            .arg(checked ? QLatin1String("[x]") : QLatin1String("[ ]")) + inner;
            }
            out += QStringLiteral("<w:p>") + paragraphProperties(child, numPrXml(ilvl, numId)) + inner
                 + QStringLiteral("</w:p>");
            wroteAny = true;
        }
        if (!wroteAny) {
            out += QStringLiteral("<w:p><w:pPr>") + numPrXml(ilvl, numId)
                 + QStringLiteral("</w:pPr></w:p>");
        }
    }
    return out;
}

// ── Tables ─────────────────────────────────────────────────────────────────
// Emits <w:tcPr> children in CT_TcPr schema order:
//   tcW, gridSpan, vMerge, shd, vAlign
QString renderTable(Package& pkg, const QJsonObject& table)
{
    const QJsonArray rows = table.value(QStringLiteral("content")).toArray();
    if (rows.isEmpty()) return {};

    int colCount = 0;
    for (const auto& r : rows) {
        if (r.isObject())
            colCount = qMax(colCount, r.toObject().value(QStringLiteral("content")).toArray().size());
    }
    if (colCount == 0) return {};

    const QJsonObject attrs = table.value(QStringLiteral("attrs")).toObject();
    const int attrWidth = attrs.value(QStringLiteral("width")).toInt(0);
    int tableWidth = attrWidth > 0 ? twipsFromPx(attrWidth) : kDefaultTableW;

    // TipTap records user-resized columns in tableCell.attrs.colwidth (px, one
    // entry per spanned column). Dividing the table evenly would throw those
    // widths away, so harvest them and only fall back to an even split for
    // columns nobody sized.
    QList<int>  colWidths(colCount, tableWidth / colCount);
    QList<bool> colSized(colCount, false);
    for (const auto& r : rows) {
        if (!r.isObject()) continue;
        int col = 0;
        const QJsonArray cells = r.toObject().value(QStringLiteral("content")).toArray();
        for (const auto& c : cells) {
            if (!c.isObject()) continue;
            const QJsonObject cellAttrs = c.toObject().value(QStringLiteral("attrs")).toObject();
            const QJsonArray cw = cellAttrs.value(QStringLiteral("colwidth")).toArray();
            for (int i = 0; i < cw.size() && col + i < colCount; ++i) {
                const int px = cw.at(i).toInt(0);
                if (px > 0 && !colSized[col + i]) {
                    colWidths[col + i] = twipsFromPx(px);
                    colSized[col + i]  = true;
                }
            }
            col += qMax(1, cellAttrs.value(QStringLiteral("colspan")).toInt(1));
        }
    }
    if (colSized.contains(true)) {
        tableWidth = 0;
        for (int w : colWidths) tableWidth += w;
    }

    QString grid;
    for (int i = 0; i < colCount; ++i)
        grid += QStringLiteral("<w:gridCol w:w=\"%1\"/>").arg(colWidths[i]);

    const QString tblPr = QStringLiteral(
        "<w:tblPr><w:tblW w:w=\"%1\" w:type=\"dxa\"/>"
        "<w:tblBorders>"
        "<w:top w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>"
        "<w:left w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>"
        "<w:bottom w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>"
        "<w:right w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>"
        "<w:insideH w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>"
        "<w:insideV w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>"
        "</w:tblBorders>"
        "<w:tblLayout w:type=\"fixed\"/>"
        "<w:tblLook w:val=\"04A0\" w:firstRow=\"1\" w:lastRow=\"0\" w:firstColumn=\"1\" "
        "w:lastColumn=\"0\" w:noHBand=\"0\" w:noVBand=\"1\"/></w:tblPr>").arg(tableWidth);

    QString body;
    bool firstRow = true;
    for (const auto& r : rows) {
        if (!r.isObject()) continue;
        const QJsonArray cells = r.toObject().value(QStringLiteral("content")).toArray();
        if (cells.isEmpty()) continue;

        QString rowXml;
        bool rowIsHeader = false;
        int  col = 0;
        for (const auto& c : cells) {
            if (!c.isObject()) continue;
            const QJsonObject cell = c.toObject();
            const bool isHeader = cell.value(QStringLiteral("type")).toString()
                                  == QLatin1String("tableHeader");
            if (isHeader) rowIsHeader = true;

            const QJsonObject cellAttrs = cell.value(QStringLiteral("attrs")).toObject();
            const int colspan = qMax(1, cellAttrs.value(QStringLiteral("colspan")).toInt(1));
            const int rowspan = qMax(1, cellAttrs.value(QStringLiteral("rowspan")).toInt(1));

            // A merged cell spans several grid columns, so its width is their sum.
            int spanWidth = 0;
            for (int i = col; i < qMin(col + colspan, colCount); ++i)
                spanWidth += colWidths[i];
            col += colspan;

            QString tcPr = QStringLiteral("<w:tcW w:w=\"%1\" w:type=\"dxa\"/>").arg(spanWidth);
            if (colspan > 1) tcPr += QStringLiteral("<w:gridSpan w:val=\"%1\"/>").arg(colspan);
            if (rowspan > 1) tcPr += QStringLiteral("<w:vMerge w:val=\"restart\"/>");

            QString shd;
            if (isHeader) {
                shd = QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"F2F2F2\"/>");
            } else {
                const QString hex = sixDigitHex(cellAttrs.value(QStringLiteral("background")).toString());
                if (!hex.isEmpty())
                    shd = QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"%1\"/>").arg(hex);
            }
            tcPr += shd;

            const QString vAlign = cellAttrs.value(QStringLiteral("verticalAlign")).toString();
            if (!vAlign.isEmpty())
                tcPr += QStringLiteral("<w:vAlign w:val=\"%1\"/>").arg(escapeXml(vAlign));

            const QString cellContent = renderBlocks(pkg, cell.value(QStringLiteral("content")).toArray());
            rowXml += QStringLiteral("<w:tc><w:tcPr>") + tcPr + QStringLiteral("</w:tcPr>")
                    + (cellContent.isEmpty() ? QStringLiteral("<w:p/>") : cellContent)
                    + QStringLiteral("</w:tc>");
        }
        if (rowXml.isEmpty()) continue;

        const QString trPr = (firstRow && rowIsHeader)
            ? QStringLiteral("<w:trPr><w:tblHeader/></w:trPr>") : QString();
        body += QStringLiteral("<w:tr>") + trPr + rowXml + QStringLiteral("</w:tr>");
        firstRow = false;
    }
    if (body.isEmpty()) return {};

    // OOXML forbids a table as the last block-level element; a trailing empty
    // paragraph keeps Word from offering to repair the file.
    return QStringLiteral("<w:tbl>") + tblPr + QStringLiteral("<w:tblGrid>") + grid
         + QStringLiteral("</w:tblGrid>") + body + QStringLiteral("</w:tbl><w:p/>");
}

// ── Blocks ─────────────────────────────────────────────────────────────────
QString renderBlock(Package& pkg, const QJsonObject& node)
{
    const QString type = node.value(QStringLiteral("type")).toString();

    if (type == QLatin1String("bulletList") || type == QLatin1String("orderedList")
        || type == QLatin1String("taskList")) {
        return renderList(pkg, node, 0);
    }
    if (type == QLatin1String("table"))  return renderTable(pkg, node);
    if (type == QLatin1String("listItem") || type == QLatin1String("taskItem")) {
        return renderBlocks(pkg, node.value(QStringLiteral("content")).toArray());
    }
    if (type == QLatin1String("horizontalRule")) {
        return QStringLiteral(
            "<w:p><w:pPr><w:pBdr>"
            "<w:bottom w:val=\"single\" w:sz=\"6\" w:space=\"1\" w:color=\"auto\"/>"
            "</w:pBdr></w:pPr></w:p>");
    }
    if (type == QLatin1String("image")) {
        const QString drawing = renderImage(pkg, node.value(QStringLiteral("attrs")).toObject());
        return drawing.isEmpty() ? QString()
                                 : QStringLiteral("<w:p>") + drawing + QStringLiteral("</w:p>");
    }

    const QString inner = renderInlines(pkg, node.value(QStringLiteral("content")).toArray());
    const QString pPr = paragraphProperties(node, QString());
    if (inner.isEmpty() && pPr.isEmpty()) return QStringLiteral("<w:p/>");
    return QStringLiteral("<w:p>") + pPr + inner + QStringLiteral("</w:p>");
}

QString renderBlocks(Package& pkg, const QJsonArray& content)
{
    QString out;
    for (const auto& c : content) {
        if (c.isObject()) out += renderBlock(pkg, c.toObject());
    }
    return out;
}

// ── Sections ───────────────────────────────────────────────────────────────
// CT_SectPr order: type, pgSz, pgMar, paperSrc, pgBorders, lnNumType, pgNumType,
// cols, formProt, vAlign, noEndnote, titlePg, textDirection, bidi, rtlGutter,
// docGrid, printerSettings.
QString headerFooterReference(Package& pkg, const QString& kind, const QString& text)
{
    if (text.isEmpty()) return {};
    const QString tag = kind == QLatin1String("header") ? QStringLiteral("hdr") : QStringLiteral("ftr");
    int index = 1;
    while (pkg.headerFooterParts.contains(kind + QString::number(index) + QStringLiteral(".xml"))) ++index;
    const QString part = kind + QString::number(index) + QStringLiteral(".xml");
    pkg.headerFooterParts.insert(part, kind);
    QString content;
    static const QRegularExpression token(QStringLiteral("\\{(PAGE|NUMPAGES)\\}"));
    const auto run = [](const QString& literal) {
        return QStringLiteral("<w:r><w:t xml:space=\"preserve\">%1</w:t></w:r>").arg(escapeXml(literal));
    };
    const QStringList paragraphs = text.split(QLatin1Char('\n'));
    for (const QString& paragraph : paragraphs) {
        content += QStringLiteral("<w:p>");
        auto matches = token.globalMatch(paragraph);
        qsizetype offset = 0;
        while (matches.hasNext()) {
            const auto match = matches.next();
            content += run(paragraph.mid(offset, match.capturedStart() - offset));
            content += QStringLiteral("<w:fldSimple w:instr=\"%1\">%2</w:fldSimple>")
                           .arg(match.captured(1), run(QStringLiteral("1")));
            offset = match.capturedEnd();
        }
        content += run(paragraph.mid(offset)) + QStringLiteral("</w:p>");
    }
    pkg.entries.insert(QStringLiteral("word/") + part, QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<w:%1 xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">%2</w:%1>")
        .arg(tag, content).toUtf8());
    const QString id = addRelationship(pkg, kind, part);
    return QStringLiteral("<w:%1Reference w:type=\"default\" r:id=\"%2\"/>").arg(kind, id);
}

QString sectionProperties(Package& pkg, QJsonObject sec, const QJsonObject& layout = {})
{
    QString out;
    for (const auto& kind : {QStringLiteral("header"), QStringLiteral("footer")}) {
        const auto key = kind + QStringLiteral("Text");
        const auto text = layout.value(key).toString(sec.value(key).toString());
        out += headerFooterReference(pkg, kind, text);
    }

    if (!layout.isEmpty()) {
        const QString size = layout.value(QStringLiteral("size")).toString();
        const bool landscape = layout.value(QStringLiteral("orientation")).toString() == QLatin1String("landscape");
        QSize dims(11906, 16838);
        if (size == QLatin1String("Letter")) dims = QSize(12240, 15840);
        if (size == QLatin1String("Legal")) dims = QSize(12240, 20160);
        if (size == QLatin1String("Tabloid")) dims = QSize(15840, 24480);
        if (landscape) dims.transpose();
        sec.insert(QStringLiteral("orient"), landscape ? 1 : 0);
        sec.insert(QStringLiteral("widthTwips"), dims.width());
        sec.insert(QStringLiteral("heightTwips"), dims.height());
        const QStringList sides{QStringLiteral("Top"), QStringLiteral("Right"),
                                QStringLiteral("Bottom"), QStringLiteral("Left")};
        for (const auto& side : sides) {
            const double inches = layout.value(QStringLiteral("margin") + side).toDouble(1);
            sec.insert(side.toLower(), qRound(qBound(0.0, inches, 5.0) * 1440));
        }
        sec.insert(QStringLiteral("titlePg"), layout.value(QStringLiteral("differentFirstPage")).toBool());
    }
    const QString type = sec.value(QStringLiteral("type")).toString();
    if (!type.isEmpty()) out += QStringLiteral("<w:type w:val=\"%1\"/>").arg(escapeXml(type));

    const int orient = sec.value(QStringLiteral("orient")).toInt(0);
    const int pgW = sec.value(QStringLiteral("widthTwips")).toInt(orient == 1 ? 15840 : 12240);
    const int pgH = sec.value(QStringLiteral("heightTwips")).toInt(orient == 1 ? 12240 : 15840);
    out += QStringLiteral("<w:pgSz w:w=\"%1\" w:h=\"%2\"%3/>")
        .arg(pgW).arg(pgH)
        .arg(orient == 1 ? QStringLiteral(" w:orient=\"landscape\"") : QString());

    out += QStringLiteral(
        "<w:pgMar w:top=\"%1\" w:right=\"%2\" w:bottom=\"%3\" w:left=\"%4\" "
        "w:header=\"%5\" w:footer=\"%6\" w:gutter=\"%7\"/>")
        .arg(sec.value(QStringLiteral("top")).toInt(1440))
        .arg(sec.value(QStringLiteral("right")).toInt(1440))
        .arg(sec.value(QStringLiteral("bottom")).toInt(1440))
        .arg(sec.value(QStringLiteral("left")).toInt(1440))
        .arg(sec.value(QStringLiteral("header")).toInt(720))
        .arg(sec.value(QStringLiteral("footer")).toInt(720))
        .arg(sec.value(QStringLiteral("gutter")).toInt(0));

    // PageStyle.cpp produces ready-made fragments for these.
    const QString lnNum = sec.value(QStringLiteral("lnNum")).toString();
    if (!lnNum.isEmpty()) out += lnNum;
    if (!layout.isEmpty() || sec.contains(QStringLiteral("pageNumberStartAt")) || sec.contains(QStringLiteral("pageNumberFormat"))) {
        const auto format = layout.value(QStringLiteral("pageNumberFormat")).toString(sec.value(QStringLiteral("pageNumberFormat")).toString());
        const QStringList supported{QStringLiteral("decimal"), QStringLiteral("upperRoman"),
            QStringLiteral("lowerRoman"), QStringLiteral("upperLetter"), QStringLiteral("lowerLetter")};
        QString wordFormat = format;
        if (wordFormat == QLatin1String("upperAlpha")) wordFormat = QStringLiteral("upperLetter");
        if (wordFormat == QLatin1String("lowerAlpha")) wordFormat = QStringLiteral("lowerLetter");
        if (!supported.contains(wordFormat)) wordFormat = QStringLiteral("decimal");
        out += QStringLiteral("<w:pgNumType w:fmt=\"%1\"").arg(wordFormat);
        if (layout.contains(QStringLiteral("pageNumberStartAt")) || sec.contains(QStringLiteral("pageNumberStartAt"))) {
            const int start = layout.value(QStringLiteral("pageNumberStartAt")).toInt(sec.value(QStringLiteral("pageNumberStartAt")).toInt(1));
            out += QStringLiteral(" w:start=\"%1\"").arg(qMax(1, start));
        }
        out += QStringLiteral("/>");
    }
    const QString cols = sec.value(QStringLiteral("cols")).toString();
    if (!cols.isEmpty()) out += cols;
    if (sec.value(QStringLiteral("titlePg")).toBool(false)) out += QStringLiteral("<w:titlePg/>");

    return QStringLiteral("<w:sectPr>") + out + QStringLiteral("</w:sectPr>");
}

// ── Static package parts ───────────────────────────────────────────────────
// Built after the document walk, since renderList() allocates the entries.
// Returns {} when the document contains no lists, so the part is omitted.
QString buildNumberingXml(const Package& pkg)
{
    if (pkg.abstractNums.isEmpty()) return {};

    return QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<w:numbering xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">")
        + pkg.abstractNums.join(QString())
        + pkg.nums.join(QString())
        + QStringLiteral("</w:numbering>\n");
}

QString buildStylesXml()
{
    QString entries;
    entries += QStringLiteral(
        "<w:docDefaults><w:rPrDefault><w:rPr>"
        "<w:rFonts w:ascii=\"Calibri\" w:hAnsi=\"Calibri\" w:cs=\"Calibri\"/>"
        "<w:sz w:val=\"22\"/><w:szCs w:val=\"22\"/></w:rPr></w:rPrDefault>"
        "<w:pPrDefault><w:pPr><w:spacing w:after=\"160\" w:line=\"259\" w:lineRule=\"auto\"/></w:pPr></w:pPrDefault>"
        "</w:docDefaults>");
    entries += QStringLiteral(
        "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\">"
        "<w:name w:val=\"Normal\"/><w:qFormat/></w:style>");

    // CT_Style order: name, aliases, basedOn, next, link, autoRedefine, hidden,
    // uiPriority, semiHidden, unhideWhenUsed, qFormat, locked, personal, …, pPr, rPr
    const int headingHalfPoints[] = {64, 56, 48, 44, 40, 36};
    for (int i = 1; i <= 6; ++i) {
        entries += QStringLiteral(
            "<w:style w:type=\"paragraph\" w:styleId=\"Heading%1\">"
            "<w:name w:val=\"heading %1\"/><w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/>"
            "<w:uiPriority w:val=\"9\"/><w:qFormat/>"
            "<w:pPr><w:keepNext/><w:spacing w:before=\"%2\" w:after=\"80\"/>"
            "<w:outlineLvl w:val=\"%3\"/></w:pPr>"
            "<w:rPr><w:b/><w:sz w:val=\"%4\"/><w:szCs w:val=\"%4\"/></w:rPr></w:style>")
            .arg(i).arg(240 + (6 - i) * 40).arg(i - 1).arg(headingHalfPoints[i - 1]);
    }
    entries += QStringLiteral(
        "<w:style w:type=\"paragraph\" w:styleId=\"Title\"><w:name w:val=\"Title\"/>"
        "<w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:uiPriority w:val=\"10\"/><w:qFormat/>"
        "<w:pPr><w:spacing w:after=\"120\"/></w:pPr>"
        "<w:rPr><w:b/><w:sz w:val=\"56\"/><w:szCs w:val=\"56\"/></w:rPr></w:style>");
    entries += QStringLiteral(
        "<w:style w:type=\"paragraph\" w:styleId=\"Subtitle\"><w:name w:val=\"Subtitle\"/>"
        "<w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:uiPriority w:val=\"11\"/><w:qFormat/>"
        "<w:pPr><w:spacing w:after=\"240\"/></w:pPr>"
        "<w:rPr><w:color w:val=\"595959\"/><w:sz w:val=\"32\"/><w:szCs w:val=\"32\"/></w:rPr></w:style>");
    entries += QStringLiteral(
        "<w:style w:type=\"paragraph\" w:styleId=\"Quote\"><w:name w:val=\"Quote\"/>"
        "<w:basedOn w:val=\"Normal\"/><w:next w:val=\"Normal\"/><w:uiPriority w:val=\"29\"/><w:qFormat/>"
        "<w:pPr><w:ind w:left=\"720\"/></w:pPr>"
        "<w:rPr><w:i/><w:color w:val=\"404040\"/></w:rPr></w:style>");
    entries += QStringLiteral(
        "<w:style w:type=\"character\" w:styleId=\"Hyperlink\"><w:name w:val=\"Hyperlink\"/>"
        "<w:uiPriority w:val=\"99\"/>"
        "<w:rPr><w:color w:val=\"0563C1\"/><w:u w:val=\"single\"/></w:rPr></w:style>");

    return QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">\n")
        + entries + QStringLiteral("</w:styles>\n");
}

QString buildSettingsXml(const QJsonObject& body)
{
    const QJsonObject vela = body.value(QStringLiteral("_vela")).toObject();
    // CT_Settings order puts zoom/defaultTabStop/compat early; documentProtection
    // must follow them.
    QString bits = QStringLiteral(
        "<w:zoom w:percent=\"100\"/><w:defaultTabStop w:val=\"720\"/>"
        "<w:characterSpacingControl w:val=\"doNotCompress\"/>");
    if (vela.contains(QStringLiteral("documentProtection")))
        bits += vela.value(QStringLiteral("documentProtection")).toString();
    if (vela.value(QStringLiteral("evenAndOddHeaders")).toBool(false))
        bits += QStringLiteral("<w:evenAndOddHeaders/>");
    if (vela.value(QStringLiteral("mirrorMargins")).toBool(false))
        bits += QStringLiteral("<w:mirrorMargins/>");
    bits += QStringLiteral(
        "<w:compat><w:compatSetting w:name=\"compatibilityMode\" "
        "w:uri=\"http://schemas.microsoft.com/office/word\" w:val=\"15\"/></w:compat>");

    return QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<w:settings xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">\n")
        + bits + QStringLiteral("\n</w:settings>\n");
}

QString buildContentTypes(const Package& pkg, bool hasNumbering)
{
    QString defaults = QStringLiteral(
        "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
        "  <Default Extension=\"xml\" ContentType=\"application/xml\"/>\n"
        "  <Default Extension=\"json\" ContentType=\"application/json\"/>\n");

    for (const QString& ext : pkg.mediaExtensions) {
        QString mime;
        if      (ext == QLatin1String("png")) mime = QStringLiteral("image/png");
        else if (ext == QLatin1String("jpg")) mime = QStringLiteral("image/jpeg");
        else if (ext == QLatin1String("gif")) mime = QStringLiteral("image/gif");
        else if (ext == QLatin1String("bmp")) mime = QStringLiteral("image/bmp");
        else if (ext == QLatin1String("tif") || ext == QLatin1String("tiff")) mime = QStringLiteral("image/tiff");
        else if (ext == QLatin1String("svg")) mime = QStringLiteral("image/svg+xml");
        else if (ext == QLatin1String("webp")) mime = QStringLiteral("image/webp");
        else mime = QStringLiteral("application/octet-stream");
        defaults += QStringLiteral("  <Default Extension=\"%1\" ContentType=\"%2\"/>\n")
                        .arg(escapeXml(ext), mime);
    }

    QString overrides = QStringLiteral(
        "  <Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>\n"
        "  <Override PartName=\"/word/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>\n"
        "  <Override PartName=\"/word/settings.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml\"/>\n");
    if (hasNumbering) {
        overrides += QStringLiteral(
            "  <Override PartName=\"/word/numbering.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml\"/>\n");
    }
    for (auto it = pkg.headerFooterParts.constBegin(); it != pkg.headerFooterParts.constEnd(); ++it) {
        overrides += QStringLiteral(
            "  <Override PartName=\"/word/%1\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.%2+xml\"/>\n")
            .arg(it.key(), it.value());
    }

    return QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n")
        + defaults + overrides + QStringLiteral("</Types>\n");
}

} // anon namespace

bool DocxExporter::exportToFile(const QString& path, const QJsonObject& body, QString& outError)
{
    Package pkg;
    const QJsonObject vela = body.value(QStringLiteral("_vela")).toObject();

    // Walk first: this allocates relationship ids and registers media parts.
    QString content = renderBlocks(pkg, body.value(QStringLiteral("content")).toArray());

    // OOXML allows exactly one body-level <w:sectPr> — the final section.
    // Earlier sections must ride inside a paragraph's <w:pPr>. The body JSON
    // carries no break positions, so prior sections are emitted as leading
    // section-break paragraphs; every definition still survives the round trip.
    const QJsonArray sections = vela.value(QStringLiteral("sections")).toArray();
    const auto layout = vela.value(QStringLiteral("pageSettings")).toObject();
    if (sections.isEmpty()) {
        if (!layout.isEmpty()) content += sectionProperties(pkg, {}, layout);
    } else {
        QString breaks;
        for (int i = 0; i + 1 < sections.size(); ++i) {
            breaks += QStringLiteral("<w:p><w:pPr>")
                    + sectionProperties(pkg, sections[i].toObject())
                    + QStringLiteral("</w:pPr></w:p>");
        }
        content = breaks + content
                + sectionProperties(pkg, sections.last().toObject(), layout);
    }

    const QString documentXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<w:document "
        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
        "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\">\n"
        "<w:body>")
        + content + QStringLiteral("</w:body>\n</w:document>\n");

    // numbering.xml is derived from what the walk allocated, so it must be
    // built after renderBlocks() above.
    const QString numberingXml = buildNumberingXml(pkg);
    const bool hasNumbering = !numberingXml.isEmpty();

    pkg.entries.insert(QStringLiteral("[Content_Types].xml"),
                       buildContentTypes(pkg, hasNumbering).toUtf8());
    pkg.entries.insert(QStringLiteral("_rels/.rels"), QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "  <Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>\n"
        "</Relationships>\n").toUtf8());
    pkg.entries.insert(QStringLiteral("word/document.xml"), documentXml.toUtf8());
    pkg.entries.insert(QStringLiteral("word/styles.xml"), buildStylesXml().toUtf8());
    pkg.entries.insert(QStringLiteral("word/settings.xml"), buildSettingsXml(body).toUtf8());
    if (hasNumbering)
        pkg.entries.insert(QStringLiteral("word/numbering.xml"), numberingXml.toUtf8());

    // Fixed parts use non-numeric ids so they cannot collide with the rId1..N
    // sequence the document walk allocated for hyperlinks and images.
    QString wordRels = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "  <Relationship Id=\"rIdStyles\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>\n"
        "  <Relationship Id=\"rIdSettings\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings\" Target=\"settings.xml\"/>\n");
    if (hasNumbering) {
        wordRels += QStringLiteral(
            "  <Relationship Id=\"rIdNumbering\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering\" Target=\"numbering.xml\"/>\n");
    }
    for (const QString& rel : pkg.relationships) wordRels += rel;
    wordRels += QStringLiteral("</Relationships>\n");
    pkg.entries.insert(QStringLiteral("word/_rels/document.xml.rels"), wordRels.toUtf8());

    // Vela sidecar: page styles, section metadata and anything else with no
    // OOXML home. Word ignores unknown parts, so this is lossless for us and
    // harmless for everyone else.
    if (!vela.isEmpty()) {
        pkg.entries.insert(QStringLiteral("word/_vela.json"),
                           QJsonDocument(vela).toJson(QJsonDocument::Compact));
    }

    if (!Zip::write(path, pkg.entries)) {
        outError = QStringLiteral("Failed to write zip: %1").arg(path);
        return false;
    }

    VE_LOG_INFO("DocxExporter: wrote " << pkg.entries.size() << " parts to " << path);
    return true;
}

} // namespace vela
