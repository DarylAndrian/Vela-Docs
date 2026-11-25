// ════════════════════════════════════════════════════════════════════════════
//  PageStyle.cpp — implementations for PageStyleSheet + the four
//                  per-section setting blocks (LineNumberSettings,
//                  ColumnSettings, DropCapSettings, DocumentProtection).
//
//  All DOCX output here targets the OOXML schema documented in
//  [MS-DOCX] / ECMA-376 Part 1, §17.6 (sections + page setup) and §17.15
//  (protection + settings).
// ════════════════════════════════════════════════════════════════════════════
#include "document/PageStyle.h"

#include <QDomDocument>
#include <QRegularExpression>

#include <algorithm>

namespace vela {

// ══════════════════════════════════════════════════════════════════════════
//  XML safety
// ══════════════════════════════════════════════════════════════════════════
//
// The literal '&' character cannot appear in this source file (it would
// be parsed as the start of an XML/HTML entity by tooling), so we construct
// it via char concatenation when emitting entities.

namespace {
constexpr QChar kAmp = QChar(0x0026);
QString amp() { return QString(kAmp); }

QString xmlEsc(const QString& s)
{
    QString out;
    out.reserve(s.size());
    for (QChar c : s) {
        switch (c.unicode()) {
        case '&':  out += amp() + QStringLiteral("amp;");  break;
        case '<':  out += amp() + QStringLiteral("lt;");   break;
        case '>':  out += amp() + QStringLiteral("gt;");   break;
        case '"':  out += amp() + QStringLiteral("quot;"); break;
        case '\'': out += amp() + QStringLiteral("apos;"); break;
        default:   out += c;
        }
    }
    return out;
}

// Helper for non-equal-width columns: round-robin distribute leftover space.
int splitColumnWidth(int /*idx*/, int totalCols, int available, int space)
{
    if (totalCols <= 0) return 0;
    if (available <= 0) return 4320;   // 3" safe default
    const int totalGap = space * (totalCols - 1);
    return (available - totalGap) / totalCols;
}

QString breakTag(PageSectionBreak k)
{
    switch (k) {
    case PageSectionBreak::NextPage:    return QStringLiteral("nextPage");
    case PageSectionBreak::Continuous:  return QStringLiteral("continuous");
    case PageSectionBreak::EvenPage:    return QStringLiteral("evenPage");
    case PageSectionBreak::OddPage:     return QStringLiteral("oddPage");
    }
    return QStringLiteral("nextPage");
}

// ─── Attribute helpers that survive escape-character quirks ───────────────
//
// QString::arg() lets us build regex patterns without quoting headaches.

QString makeLnNumTypeRegex()
{
    QString p;
    p += QStringLiteral("<w:lnNumType[^>]*w:countBy=\"([0-9]+)\"");
    p += QStringLiteral("[^>]*w:start=\"([0-9]+)\"");
    p += QStringLiteral("[^>]*w:distance=\"([0-9]+)\"");
    p += QStringLiteral("[^>]*w:restart=\"([A-Za-z]+)\"");
    p += QStringLiteral("[^>]*/>");
    return p;
}

QString makeColsHeaderRegex()
{
    QString p;
    p += QStringLiteral("<w:cols[^>]*w:num=\"([0-9]+)\"");
    p += QStringLiteral("[^>]*w:space=\"([0-9]+)\"");
    return p;
}
} // anon namespace

// ══════════════════════════════════════════════════════════════════════════
//  LineNumberSettings
// ══════════════════════════════════════════════════════════════════════════

QString LineNumberSettings::toXml() const
{
    if (!enabled) return {};
    const QString restart = continuous        ? QStringLiteral("continuous")
                          : restartPerSection ? QStringLiteral("newSection")
                          : restartPerPage    ? QStringLiteral("newPage")
                                              : QStringLiteral("continuous");
    return QStringLiteral("<w:lnNumType w:countBy=\"")        + QString::number(countBy)
         + QStringLiteral("\" w:start=\"")                  + QString::number(startAt)
         + QStringLiteral("\" w:distance=\"")               + QString::number(distance)
         + QStringLiteral("\" w:restart=\"")                + restart
         + QStringLiteral("\"/>");
}

bool LineNumberSettings::fromXml(const QString& xml, LineNumberSettings& out)
{
    if (xml.isEmpty()) return false;
    static const QRegularExpression re(
        makeLnNumTypeRegex(),
        QRegularExpression::DotMatchesEverythingOption);
    auto m = re.match(xml);
    if (!m.hasMatch()) {
        // Permissive fallback: assume default first-line numbering if any
        // <w:lnNumType> tag is present but attributes don't match.
        if (xml.contains(QStringLiteral("<w:lnNumType"))) {
            out.enabled = true;
            out.countBy = 1; out.startAt = 1; out.distance = 360;
            out.restartPerPage = true; out.continuous = false;
            return true;
        }
        return false;
    }
    out.enabled           = true;
    out.countBy           = m.captured(1).toInt();
    out.startAt           = m.captured(2).toInt();
    out.distance          = m.captured(3).toInt();
    const QString r       = m.captured(4);
    out.continuous        = (r == QLatin1String("continuous"));
    out.restartPerSection = (r == QLatin1String("newSection"));
    out.restartPerPage    = (r == QLatin1String("newPage"));
    return true;
}

// ══════════════════════════════════════════════════════════════════════════
//  ColumnSettings
// ══════════════════════════════════════════════════════════════════════════

QString ColumnSettings::toXml() const
{
    if (count <= 1) return {};
    if (equalWidth || customWidths.isEmpty()) {
        return QStringLiteral("<w:cols w:num=\"")      + QString::number(count)
             + QStringLiteral("\" w:space=\"")         + QString::number(space)
             + QStringLiteral("\" w:equalWidth=\"")    + (equalWidth ? QStringLiteral("1") : QStringLiteral("0"))
             + QStringLiteral("\" w:sep=\"")           + (separator ? QStringLiteral("1") : QStringLiteral("0"))
             + QStringLiteral("\"/>");
    }
    QStringList cols;
    for (int i = 0; i < count; ++i) {
        const int w = i < customWidths.size()
                    ? customWidths[i]
                    : splitColumnWidth(i, count, /*available*/ 12240 - 2880, space);
        cols << QStringLiteral("<w:col w:w=\"") + QString::number(w)
             + QStringLiteral("\" w:space=\"")  + QString::number(i + 1 < count ? space : 0)
             + QStringLiteral("\"/>");
    }
    return QStringLiteral("<w:cols w:num=\"")        + QString::number(count)
         + QStringLiteral("\" w:space=\"")          + QString::number(space)
         + QStringLiteral("\" w:equalWidth=\"0\" w:sep=\"")
         + (separator ? QStringLiteral("1") : QStringLiteral("0"))
         + QStringLiteral("\">")
         + cols.join(QString())
         + QStringLiteral("</w:cols>");
}

bool ColumnSettings::fromXml(const QString& xml, ColumnSettings& out)
{
    if (xml.isEmpty()) return false;
    static const QRegularExpression reNum(
        makeColsHeaderRegex(),
        QRegularExpression::DotMatchesEverythingOption);
    auto m = reNum.match(xml);
    if (!m.hasMatch()) return false;
    out.count     = m.captured(1).toInt();
    out.space     = m.captured(2).toInt();
    out.equalWidth = !xml.contains(QStringLiteral("w:equalWidth=\"0\""));
    out.separator  = xml.contains(QStringLiteral("w:sep=\"1\""));
    out.customWidths.clear();
    return true;
}

// ══════════════════════════════════════════════════════════════════════════
//  DropCapSettings
// ══════════════════════════════════════════════════════════════════════════

QString DropCapSettings::toXml() const
{
    if (type == Type::None) return {};
    const QString kind = (type == Type::Drop) ? QStringLiteral("drop") : QStringLiteral("margin");
    QString out = QStringLiteral("<w:dropCap w:val=\"") + kind
                + QStringLiteral("\" w:lines=\"")      + QString::number(lines)
                + QStringLiteral("\"");
    if (!font.isEmpty()) out += QStringLiteral(" w:font=\"") + xmlEsc(font) + QStringLiteral("\"");
    out += QStringLiteral("/>");
    return out;
}

bool DropCapSettings::fromXml(const QString& xml, DropCapSettings& out)
{
    if (xml.isEmpty()) return false;
    if      (xml.contains(QStringLiteral("w:val=\"drop\"")))   out.type = Type::Drop;
    else if (xml.contains(QStringLiteral("w:val=\"margin\""))) out.type = Type::Margin;
    else out.type = Type::None;
    static const QRegularExpression reLines(QStringLiteral("w:lines=\"([0-9]+)\""));
    auto m = reLines.match(xml);
    if (m.hasMatch()) out.lines = m.captured(1).toInt();
    return out.type != Type::None;
}

// ══════════════════════════════════════════════════════════════════════════
//  DocumentProtection
// ══════════════════════════════════════════════════════════════════════════

QString DocumentProtection::labelFor(Edit e)
{
    switch (e) {
    case Edit::NoProtection:   return QStringLiteral("No protection");
    case Edit::ReadOnly:       return QStringLiteral("Read-only");
    case Edit::Comments:       return QStringLiteral("Comments only");
    case Edit::TrackedChanges: return QStringLiteral("Tracked changes only");
    case Edit::Forms:          return QStringLiteral("Filling in forms");
    }
    return QStringLiteral("No protection");
}

QString DocumentProtection::docxValueFor(Edit e)
{
    switch (e) {
    case Edit::NoProtection:   return {};
    case Edit::ReadOnly:       return QStringLiteral("readOnly");
    case Edit::Comments:       return QStringLiteral("comments");
    case Edit::TrackedChanges: return QStringLiteral("trackedChanges");
    case Edit::Forms:          return QStringLiteral("forms");
    }
    return {};
}

DocumentProtection::Edit DocumentProtection::fromDocxValue(const QString& v)
{
    if (v == QLatin1String("readOnly"))       return Edit::ReadOnly;
    if (v == QLatin1String("comments"))       return Edit::Comments;
    if (v == QLatin1String("trackedChanges")) return Edit::TrackedChanges;
    if (v == QLatin1String("forms"))          return Edit::Forms;
    return Edit::NoProtection;
}

QString DocumentProtection::toXml() const
{
    if (mode == Edit::NoProtection || !enforce) return {};
    QString out = QStringLiteral("<w:documentProtection w:edit=\"")
                + docxValueFor(mode)
                + QStringLiteral("\" w:enforcement=\"1\"");
    if (!passwordHash.isEmpty()) {
        out += QStringLiteral(" w:cryptProviderType=\"rsaAES\"")
            +  QStringLiteral(" w:cryptAlgorithmClass=\"hash\"")
            +  QStringLiteral(" w:cryptAlgorithmType=\"typeAny\"")
            +  QStringLiteral(" w:cryptAlgorithmSid=\"14\"")
            +  QStringLiteral(" w:cryptSpinCount=\"") + QString::number(spinCount) + QStringLiteral("\"")
            +  QStringLiteral(" w:hash=\"")  + xmlEsc(passwordHash) + QStringLiteral("\"")
            +  QStringLiteral(" w:salt=\"") + xmlEsc(salt)          + QStringLiteral("\"");
        if (!algorithmicIdExt.isEmpty()) {
            out += QStringLiteral(" w:algorithmName=\"") + xmlEsc(algorithmicIdExt) + QStringLiteral("\"");
        }
    }
    out += QStringLiteral("/>");
    return out;
}

bool DocumentProtection::fromXml(const QString& xml, DocumentProtection& out)
{
    if (xml.isEmpty()) return false;
    static const QRegularExpression reEdit(QStringLiteral("w:edit=\"([A-Za-z]+)\""));
    auto m = reEdit.match(xml);
    if (!m.hasMatch()) { out.mode = Edit::NoProtection; out.enforce = false; return false; }
    out.mode    = fromDocxValue(m.captured(1));
    out.enforce = xml.contains(QStringLiteral("w:enforcement=\"1\""));
    if (out.mode == Edit::NoProtection && !out.enforce) return true;
    static const QRegularExpression reHash(QStringLiteral("w:hash=\"([^\"]+)\""));
    static const QRegularExpression reSalt(QStringLiteral("w:salt=\"([^\"]+)\""));
    static const QRegularExpression reAlg(QStringLiteral("w:cryptAlgorithmSid=\"([0-9]+)\""));
    auto mh = reHash.match(xml);
    auto ms = reSalt.match(xml);
    if (mh.hasMatch()) out.passwordHash = mh.captured(1);
    if (ms.hasMatch()) out.salt          = ms.captured(1);
    auto ma = reAlg.match(xml);
    if (ma.hasMatch()) out.algorithmicIdExt = ma.captured(1);
    return true;
}

// ══════════════════════════════════════════════════════════════════════════
//  PageStyle + PageStyleSheet
// ══════════════════════════════════════════════════════════════════════════

QString PageStyle::toXmlStyleEntry() const
{
    const bool landscape = orientation == PageOrientation::Landscape;
    const int w = landscape ? qMax(widthTwips, heightTwips) : widthTwips;
    const int h = landscape ? qMin(widthTwips, heightTwips) : heightTwips;
    const QString orientAttr = (orientation == PageOrientation::Landscape)
                              ? QStringLiteral(" w:orient=\"landscape\"")
                              : QString();

    QString pPr;
    pPr += QStringLiteral("<w:pgSz w:w=\"")  + QString::number(w)
         + QStringLiteral("\" w:h=\"")       + QString::number(h)
         + QStringLiteral("\"") + orientAttr + QStringLiteral("/>");
    pPr += QStringLiteral("<w:pgMar w:top=\"")      + QString::number(marginTop)
         + QStringLiteral("\" w:right=\"")         + QString::number(marginRight)
         + QStringLiteral("\" w:bottom=\"")        + QString::number(marginBottom)
         + QStringLiteral("\" w:left=\"")          + QString::number(marginLeft)
         + QStringLiteral("\" w:header=\"")        + QString::number(headerOffset)
         + QStringLiteral("\" w:footer=\"")        + QString::number(footerOffset)
         + QStringLiteral("\" w:gutter=\"")        + QString::number(gutter)
         + QStringLiteral("\"/>");
    if (mirrorMargins) pPr += QStringLiteral("<w:mirrorMargins/>");
    if (titlePg)       pPr += QStringLiteral("<w:titlePg/>");
    const QString colsXml  = columns.toXml();
    const QString linesXml = lineNumbers.toXml();
    if (!colsXml.isEmpty())  pPr += colsXml;
    if (!linesXml.isEmpty()) pPr += linesXml;
    if (!headerRef.isEmpty())     pPr += QStringLiteral("<w:headerReference w:type=\"default\" r:id=\"") + headerRef + QStringLiteral("\"/>");
    if (!footerRef.isEmpty())     pPr += QStringLiteral("<w:footerReference w:type=\"default\" r:id=\"") + footerRef + QStringLiteral("\"/>");
    if (!headerFirstRef.isEmpty())pPr += QStringLiteral("<w:headerReference w:type=\"first\"   r:id=\"") + headerFirstRef + QStringLiteral("\"/>");
    if (!footerFirstRef.isEmpty())pPr += QStringLiteral("<w:footerReference w:type=\"first\"   r:id=\"") + footerFirstRef + QStringLiteral("\"/>");
    if (!headerEvenRef.isEmpty()) pPr += QStringLiteral("<w:headerReference w:type=\"even\"    r:id=\"") + headerEvenRef + QStringLiteral("\"/>");
    if (!footerEvenRef.isEmpty()) pPr += QStringLiteral("<w:footerReference w:type=\"even\"    r:id=\"") + footerEvenRef + QStringLiteral("\"/>");

    QString result = QStringLiteral("<w:style w:type=\"page\" w:styleId=\"")
                   + xmlEsc(id) + (builtin ? QStringLiteral("\">") : QStringLiteral("\" w:customStyle=\"1\">"));
    result += QStringLiteral("<w:name w:val=\"") + xmlEsc(name) + QStringLiteral("\"/>");
    result += QStringLiteral("<w:basedOn w:val=\"PageStyleDefault\"/>");
    result += QStringLiteral("<w:pPr>") + pPr + QStringLiteral("</w:pPr>");
    result += QStringLiteral("</w:style>");
    return result;
}

PageStyleSheet::PageStyleSheet()
{
    seedBuiltin();
}

void PageStyleSheet::seedBuiltin()
{
    m_builtinOrder = {
        QStringLiteral("PageStyleDefault"),
        QStringLiteral("PageStyleLandscape"),
        QStringLiteral("PageStyleTitlePage")
    };

    if (!m_styles.contains(QStringLiteral("PageStyleDefault"))) {
        PageStyle def;
        def.id      = QStringLiteral("PageStyleDefault");
        def.name    = QStringLiteral("Default");
        def.builtin = true;
        m_styles.insert(def.id, def);
    }

    if (!m_styles.contains(QStringLiteral("PageStyleLandscape"))) {
        PageStyle ls;
        ls.id           = QStringLiteral("PageStyleLandscape");
        ls.name         = QStringLiteral("Landscape");
        ls.orientation  = PageOrientation::Landscape;
        ls.widthTwips   = 15840;   // 11"
        ls.heightTwips  = 12240;   // 8.5"
        ls.builtin      = true;
        m_styles.insert(ls.id, ls);
    }

    if (!m_styles.contains(QStringLiteral("PageStyleTitlePage"))) {
        PageStyle tp;
        tp.id                  = QStringLiteral("PageStyleTitlePage");
        tp.name                = QStringLiteral("Title Page");
        tp.differentFirstPage  = true;  // mirrors <w:titlePg/>
        tp.titlePg             = true;
        tp.marginTop           = 2160;   // 1.5"
        tp.marginBottom        = 2160;
        tp.builtin             = true;
        m_styles.insert(tp.id, tp);
    }
}

void PageStyleSheet::registerStyle(const PageStyle& def)
{
    if (def.id.isEmpty()) return;
    PageStyle merged = def;
    if (m_styles.value(def.id).builtin) merged.builtin = true;
    m_styles.insert(def.id, merged);
    if (!m_builtinOrder.contains(def.id)) m_builtinOrder.append(def.id);
}

bool PageStyleSheet::unregisterStyle(const QString& id)
{
    auto it = m_styles.constFind(id);
    if (it == m_styles.constEnd()) return false;
    if (it.value().builtin) return false;
    m_styles.erase(it);
    m_builtinOrder.removeAll(id);
    return true;
}

PageStyle PageStyleSheet::get(const QString& id) const
{
    return m_styles.value(id);
}

QList<PageStyle> PageStyleSheet::list() const
{
    QList<PageStyle> out;
    for (const auto& id : m_builtinOrder) {
        if (m_styles.contains(id)) out.append(m_styles.value(id));
    }
    QList<PageStyle> custom;
    for (auto it = m_styles.constBegin(); it != m_styles.constEnd(); ++it) {
        if (!it.value().builtin) custom.append(it.value());
    }
    std::sort(custom.begin(), custom.end(),
              [](const PageStyle& a, const PageStyle& b) { return a.name < b.name; });
    out.append(custom);
    return out;
}

QStringList PageStyleSheet::toDocxFragments() const
{
    QStringList out;
    for (const auto& p : m_styles) out.append(p.toXmlStyleEntry());
    return out;
}

namespace {

/// Extract the `<w:cols …>…</w:cols>` substring (or empty).
QString findSubXml(const QString& src, const QString& tag)
{
    const QString open = QStringLiteral("<") + tag;
    int idx = src.indexOf(open);
    if (idx < 0) return {};
    int end = src.indexOf(QStringLiteral("/>"), idx);
    if (end >= 0) return src.mid(idx, end + 2 - idx);
    end = src.indexOf(QStringLiteral("</") + tag + QStringLiteral(">"), idx);
    if (end < 0) return {};
    return src.mid(idx, end + 3 + tag.size() - idx);
}

} // anon

void PageStyleSheet::fromDocxFragment(const QString& xml)
{
    if (xml.isEmpty()) return;
    QDomDocument d;
    const QString wrapped = QStringLiteral(
        "<styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">%1</styles>").arg(xml);
    if (!d.setContent(wrapped)) return;
    const QDomElement style = d.documentElement().firstChildElement();
    if (style.tagName() != QLatin1String("w:style") && style.tagName() != QLatin1String("style")) return;
    if (style.attribute(QStringLiteral("w:type")) != QLatin1String("page")) return;

    PageStyle p;
    p.id      = style.attribute(QStringLiteral("w:styleId"));
    p.name    = style.firstChildElement(QStringLiteral("w:name")).attribute(QStringLiteral("w:val"));
    p.builtin = style.attribute(QStringLiteral("w:customStyle")) != QLatin1String("1");
    if (p.id.isEmpty()) return;

    const QDomElement pPr = style.firstChildElement(QStringLiteral("w:pPr"));
    if (pPr.isNull()) { m_styles.insert(p.id, p); return; }

    const QDomElement pgSz = pPr.firstChildElement(QStringLiteral("w:pgSz"));
    if (!pgSz.isNull()) {
        p.widthTwips  = pgSz.attribute(QStringLiteral("w:w")).toInt();
        p.heightTwips = pgSz.attribute(QStringLiteral("w:h")).toInt();
        if (pgSz.attribute(QStringLiteral("w:orient")) == QLatin1String("landscape")) {
            p.orientation = PageOrientation::Landscape;
        }
    }

    const QDomElement pgMar = pPr.firstChildElement(QStringLiteral("w:pgMar"));
    if (!pgMar.isNull()) {
        p.marginTop    = pgMar.attribute(QStringLiteral("w:top")).toInt();
        p.marginRight  = pgMar.attribute(QStringLiteral("w:right")).toInt();
        p.marginBottom = pgMar.attribute(QStringLiteral("w:bottom")).toInt();
        p.marginLeft   = pgMar.attribute(QStringLiteral("w:left")).toInt();
        p.headerOffset = pgMar.attribute(QStringLiteral("w:header")).toInt();
        p.footerOffset = pgMar.attribute(QStringLiteral("w:footer")).toInt();
        p.gutter       = pgMar.attribute(QStringLiteral("w:gutter")).toInt();
    }

    if (!pPr.firstChildElement(QStringLiteral("w:mirrorMargins")).isNull()) p.mirrorMargins = true;
    if (!pPr.firstChildElement(QStringLiteral("w:titlePg")).isNull())       p.titlePg       = true;
    p.differentFirstPage = p.titlePg;

    const QString colsXml = findSubXml(xml, QStringLiteral("w:cols"));
    ColumnSettings cols;
    if (ColumnSettings::fromXml(colsXml, cols)) p.columns = cols;

    const QString linesXml = findSubXml(xml, QStringLiteral("w:lnNumType"));
    LineNumberSettings ls;
    if (LineNumberSettings::fromXml(linesXml, ls)) p.lineNumbers = ls;

    m_styles.insert(p.id, p);
}

QString PageStyleSheet::escapeXml(const QString& s) const
{
    return xmlEsc(s);
}

// ══════════════════════════════════════════════════════════════════════════
//  SectionStack
// ══════════════════════════════════════════════════════════════════════════

void SectionStack::append(const SectionData& s)
{
    m_sections.append(s);
}

void SectionStack::replace(int i, const SectionData& s)
{
    if (i >= 0 && i < m_sections.size()) m_sections[i] = s;
}

QString SectionStack::serializeForDocx(const QString& /*defaultPageStyleId*/) const
{
    QString out;
    for (int i = 0; i < m_sections.size(); ++i) {
        const SectionData& s = m_sections[i];
        const int pgW = (s.orientation == PageOrientation::Landscape) ? 15840 : 12240;
        const int pgH = (s.orientation == PageOrientation::Landscape) ? 12240 : 15840;
        QString one;
        one += QStringLiteral("<w:type w:val=\"") + breakTag(s.breakKind) + QStringLiteral("\"/>");
        one += QStringLiteral("<w:pgSz w:w=\"")  + QString::number(pgW)
             + QStringLiteral("\" w:h=\"")       + QString::number(pgH)
             + QStringLiteral("\"/>");
        one += QStringLiteral("<w:pgMar w:top=\"")    + QString::number(s.marginTop)
             + QStringLiteral("\" w:right=\"")       + QString::number(s.marginRight)
             + QStringLiteral("\" w:bottom=\"")      + QString::number(s.marginBottom)
             + QStringLiteral("\" w:left=\"")        + QString::number(s.marginLeft)
             + QStringLiteral("\" w:header=\"")      + QString::number(s.headerOffset)
             + QStringLiteral("\" w:footer=\"")      + QString::number(s.footerOffset)
             + QStringLiteral("\" w:gutter=\"")      + QString::number(s.gutter)
             + QStringLiteral("\"/>");
        const QString colsXml  = s.columns.toXml();
        const QString linesXml = s.lineNumbers.toXml();
        if (!colsXml.isEmpty())  one += colsXml;
        if (!linesXml.isEmpty()) one += linesXml;
        if (s.titlePg) one += QStringLiteral("<w:titlePg/>");
        one += QStringLiteral("<w:rsidR w:val=\"00000000\"/>");
        out += QStringLiteral("<w:sectPr>") + one + QStringLiteral("</w:sectPr>");
    }
    return out;
}

} // namespace vela
