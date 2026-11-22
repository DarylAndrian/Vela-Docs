// ════════════════════════════════════════════════════════════════════════════
//  PageStyle.h — v0.5.0 — Page Styles (the LibreOffice Writer–grade section
//                  model that supersedes the v0.3.x one-page-global setup).
//
//  Page Styles in OOXML live in two places:
//    • word/document.xml   → <w:sectPr> reference to a style id + per-section
//                            flags (<w:titlePg/>, <w:evenAndOddHeaders/>)
//    • word/styles.xml    → named <w:style w:type="page" w:styleId="…">
//                            holding <w:pgSz>, <w:pgMar>, <w:cols>,
//                            <w:lnNumType>, header/footer references
//
//  In this model each Section holds a *style id*; the actual page layout lives
//  in a named PageStyle entry on the document. Following LibreOffice/Word,
//  "Heading styles for the document body" → SectionBreak → PageStyle.
//
//  All measurements are in Twips (1/1440 inch) for compatibility with the
//  DOCX wire format. Columns and line-number fields are the same shape used
//  inside <w:sectPr>.
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

namespace vela {

// ─── Break kinds — same as before; kept here so PageStyle.h is self-contained
// ──────────────────────────────────────────────────────────────────────────
enum class PageSectionBreak {
    NextPage  = 0,    ///< break + new section on next page
    Continuous,       ///< start section on the same page
    EvenPage,         ///< start section on the next even-numbered page
    OddPage           ///< start section on the next odd-numbered page
};

enum class PageOrientation {
    Portrait  = 0,
    Landscape = 1
};

struct LineNumberSettings {
    bool    enabled           = false;
    int     countBy           = 1;     ///< show a number every Nth line; 1 = every line
    int     startAt           = 1;     ///< start value
    int     distance          = 360;   ///< <w:distance> in twips (default 0.25")
    bool    restartPerPage    = true;
    bool    restartPerSection = false;
    bool    continuous        = false; ///< continuous from previous section
    QString toXml() const;
    static bool fromXml(const QString& xml, LineNumberSettings& out);
    /// Helper: default values used when the editor creates fresh settings.
    static LineNumberSettings makeDefault() { return LineNumberSettings{}; }
};

struct ColumnSettings {
    int  count         = 1;     ///< 1..16; 1 = single column
    int  space         = 720;   ///< gap in twips (default 0.5")
    bool equalWidth    = true;  ///< all columns same width
    QVector<int> customWidths;  ///< per-column widths when equalWidth is false
    bool separator    = false;  ///< draw a vertical hairline between columns

    QString toXml() const;
    static bool fromXml(const QString& xml, ColumnSettings& out);
    static ColumnSettings makeDefault() { return ColumnSettings{}; }
};

struct DropCapSettings {
    enum class Type { None, Drop, Margin };
    Type    type   = Type::None;
    int     lines  = 2;     ///< # of lines to drop down
    int     indent = 0;     ///< twips; used by w:cap "margin" variant
    QString font;           ///< optional override font
    int     size   = 0;     ///< optional override size (pt)

    QString toXml() const;
    static bool fromXml(const QString& xml, DropCapSettings& out);
    static DropCapSettings makeDefault() { return DropCapSettings{}; }
};

// ─── Document protection (read-only / comments-only / tracked-changes) ─────
struct DocumentProtection {
    enum class Edit {
        NoProtection   = 0,   ///< anyone may edit; no protection in <w:documentProtection>
        ReadOnly,             ///< enforce <w:edit val="readOnly"/>
        Comments,             ///< <w:edit val="comments"> — only comments allowed
        TrackedChanges,       ///< <w:edit val="trackedChanges"> — track changes forced on
        Forms                ///< <w:edit val="forms">  — form fields only
    };

    Edit mode = Edit::NoProtection;
    bool enforce = false;         ///< If false the field is never written
    QString passwordHash;         ///< DOCX stores salted SHA-1; we accept opaque
    QString salt;                 ///<   strings and round-trip them bit-for-bit
    QString algorithmicIdExt;     ///< <w:algorithmName> for newer hash variants
    int  spinCount = 100000;

    QString toXml() const;
    static bool fromXml(const QString& xml, DocumentProtection& out);

    /// UI label for each enum (used by the Protection dialog).
    static QString labelFor(Edit e);
    static QString docxValueFor(Edit e);
    static Edit   fromDocxValue(const QString& v);
};

// ─── Page Style (named layout template) ───────────────────────────────────
struct PageStyle {
    QString id;                       ///< "PageStyle1", "PageStyleTitlePage"…
    QString name;                     ///< "Default", "Landscape", "Title Page"
    bool builtin = false;

    // Size & orientation
    int widthTwips  = 12240;          ///< 8.5" — letter
    int heightTwips = 15840;          ///< 11.0"
    PageOrientation orientation = PageOrientation::Portrait;

