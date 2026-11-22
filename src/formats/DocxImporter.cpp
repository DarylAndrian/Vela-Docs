// ════════════════════════════════════════════════════════════════════════════
//  DocxImporter.cpp — Phase 1 implementation (real OOXML parser)
//                     v0.5.0: reads page styles, sections, columns, line
//                     numbers, drop caps, document protection back into
//                     the body JSON's "_vela" sidecar block.
// ════════════════════════════════════════════════════════════════════════════

#include "formats/DocxImporter.h"
#include "util/Logger.h"
#include "util/Zip.h"

#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace vela {

DocxImporter::DocxImporter(QObject* parent)
    : QObject(parent)
{
}

namespace {

QString localName(const QString& qualifiedName)
{
    const int colon = qualifiedName.indexOf(QLatin1Char(':'));
    return colon < 0 ? qualifiedName : qualifiedName.mid(colon + 1);
}

QString readElementText(QXmlStreamReader& xml)
{
    QString result;
    while (!xml.atEnd()) {
        const auto tt = xml.readNext();
        if (xml.isStartElement()) {
            const QString name = localName(xml.name().toString());
            if (name == QLatin1String("t") || name == QLatin1String("tab")) {
                if (name == QLatin1String("tab")) result += QLatin1Char('\t');
            }
        } else if (xml.isCharacters()) {
            result += xml.text().toString();
        } else if (xml.isEndElement()) {
            break;
        }
    }
    return result;
}

// rId -> external target, so <w:hyperlink r:id="…"> can be resolved to an href.
using RelTargets = QHash<QString, QString>;

QJsonObject makeMark(const QString& type, const QJsonObject& attrs = QJsonObject())
{
    QJsonObject mark;
    mark.insert(QStringLiteral("type"), type);
    if (!attrs.isEmpty()) mark.insert(QStringLiteral("attrs"), attrs);
    return mark;
}

QJsonObject parseRun(QXmlStreamReader& xml, const RelTargets& rels,
                     const QString& inheritedLinkHref = QString())
{
    QJsonArray marks;
    QString text;

    // Character formatting collected from <w:rPr>, folded into a single
    // textStyle mark at the end (that is how TipTap represents it).
    QString fontFamily, fontSize, color, highlight;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QString name = localName(xml.name().toString());
            if (name == QLatin1String("rPr")) {
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isStartElement()) {
                        const QString m = localName(xml.name().toString());
                        const auto attrs = xml.attributes();
                        if      (m == QLatin1String("b"))       marks.append(makeMark(QStringLiteral("bold")));
                        else if (m == QLatin1String("i"))       marks.append(makeMark(QStringLiteral("italic")));
                        else if (m == QLatin1String("strike"))  marks.append(makeMark(QStringLiteral("strike")));
                        else if (m == QLatin1String("u")) {
                            QJsonObject u;
                            if (attrs.value(QStringLiteral("w:val")).toString() == QLatin1String("double"))
                                u.insert(QStringLiteral("decoration"), QStringLiteral("double"));
                            marks.append(makeMark(QStringLiteral("underline"), u));
                        }
                        else if (m == QLatin1String("rFonts"))    fontFamily = attrs.value(QStringLiteral("w:ascii")).toString();
                        else if (m == QLatin1String("sz"))        fontSize   = attrs.value(QStringLiteral("w:val")).toString();
                        else if (m == QLatin1String("color"))     color      = attrs.value(QStringLiteral("w:val")).toString();
                        else if (m == QLatin1String("highlight")) highlight  = attrs.value(QStringLiteral("w:val")).toString();
                        else if (m == QLatin1String("shd")) {
                            // The exporter writes hex highlights as run shading.
                            const QString fill = attrs.value(QStringLiteral("w:fill")).toString();
                            if (!fill.isEmpty() && fill != QLatin1String("auto")) highlight = fill;
                        }
                        xml.skipCurrentElement();
                    } else if (xml.isEndElement()
                            && localName(xml.name().toString()) == QLatin1String("rPr")) {
                        break;
                    }
                }
            } else if (name == QLatin1String("t")) {
                text += xml.readElementText();
            } else if (name == QLatin1String("tab")) {
                text += QLatin1Char('\t');
            } else if (name == QLatin1String("br")) {
                const QString brType = xml.attributes().value(QStringLiteral("w:type")).toString();
                if (brType == QLatin1String("page")) {
                    // A page break is its own node; flush the text run first.
                    xml.skipCurrentElement();
                    continue;
                }
                text += QLatin1Char('\n');
            }
        } else if (xml.isEndElement()
                   && localName(xml.name().toString()) == QLatin1String("r")) {
            break;
        }
    }

    QJsonObject textStyleAttrs;
    if (!fontFamily.isEmpty()) textStyleAttrs.insert(QStringLiteral("fontFamily"), fontFamily);
    if (!fontSize.isEmpty()) {
        bool ok = false;
        const int halfPoints = fontSize.toInt(&ok);
        if (ok && halfPoints > 0)
            textStyleAttrs.insert(QStringLiteral("fontSize"), QStringLiteral("%1pt").arg(halfPoints / 2.0));
    }
    if (!color.isEmpty() && color != QLatin1String("auto"))
        textStyleAttrs.insert(QStringLiteral("color"), QStringLiteral("#%1").arg(color.toLower()));
    if (!textStyleAttrs.isEmpty()) marks.append(makeMark(QStringLiteral("textStyle"), textStyleAttrs));

    if (!highlight.isEmpty()) {
        QJsonObject h;
        h.insert(QStringLiteral("color"),
                 highlight.startsWith(u'#') ? highlight : QStringLiteral("#%1").arg(highlight.toLower()));
        marks.append(makeMark(QStringLiteral("highlight"), h));
    }

    const QString href = inheritedLinkHref.isEmpty() ? QString() : inheritedLinkHref;
    if (!href.isEmpty()) {
        QJsonObject l;
        l.insert(QStringLiteral("href"), href);
        marks.append(makeMark(QStringLiteral("link"), l));
    }

    QJsonObject run;
    run.insert(QStringLiteral("type"), QStringLiteral("text"));
    run.insert(QStringLiteral("text"), text);
    if (!marks.isEmpty()) run.insert(QStringLiteral("marks"), marks);
    return run;
}

