// ════════════════════════════════════════════════════════════════════════════
//  StyleSheet.cpp — v0.4.0
// ════════════════════════════════════════════════════════════════════════════
#include "document/StyleSheet.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace vela {

StyleSheet::StyleSheet() = default;

void StyleSheet::seedBuiltinStyles()
{
    // The 11 built-in styles, in canonical order. Matches the JS side.
    m_builtinOrder = {
        "Normal", "Title", "Subtitle",
        "Heading1", "Heading2", "Heading3", "Heading4", "Heading5",
        "Quote", "Code", "ListParagraph",
    };

    struct Seed {
        const char* id; const char* name; const char* font;
        int size; bool bold; bool italic; bool underline;
        const char* color; const char* alignment;
        int before; int after;
        const char* basedOn; const char* bg; const char* transform;
        const char* letterSpacing; int indentLeft; int outline; bool builtin;
    };
    const Seed seeds[] = {
        {"Normal",         "Normal Text", "Source Serif 4", 12,
         false, false, false, "#222222", "left", 0, 6,
         "", "", "", "", 0, -1, true},
        {"Title",          "Title", "Source Serif 4", 26,
         true, false, false, "#3525CD", "center", 0, 12,
         "Normal", "", "", "", 0, 0, true},
        {"Subtitle",       "Subtitle", "Source Serif 4", 16,
         false, true, false, "#464555", "center", 0, 24,
         "Normal", "", "", "", 0, -1, true},
        {"Heading1",       "Heading 1", "Inter", 22,
         true, false, false, "#3525CD", "left", 24, 6,
         "Normal", "", "", "", 0, 0, true},
        {"Heading2",       "Heading 2", "Inter", 18,
         true, false, false, "#464555", "left", 18, 4,
         "Normal", "", "", "", 0, 1, true},
        {"Heading3",       "Heading 3", "Inter", 16,
         true, false, false, "#222222", "left", 12, 3,
         "Normal", "", "", "", 0, 2, true},
        {"Heading4",       "Heading 4", "Inter", 14,
         true, false, false, "#222222", "left", 10, 2,
         "Normal", "", "", "", 0, 3, true},
        {"Heading5",       "Heading 5", "Inter", 12,
         true, false, false, "#222222", "left", 8, 2,
         "Normal", "", "uppercase", "0.04em", 0, 4, true},
        {"Quote",          "Quote", "Source Serif 4", 13,
         false, true, false, "#464555", "left", 8, 8,
         "Normal", "", "", "", 24, -1, true},
        {"Code",           "Code", "JetBrains Mono", 12,
         false, false, false, "#222222", "left", 4, 4,
         "Normal", "#F4F4F4", "", "", 0, -1, true},
        {"ListParagraph",  "List Paragraph", "Source Serif 4", 12,
         false, false, false, "#222222", "left", 0, 2,
         "Normal", "", "", "", 0, -1, true},
    };

    for (const Seed& s : seeds) {
        StyleDef d;
        d.id = QString::fromUtf8(s.id);
        d.name = QString::fromUtf8(s.name);
        d.font = QString::fromUtf8(s.font);
        d.size = s.size;
        d.bold = s.bold;
        d.italic = s.italic;
        d.underline = s.underline;
        d.color = QString::fromUtf8(s.color);
        d.alignment = QString::fromUtf8(s.alignment);
        d.spacingBefore = s.before;
        d.spacingAfter = s.after;
        d.basedOn = QString::fromUtf8(s.basedOn);
        d.background = QString::fromUtf8(s.bg);
        d.textTransform = QString::fromUtf8(s.transform);
        d.letterSpacing = QString::fromUtf8(s.letterSpacing);
        d.indentLeft = s.indentLeft;
        d.outlineLevel = s.outline;
        d.builtin = true;
        if (!m_styles.contains(d.id)) m_styles.insert(d.id, d);
    }
}

void StyleSheet::registerStyle(const StyleDef& def)
{
    if (def.id.isEmpty()) return;
    StyleDef merged = m_styles.value(def.id);
    merged = def;
    if (m_styles.value(def.id).builtin) merged.builtin = true;
    m_styles.insert(def.id, merged);
}