    // Margins (twips, all 1440 = 1")
    int marginTop    = 1440;
    int marginRight  = 1440;
    int marginBottom = 1440;
    int marginLeft   = 1440;
    int headerOffset = 720;           ///< distance from top to header
    int footerOffset = 720;           ///< distance from bottom to footer
    int gutter       = 0;             ///< mirror gutter

    // Section flags
    bool mirrorMargins        = false;///< mirror left/right on facing pages
    bool differentFirstPage   = false;///< <w:titlePg/>
    bool differentOddEven     = false;///< document-level (held in <w:settings>)
    /// If true, this section's header/footer are linked to the previous one.
    bool titlePg              = false;
    bool oddEven              = false;

    // Columns + line numbering + protection
    ColumnSettings       columns    = {};
    LineNumberSettings   lineNumbers= {};
    DocumentProtection   protection = {};

    // Header/footer references — each is either empty (== "use default") or
    // a relationship id pointing to word/header*.xml / word/footer*.xml.
    QString headerRef;       // <w:headerReference w:type="default" r:id="…"/>
    QString footerRef;
    QString headerFirstRef;  // <w:headerReference w:type="first"   r:id="…"/>
    QString footerFirstRef;
    QString headerEvenRef;   // <w:headerReference w:type="even"    r:id="…"/>
    QString footerEvenRef;

    /// Serialize the named style entry (used inside word/styles.xml).
    QString toXmlStyleEntry() const;
};

class PageStyleSheet
{
public:
    PageStyleSheet();

    /// Seed the built-in styles: "Default", "Title Page", "Landscape".
    /// Idempotent — safe to call repeatedly.
    void seedBuiltin();

    /// Register / update a style by id. Builtins keep their `builtin` flag.
    void registerStyle(const PageStyle& def);

    /// Remove a non-builtin style. Returns false for builtins or missing id.
    bool unregisterStyle(const QString& id);

    /// Look up a style by id. Returns nullptr (via QHash::value semantics)
    /// if not found.
    PageStyle get(const QString& id) const;

    /// All styles, built-in first then custom alphabetical.
    QList<PageStyle> list() const;

    /// Resolve the "default" style id — falls back to "PageStyleDefault".
    QString defaultId() const { return QStringLiteral("PageStyleDefault"); }

    /// Serialize all named page styles to a list of XML fragments suitable
    /// for inclusion in word/styles.xml.
    QStringList toDocxFragments() const;

    /// Parse a single `<w:style w:type="page">` entry and add it.
    void fromDocxFragment(const QString& xml);

private:
    QHash<QString, PageStyle> m_styles;
    QStringList m_builtinOrder;

    QString escapeXml(const QString& s) const;
};

// ─── Section rewrite (extends the v0.3.x Section struct) ──────────────────
//
// Sections are still stored as a SectionStack on Document. The block below
// is a strict superset of the old `Section` fields, so any consumer that
// knew about `Section.orientation` still compiles.
// ──────────────────────────────────────────────────────────────────────────
struct SectionData {
    PageSectionBreak   breakKind = PageSectionBreak::NextPage;
    PageOrientation    orientation = PageOrientation::Portrait;
    int                marginTop    = 1440;
    int                marginRight  = 1440;
    int                marginBottom = 1440;
    int                marginLeft   = 1440;
    int                headerOffset = 720;
    int                footerOffset = 720;
    int                gutter       = 0;

    ColumnSettings     columns    = {};
    LineNumberSettings lineNumbers= {};

    /// Per-section header/footer text; each is filled in only when the
    /// user does not link to previous. Empty string means "inherited".
    QString firstHeader, firstFooter;
    QString oddHeader,   oddFooter;
    QString evenHeader,  evenFooter;

    /// Pointer (by id) to the PageStyle entry that owns this section's
    /// pgSz / pgMar layout. Empty means "use Default".
    QString pageStyleId;

    /// Link-to-previous header/footer. When true the section inherits
    /// headers from the previous one (no <w:headerReference> written).
    bool linkHeaderToPrevious = true;
    bool linkFooterToPrevious = true;

    /// "Different first page" flag (writes <w:titlePg/> inside <w:sectPr>).
    bool titlePg = false;

    /// Per-section protection override; empty struct == no override.
    DocumentProtection protection = {};
};

class SectionStack
{
public:
    int               count()     const { return m_sections.size(); }
    const SectionData&at(int i)    const { return m_sections[i]; }
    void              append(const SectionData& s);
    void              replace(int i, const SectionData& s);

    /// Serialize the chain of <w:sectPr> for this document. The last entry
    /// is the document's terminator; intermediate ones live inside the
    /// pPr of their owning paragraph.
    QString serializeForDocx(const QString& defaultPageStyleId = QStringLiteral("PageStyleDefault")) const;

private:
    QVector<SectionData> m_sections;
};

} // namespace vela