// Maps an OOXML paragraph style id onto the ProseMirror node the editor uses.
QJsonObject blockNodeForStyle(const QString& styleId)
{
    QJsonObject node;
    if (styleId.startsWith(QLatin1String("Heading"))) {
        bool ok = false;
        const int level = styleId.mid(7).toInt(&ok);
        if (ok && level >= 1 && level <= 6) {
            node.insert(QStringLiteral("type"), QStringLiteral("heading"));
            QJsonObject attrs;
            attrs.insert(QStringLiteral("level"), level);
            node.insert(QStringLiteral("attrs"), attrs);
            return node;
        }
    }
    if (styleId == QLatin1String("Title"))    { node.insert(QStringLiteral("type"), QStringLiteral("title"));    return node; }
    if (styleId == QLatin1String("Subtitle")) { node.insert(QStringLiteral("type"), QStringLiteral("subtitle")); return node; }
    if (styleId == QLatin1String("Quote"))    { node.insert(QStringLiteral("type"), QStringLiteral("blockquote"));return node; }

    node.insert(QStringLiteral("type"), QStringLiteral("paragraph"));
    if (!styleId.isEmpty()) {
        QJsonObject attrs;
        attrs.insert(QStringLiteral("customStyle"), styleId);
        node.insert(QStringLiteral("attrs"), attrs);
    }
    return node;
}

QString jcToTextAlign(const QString& jc)
{
    if (jc == QLatin1String("center")) return QStringLiteral("center");
    if (jc == QLatin1String("right"))  return QStringLiteral("right");
    if (jc == QLatin1String("both"))   return QStringLiteral("justify");
    if (jc == QLatin1String("left"))   return QStringLiteral("left");
    return {};
}