bool StyleSheet::unregisterStyle(const QString& id)
{
    auto it = m_styles.constFind(id);
    if (it == m_styles.constEnd()) return false;
    if (it.value().builtin) return false;
    m_styles.erase(it);
    return true;
}

StyleDef StyleSheet::get(const QString& id) const
{
    return m_styles.value(id);
}

QList<StyleDef> StyleSheet::list() const
{
    QList<StyleDef> out;
    for (const auto& id : m_builtinOrder) {
        if (m_styles.contains(id)) out.append(m_styles.value(id));
    }
    QList<StyleDef> custom;
    for (auto it = m_styles.constBegin(); it != m_styles.constEnd(); ++it) {
        if (!it.value().builtin) custom.append(it.value());
    }
    std::sort(custom.begin(), custom.end(),
              [](const StyleDef& a, const StyleDef& b) { return a.name < b.name; });
    out.append(custom);
    return out;
}

QStringList StyleSheet::toDocxFragments() const
{
    QStringList out;
    for (const StyleDef& s : m_styles) out.append(styleToXml(s));
    return out;
}

void StyleSheet::fromDocxFragment(const QString& xml)
{
    QDomDocument doc;
    if (!doc.setContent(xml)) return;
    const QDomElement styleEl = doc.documentElement();
    if (styleEl.tagName() != QStringLiteral("style")) return;

    StyleDef def;
    def.id     = styleEl.attribute(QStringLiteral("w:styleId"));
    def.basedOn = styleEl.firstChildElement(QStringLiteral("w:basedOn")).attribute(QStringLiteral("w:val"));
    def.name   = styleEl.firstChildElement(QStringLiteral("w:name")).attribute(QStringLiteral("w:val"));
    def.type   = styleEl.attribute(QStringLiteral("w:type"), QStringLiteral("paragraph"));
    def.builtin = styleEl.attribute(QStringLiteral("w:customStyle")) != QStringLiteral("1");

    const QDomElement pPr = styleEl.firstChildElement(QStringLiteral("w:pPr"));
    if (!pPr.isNull()) {
        const QDomElement sp = pPr.firstChildElement(QStringLiteral("w:spacing"));
        if (!sp.isNull()) {
            def.spacingBefore = sp.attribute(QStringLiteral("w:before"), QStringLiteral("0")).toInt() / 20;
            def.spacingAfter  = sp.attribute(QStringLiteral("w:after"),  QStringLiteral("0")).toInt() / 20;
        }
        const QDomElement jc = pPr.firstChildElement(QStringLiteral("w:jc"));
        if (!jc.isNull()) def.alignment = jc.attribute(QStringLiteral("w:val"));
        const QDomElement ind = pPr.firstChildElement(QStringLiteral("w:ind"));
        if (!ind.isNull()) def.indentLeft = ind.attribute(QStringLiteral("w:left"), QStringLiteral("0")).toInt() / 20;
        const QDomElement ol = pPr.firstChildElement(QStringLiteral("w:outlineLvl"));
        if (!ol.isNull()) def.outlineLevel = ol.attribute(QStringLiteral("w:val")).toInt();
    }

    const QDomElement rPr = styleEl.firstChildElement(QStringLiteral("w:rPr"));
    if (!rPr.isNull()) {
        const QDomElement rFonts = rPr.firstChildElement(QStringLiteral("w:rFonts"));
        if (!rFonts.isNull()) def.font = rFonts.attribute(QStringLiteral("w:ascii"));
        if (!rPr.firstChildElement(QStringLiteral("w:b")).isNull())     def.bold      = true;
        if (!rPr.firstChildElement(QStringLiteral("w:i")).isNull())     def.italic    = true;
        if (!rPr.firstChildElement(QStringLiteral("w:u")).isNull())     def.underline = true;
        const QDomElement color = rPr.firstChildElement(QStringLiteral("w:color"));
        if (!color.isNull()) def.color = QStringLiteral("#") + color.attribute(QStringLiteral("w:val"));
        const QDomElement sz = rPr.firstChildElement(QStringLiteral("w:sz"));
        if (!sz.isNull()) def.size = sz.attribute(QStringLiteral("w:val")).toInt() / 2;
        if (!rPr.firstChildElement(QStringLiteral("w:caps")).isNull()) def.textTransform = QStringLiteral("uppercase");
    }

    if (!def.id.isEmpty()) m_styles.insert(def.id, def);
}