QJsonObject parseParagraph(QXmlStreamReader& xml, const RelTargets& rels)
{
    QJsonArray content;
    QString styleId;
    QString textAlign;
    int numId = 0, ilvl = 0;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QString name = localName(xml.name().toString());
            if (name == QLatin1String("pPr")) {
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isStartElement()) {
                        const QString s = localName(xml.name().toString());
                        const auto attrs = xml.attributes();
                        if (s == QLatin1String("pStyle")) {
                            styleId = attrs.value(QStringLiteral("w:val")).toString();
                        } else if (s == QLatin1String("jc")) {
                            textAlign = jcToTextAlign(attrs.value(QStringLiteral("w:val")).toString());
                        } else if (s == QLatin1String("numPr")) {
                            while (!xml.atEnd()) {
                                xml.readNext();
                                if (xml.isStartElement()) {
                                    const QString n = localName(xml.name().toString());
                                    const QString v = xml.attributes().value(QStringLiteral("w:val")).toString();
                                    if      (n == QLatin1String("numId")) numId = v.toInt();
                                    else if (n == QLatin1String("ilvl"))  ilvl  = v.toInt();
                                    xml.skipCurrentElement();
                                } else if (xml.isEndElement()
                                        && localName(xml.name().toString()) == QLatin1String("numPr")) {
                                    break;
                                }
                            }
                            continue;
                        } else if (s == QLatin1String("sectPr")) {
                            // A paragraph-embedded section break carries no content.
                            xml.skipCurrentElement();
                            continue;
                        }
                        xml.skipCurrentElement();
                    } else if (xml.isEndElement()
                            && localName(xml.name().toString()) == QLatin1String("pPr")) {
                        break;
                    }
                }
            } else if (name == QLatin1String("r")) {
                content.append(parseRun(xml, rels));
            } else if (name == QLatin1String("hyperlink")) {
                const QString relId = xml.attributes().value(QStringLiteral("r:id")).toString();
                const QString href  = rels.value(relId);
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isStartElement()
                        && localName(xml.name().toString()) == QLatin1String("r")) {
                        content.append(parseRun(xml, rels, href));
                    } else if (xml.isEndElement()
                            && localName(xml.name().toString()) == QLatin1String("hyperlink")) {
                        break;
                    }
                }
            } else {
                xml.skipCurrentElement();
            }
        } else if (xml.isEndElement()
                   && localName(xml.name().toString()) == QLatin1String("p")) {
            break;
        }
    }

    QJsonObject para = blockNodeForStyle(styleId);
    QJsonObject attrs = para.value(QStringLiteral("attrs")).toObject();
    if (!textAlign.isEmpty()) attrs.insert(QStringLiteral("textAlign"), textAlign);
    if (numId > 0) {
        attrs.insert(QStringLiteral("_numId"), numId);
        attrs.insert(QStringLiteral("_ilvl"),  ilvl);
    }
    if (!attrs.isEmpty()) para.insert(QStringLiteral("attrs"), attrs);
    if (!content.isEmpty()) para.insert(QStringLiteral("content"), content);
    return para;
}

// ── Tables ─────────────────────────────────────────────────────────────────
QJsonObject parseTable(QXmlStreamReader& xml, const RelTargets& rels)
{
    QJsonArray rows;
    QList<int> gridTwips;   // per-column widths from <w:tblGrid>, in twentieths of a point
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QString name = localName(xml.name().toString());
            if (name == QLatin1String("tblGrid")) {
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isStartElement()) {
                        if (localName(xml.name().toString()) == QLatin1String("gridCol"))
                            gridTwips.append(xml.attributes().value(QStringLiteral("w:w")).toInt());
                        xml.skipCurrentElement();
                    } else if (xml.isEndElement()
                            && localName(xml.name().toString()) == QLatin1String("tblGrid")) {
                        break;
                    }
                }
            } else if (name == QLatin1String("tr")) {
                QJsonArray cells;
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isStartElement()) {
                        const QString cellName = localName(xml.name().toString());
                        if (cellName == QLatin1String("tc")) {
                            QJsonArray cellContent;
                            int colspan = 1, rowspan = 1;
                            bool sawVMergeContinue = false;
                            while (!xml.atEnd()) {
                                xml.readNext();
                                if (xml.isStartElement()) {
                                    const QString inCell = localName(xml.name().toString());
                                    if (inCell == QLatin1String("p")) {
                                        cellContent.append(parseParagraph(xml, rels));
                                    } else if (inCell == QLatin1String("tcPr")) {
                                        while (!xml.atEnd()) {
                                            xml.readNext();
                                            if (xml.isStartElement()) {
                                                const QString p = localName(xml.name().toString());
                                                const auto a = xml.attributes();
                                                if (p == QLatin1String("gridSpan"))
                                                    colspan = a.value(QStringLiteral("w:val")).toInt();
                                                else if (p == QLatin1String("vMerge")) {
                                                    // val="restart" opens a span; a bare
                                                    // <w:vMerge/> continues one.
                                                    if (a.value(QStringLiteral("w:val")).toString()
                                                        != QLatin1String("restart"))
                                                        sawVMergeContinue = true;
                                                }
                                                xml.skipCurrentElement();
                                            } else if (xml.isEndElement()
                                                    && localName(xml.name().toString()) == QLatin1String("tcPr")) {
                                                break;
                                            }
                                        }
                                    } else {
                                        xml.skipCurrentElement();
                                    }
                                } else if (xml.isEndElement()
                                        && localName(xml.name().toString()) == QLatin1String("tc")) {
                                    break;
                                }
                            }
                            Q_UNUSED(rowspan);
                            if (sawVMergeContinue) continue;   // merged into the cell above

                            QJsonObject cellAttrs;
                            cellAttrs.insert(QStringLiteral("colspan"), colspan);
                            cellAttrs.insert(QStringLiteral("rowspan"), 1);

                            QJsonObject cell;
                            cell.insert(QStringLiteral("type"), QStringLiteral("tableCell"));
                            cell.insert(QStringLiteral("attrs"), cellAttrs);
                            cell.insert(QStringLiteral("content"), cellContent);
                            cells.append(cell);
                        } else {
                            xml.skipCurrentElement();
                        }
                    } else if (xml.isEndElement()
                            && localName(xml.name().toString()) == QLatin1String("tr")) {
                        break;
                    }
                }
                if (!cells.isEmpty()) {
                    QJsonObject row;
                    row.insert(QStringLiteral("type"), QStringLiteral("tableRow"));
                    row.insert(QStringLiteral("content"), cells);
                    rows.append(row);
                }
            } else {
                xml.skipCurrentElement();
            }
        } else if (xml.isEndElement()
                   && localName(xml.name().toString()) == QLatin1String("tbl")) {
            break;
        }
    }

    // TipTap stores per-column widths as tableCell.attrs.colwidth in px, one
    // entry per spanned column. Without it a resized table re-imports with
    // every column the same width.
    if (!gridTwips.isEmpty()) {
        for (int r = 0; r < rows.size(); ++r) {
            if (!rows[r].isObject()) continue;
            QJsonObject row = rows[r].toObject();
            QJsonArray cells = row.value(QStringLiteral("content")).toArray();
            int col = 0;
            for (int i = 0; i < cells.size(); ++i) {
                QJsonObject cell = cells[i].toObject();
                QJsonObject cellAttrs = cell.value(QStringLiteral("attrs")).toObject();
                const int colspan = qMax(1, cellAttrs.value(QStringLiteral("colspan")).toInt(1));
                QJsonArray colwidth;
                const int last = qMin(col + colspan, static_cast<int>(gridTwips.size()));
                for (int k = col; k < last; ++k)
                    colwidth.append(qMax(1, gridTwips[k] / 15));
                col += colspan;
                if (!colwidth.isEmpty()) {
                    cellAttrs.insert(QStringLiteral("colwidth"), colwidth);
                    cell.insert(QStringLiteral("attrs"), cellAttrs);
                    cells[i] = cell;
                }
            }
            row.insert(QStringLiteral("content"), cells);
            rows[r] = row;
        }
    }

    QJsonObject table;
    table.insert(QStringLiteral("type"), QStringLiteral("table"));
    table.insert(QStringLiteral("content"), rows);
    return table;
}