bool StyleSheet::saveCustomStyles(const QString& dirPath) const
{
    QDir d(dirPath);
    if (!d.exists()) d.mkpath(QStringLiteral("."));
    const QString path = d.filePath(QStringLiteral("custom-styles.json"));

    QJsonArray arr;
    for (auto it = m_styles.constBegin(); it != m_styles.constEnd(); ++it) {
        if (it.value().builtin) continue;
        const StyleDef& s = it.value();
        QJsonObject o;
        o.insert(QStringLiteral("id"),                  s.id);
        o.insert(QStringLiteral("name"),                s.name);
        o.insert(QStringLiteral("basedOn"),             s.basedOn);
        o.insert(QStringLiteral("type"),                s.type);
        o.insert(QStringLiteral("font"),                s.font);
        o.insert(QStringLiteral("size"),                s.size);
        o.insert(QStringLiteral("bold"),                s.bold);
        o.insert(QStringLiteral("italic"),              s.italic);
        o.insert(QStringLiteral("underline"),           s.underline);
        o.insert(QStringLiteral("color"),               s.color);
        o.insert(QStringLiteral("alignment"),           s.alignment);
        o.insert(QStringLiteral("spacingBefore"),       s.spacingBefore);
        o.insert(QStringLiteral("spacingAfter"),        s.spacingAfter);
        o.insert(QStringLiteral("indentLeft"),          s.indentLeft);
        o.insert(QStringLiteral("background"),          s.background);
        o.insert(QStringLiteral("textTransform"),       s.textTransform);
        o.insert(QStringLiteral("letterSpacing"),       s.letterSpacing);
        o.insert(QStringLiteral("outlineLevel"),        s.outlineLevel);
        arr.append(o);
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    return true;
}

void StyleSheet::loadCustomStyles(const QString& dirPath)
{
    const QString path = QDir(dirPath).filePath(QStringLiteral("custom-styles.json"));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        StyleDef d;
        d.id              = o.value(QStringLiteral("id")).toString();
        d.name            = o.value(QStringLiteral("name")).toString();
        d.basedOn         = o.value(QStringLiteral("basedOn")).toString();
        d.type            = o.value(QStringLiteral("type")).toString(QStringLiteral("paragraph"));
        d.font            = o.value(QStringLiteral("font")).toString();
        d.size            = o.value(QStringLiteral("size")).toInt(12);
        d.bold            = o.value(QStringLiteral("bold")).toBool();
        d.italic          = o.value(QStringLiteral("italic")).toBool();
        d.underline       = o.value(QStringLiteral("underline")).toBool();
        d.color           = o.value(QStringLiteral("color")).toString();
        d.alignment       = o.value(QStringLiteral("alignment")).toString(QStringLiteral("left"));
        d.spacingBefore   = o.value(QStringLiteral("spacingBefore")).toInt();
        d.spacingAfter    = o.value(QStringLiteral("spacingAfter")).toInt();
        d.indentLeft      = o.value(QStringLiteral("indentLeft")).toInt();
        d.background      = o.value(QStringLiteral("background")).toString();
        d.textTransform   = o.value(QStringLiteral("textTransform")).toString();
        d.letterSpacing   = o.value(QStringLiteral("letterSpacing")).toString();
        d.outlineLevel    = o.value(QStringLiteral("outlineLevel")).toInt(-1);
        d.builtin         = false;
        if (!d.id.isEmpty()) m_styles.insert(d.id, d);
    }
}

// ── XML helpers ────────────────────────────────────────────────────────