// numId -> true when that list numbers rather than bullets.
QHash<int, bool> parseNumberingFormats(const QByteArray& numberingXml)
{
    QHash<int, bool> ordered;
    if (numberingXml.isEmpty()) return ordered;

    QHash<int, int> numToAbstract;
    QHash<int, bool> abstractIsOrdered;

    QXmlStreamReader xml(numberingXml);
    int currentAbstract = -1;
    bool abstractDecided = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QString name = localName(xml.name().toString());
            if (name == QLatin1String("abstractNum")) {
                currentAbstract = xml.attributes().value(QStringLiteral("w:abstractNumId")).toString().toInt();
                abstractDecided = false;
            } else if (name == QLatin1String("numFmt") && currentAbstract >= 0 && !abstractDecided) {
                const QString fmt = xml.attributes().value(QStringLiteral("w:val")).toString();
                abstractIsOrdered.insert(currentAbstract, fmt != QLatin1String("bullet"));
                abstractDecided = true;
            } else if (name == QLatin1String("num")) {
                const int numId = xml.attributes().value(QStringLiteral("w:numId")).toString().toInt();
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isStartElement()
                        && localName(xml.name().toString()) == QLatin1String("abstractNumId")) {
                        numToAbstract.insert(numId,
                            xml.attributes().value(QStringLiteral("w:val")).toString().toInt());
                        xml.skipCurrentElement();
                        break;
                    } else if (xml.isEndElement()
                            && localName(xml.name().toString()) == QLatin1String("num")) {
                        break;
                    }
                }
            }
        } else if (xml.isEndElement()
                   && localName(xml.name().toString()) == QLatin1String("abstractNum")) {
            currentAbstract = -1;
        }
    }
    for (auto it = numToAbstract.constBegin(); it != numToAbstract.constEnd(); ++it)
        ordered.insert(it.key(), abstractIsOrdered.value(it.value(), false));
    return ordered;
}

RelTargets parseRelationships(const QByteArray& relsXml)
{
    RelTargets targets;
    if (relsXml.isEmpty()) return targets;
    QXmlStreamReader xml(relsXml);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && localName(xml.name().toString()) == QLatin1String("Relationship")) {
            const auto a = xml.attributes();
            const QString type = a.value(QStringLiteral("Type")).toString();
            if (type.endsWith(QLatin1String("/hyperlink")) || type.endsWith(QLatin1String("/image")))
                targets.insert(a.value(QStringLiteral("Id")).toString(),
                               a.value(QStringLiteral("Target")).toString());
        }
    }
    return targets;
}

// Folds consecutive paragraphs that share a numId into bulletList / orderedList /
// taskList nodes with listItem children — the shape the editor expects.
QJsonArray groupLists(const QJsonArray& flat, const QHash<int, bool>& orderedByNumId)
{
    QJsonArray out;
    QJsonArray pendingItems;
    int pendingNumId = 0;

    auto flush = [&]() {
        if (pendingItems.isEmpty()) return;
        QJsonObject list;
        list.insert(QStringLiteral("type"),
                    orderedByNumId.value(pendingNumId, false)
                        ? QStringLiteral("orderedList") : QStringLiteral("bulletList"));
        list.insert(QStringLiteral("content"), pendingItems);
        out.append(list);
        pendingItems = QJsonArray();
        pendingNumId = 0;
    };

    for (const auto& v : flat) {
        const QJsonObject node = v.toObject();
        const QJsonObject attrs = node.value(QStringLiteral("attrs")).toObject();
        const int numId = attrs.value(QStringLiteral("_numId")).toInt(0);
        if (numId == 0) { flush(); out.append(node); continue; }

        if (numId != pendingNumId) flush();
        pendingNumId = numId;

        // Strip the private numbering keys before they reach the editor schema.
        QJsonObject cleanAttrs = attrs;
        cleanAttrs.remove(QStringLiteral("_numId"));
        cleanAttrs.remove(QStringLiteral("_ilvl"));
        QJsonObject para = node;
        if (cleanAttrs.isEmpty()) para.remove(QStringLiteral("attrs"));
        else                      para.insert(QStringLiteral("attrs"), cleanAttrs);

        QJsonObject item;
        item.insert(QStringLiteral("type"), QStringLiteral("listItem"));
        item.insert(QStringLiteral("content"), QJsonArray{para});
        pendingItems.append(item);
    }
    flush();
    return out;
}

void absorbSections(const QByteArray& docXml, QJsonObject& vela)
{
    // Walk all <w:sectPr> elements (including the one nested in body pPr).
    // Convert each into a JSON entry stored at vela["sections"].
    QXmlStreamReader xml(docXml);
    QJsonArray sections;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QString name = localName(xml.name().toString());
            if (name == QLatin1String("sectPr")) {
                QJsonObject s;
                QString cols;
                QString lines;
                QString titlePg;
                QString type;
                bool titlePgSeen = false;
                while (!xml.atEnd()) {
                    xml.readNext();
                    if (xml.isStartElement()) {
                        const QString ln = localName(xml.name().toString());
                        const auto a = xml.attributes();
                        if (ln == QLatin1String("pgSz")) {
                            const int w = a.value(QStringLiteral("w:w")).toInt();
                            const int h = a.value(QStringLiteral("w:h")).toInt();
                            s.insert(QStringLiteral("orient"), (w > h) ? 1 : 0);
                        } else if (ln == QLatin1String("pgMar")) {
                            s.insert(QStringLiteral("top"),    a.value(QStringLiteral("w:top")).toInt());
                            s.insert(QStringLiteral("right"),  a.value(QStringLiteral("w:right")).toInt());
                            s.insert(QStringLiteral("bottom"), a.value(QStringLiteral("w:bottom")).toInt());
                            s.insert(QStringLiteral("left"),   a.value(QStringLiteral("w:left")).toInt());
                            s.insert(QStringLiteral("header"), a.value(QStringLiteral("w:header")).toInt());
                            s.insert(QStringLiteral("footer"), a.value(QStringLiteral("w:footer")).toInt());
                            s.insert(QStringLiteral("gutter"), a.value(QStringLiteral("w:gutter")).toInt());
                        } else if (ln == QLatin1String("type")) {
                            type = a.value(QStringLiteral("w:val")).toString();
                        } else if (ln == QLatin1String("titlePg")) {
                            titlePgSeen = true;
                            titlePg = QStringLiteral("<w:titlePg/>");
                        } else if (ln == QLatin1String("cols")) {
                            // Rebuild from the real attributes — reading
                            // a.value(QString()) always yields "" and silently
                            // dropped every column setting.
                            QString attrs;
                            for (const char* name : {"w:num", "w:space", "w:equalWidth", "w:sep"}) {
                                const QString value = a.value(QLatin1String(name)).toString();
                                if (!value.isEmpty())
                                    attrs += QStringLiteral(" %1=\"%2\"").arg(QLatin1String(name), value);
                            }
                            cols = QStringLiteral("<w:cols%1/>").arg(attrs);
                        } else if (ln == QLatin1String("lnNumType")) {
                            QString attrs;
                            for (const char* name : {"w:countBy", "w:start", "w:distance", "w:restart"}) {
                                const QString value = a.value(QLatin1String(name)).toString();
                                if (!value.isEmpty())
                                    attrs += QStringLiteral(" %1=\"%2\"").arg(QLatin1String(name), value);
                            }
                            lines = QStringLiteral("<w:lnNumType%1/>").arg(attrs);
                        }
                        xml.skipCurrentElement();
                    } else if (xml.isEndElement()
                            && localName(xml.name().toString()) == QLatin1String("sectPr")) {
                        break;
                    }
                }
                if (!type.isEmpty())    s.insert(QStringLiteral("type"), type);
                if (!titlePgSeen)        s.insert(QStringLiteral("titlePg"), false);
                if (!cols.isEmpty())     s.insert(QStringLiteral("cols"), cols);
                if (!lines.isEmpty())    s.insert(QStringLiteral("lnNum"), lines);
                sections.append(s);
            } else if (name == QLatin1String("tbl")) {
                xml.skipCurrentElement();
            }
        }
    }
    if (!sections.isEmpty()) vela.insert(QStringLiteral("sections"), sections);
}