QString StyleSheet::styleToXml(const StyleDef& s) const
{
    const bool isCustom = !s.builtin;
    const QString basedOn = s.basedOn.isEmpty()
        ? QString() : QStringLiteral("<w:basedOn w:val=\"") + escapeXml(s.basedOn) + QStringLiteral("\"/>");
    const QString next = s.basedOn.isEmpty()
        ? QString() : QStringLiteral("<w:next w:val=\"") + escapeXml(s.basedOn) + QStringLiteral("\"/>");

    QStringList pPrParts;
    if (s.outlineLevel >= 0) {
        pPrParts << QStringLiteral("<w:outlineLvl w:val=\"") + QString::number(s.outlineLevel) + QStringLiteral("\"/>");
    }
    if (s.spacingBefore || s.spacingAfter) {
        pPrParts << QStringLiteral("<w:spacing w:before=\"") + QString::number(s.spacingBefore * 20)
                 + QStringLiteral("\" w:after=\"") + QString::number(s.spacingAfter * 20) + QStringLiteral("\"/>");
    }
    if (s.alignment != QStringLiteral("left")) {
        pPrParts << QStringLiteral("<w:jc w:val=\"") + escapeXml(s.alignment) + QStringLiteral("\"/>");
    }
    if (s.indentLeft) {
        pPrParts << QStringLiteral("<w:ind w:left=\"") + QString::number(s.indentLeft * 20) + QStringLiteral("\"/>");
    }
    const QString pPr = pPrParts.isEmpty()
        ? QString() : QStringLiteral("<w:pPr>") + pPrParts.join(QString()) + QStringLiteral("</w:pPr>");

    QStringList rPrParts;
    if (!s.font.isEmpty()) {
        rPrParts << QStringLiteral("<w:rFonts w:ascii=\"") + escapeXml(s.font)
                  + QStringLiteral("\" w:hAnsi=\"") + escapeXml(s.font) + QStringLiteral("\"/>");
    }
    if (s.bold)      rPrParts << QStringLiteral("<w:b/>");
    if (s.italic)    rPrParts << QStringLiteral("<w:i/>");
    if (s.underline) rPrParts << QStringLiteral("<w:u w:val=\"single\"/>");
    if (!s.color.isEmpty()) {
        QString c = s.color;
        c.remove(QLatin1Char('#'));
        rPrParts << QStringLiteral("<w:color w:val=\"") + c.toUpper() + QStringLiteral("\"/>");
    }
    if (s.size) {
        rPrParts << QStringLiteral("<w:sz w:val=\"") + QString::number(s.size * 2) + QStringLiteral("\"/>");
    }
    if (s.textTransform == QStringLiteral("uppercase")) {
        rPrParts << QStringLiteral("<w:caps/>");
    }
    if (!s.background.isEmpty()) {
        QString bg = s.background; bg.remove(QLatin1Char('#'));
        rPrParts << QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"")
                  + bg.toUpper() + QStringLiteral("\"/>");
    }
    const QString rPr = rPrParts.isEmpty()
        ? QString() : QStringLiteral("<w:rPr>") + rPrParts.join(QString()) + QStringLiteral("</w:rPr>");

    QString result;
    result += QStringLiteral("<w:style w:type=\"") + s.type + QStringLiteral("\"");
    if (isCustom) result += QStringLiteral(" w:customStyle=\"1\"");
    result += QStringLiteral(" w:styleId=\"") + escapeXml(s.id) + QStringLiteral("\">");
    result += QStringLiteral("<w:name w:val=\"") + escapeXml(s.name) + QStringLiteral("\"/>");
    result += basedOn + next;
    result += pPr + rPr;
    result += QStringLiteral("</w:style>");
    return result;
}

QString StyleSheet::escapeXml(const QString& s) const
{
    // Build & using char concatenation so the literal '&' never appears
    // in this source — see also color-picker.js / modals.js for the
    // same workaround.
    QString AMP(1, QChar(0x0026));
    QString out;
    out.reserve(s.size());
    for (QChar c : s) {
        switch (c.unicode()) {
            case '&':  out += AMP + QStringLiteral("amp;"); break;
            case '<':  out += AMP + QStringLiteral("lt;"); break;
            case '>':  out += AMP + QStringLiteral("gt;"); break;
            case '"':  out += AMP + QStringLiteral("quot;"); break;
            case '\'': out += AMP + QStringLiteral("apos;"); break;
            default:   out += c;
        }
    }
    return out;
}

} // namespace vela