void absorbSettings(const QByteArray& settingsXml, QJsonObject& vela)
{
    if (settingsXml.isEmpty()) return;
    // QByteArray::contains needs a byte literal — QString→QByteArray conversion
    // is disabled by QT_NO_CAST_TO_ASCII.
    if (settingsXml.contains(QByteArrayLiteral("<w:evenAndOddHeaders/>"))) {
        vela.insert(QStringLiteral("evenAndOddHeaders"), true);
    }
    if (settingsXml.contains(QByteArrayLiteral("<w:mirrorMargins/>"))) {
        vela.insert(QStringLiteral("mirrorMargins"), true);
    }
    static const QRegularExpression reProt(
        QStringLiteral("<w:documentProtection[^/]*/>"));
    auto m = reProt.match(QString::fromUtf8(settingsXml));
    if (m.hasMatch()) {
        vela.insert(QStringLiteral("documentProtection"), m.captured(0));
    }
}

} // anon namespace

bool DocxImporter::importFromFile(const QString& path, QJsonObject& outBody, QString& outError)
{
    QHash<QString, QByteArray> entries;
    if (!Zip::openForReading(path, entries)) {
        outError = QStringLiteral("Could not open .docx: not a valid zip archive");
        return false;
    }
    if (!entries.contains(QStringLiteral("word/document.xml"))) {
        outError = QStringLiteral("Missing word/document.xml — not a valid .docx");
        return false;
    }
    const QByteArray docXml = entries.value(QStringLiteral("word/document.xml"));
    const RelTargets rels = parseRelationships(
        entries.value(QStringLiteral("word/_rels/document.xml.rels")));

    QXmlStreamReader xml(docXml);
    QJsonArray flat;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QString name = localName(xml.name().toString());
            if (name == QLatin1String("p")) {
                flat.append(parseParagraph(xml, rels));
            } else if (name == QLatin1String("tbl")) {
                flat.append(parseTable(xml, rels));
            } else if (name == QLatin1String("sectPr")) {
                xml.skipCurrentElement();
            }
        }
    }
    if (xml.hasError() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        outError = QStringLiteral("XML parse error: %1").arg(xml.errorString());
        VE_LOG_WARNING(outError);
    }

    const QHash<int, bool> orderedByNumId = parseNumberingFormats(
        entries.value(QStringLiteral("word/numbering.xml")));
    const QJsonArray body = groupLists(flat, orderedByNumId);

    outBody = QJsonObject();
    outBody.insert(QStringLiteral("type"),    QStringLiteral("doc"));
    outBody.insert(QStringLiteral("content"), body);

    // The exporter round-trips Vela metadata verbatim in word/_vela.json. Fall
    // back to reconstructing it from the OOXML parts for foreign documents.
    QJsonObject vela;
    if (entries.contains(QStringLiteral("word/_vela.json"))) {
        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(
            entries.value(QStringLiteral("word/_vela.json")), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            vela = doc.object();
        } else {
            VE_LOG_WARNING(QStringLiteral("DocxImporter: ignoring unreadable word/_vela.json: %1")
                               .arg(parseError.errorString()));
        }
    }
    if (vela.isEmpty()) {
        absorbSections(docXml, vela);
        if (entries.contains(QStringLiteral("word/settings.xml")))
            absorbSettings(entries.value(QStringLiteral("word/settings.xml")), vela);
    }
    if (!vela.isEmpty())
        outBody.insert(QStringLiteral("_vela"), vela);

    VE_LOG_INFO("DocxImporter: imported "
                << body.size() << " top-level blocks from " << path);
    return true;
}

} // namespace vela
