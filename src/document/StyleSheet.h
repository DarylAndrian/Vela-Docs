// ════════════════════════════════════════════════════════════════════════════
//  StyleSheet.h — v0.4.0
//
//  C++ mirror of the editor-playground/src/style-manager.js module.
//  Holds a Map<id, StyleDef> of paragraph styles (built-in + user-created),
//  serializes to DOCX `<w:style>` fragments, and parses them back.
//
//  This is the C++ side that the EditorBridge reads from / writes to when
//  the user creates or modifies styles via the Backstage > Styles UI.
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

namespace vela {

/// A single paragraph or character style. Mirrors the JS shape used by
/// editor-playground/src/style-manager.js exactly so the two sides stay
/// in sync.
struct StyleDef {
    QString id;                  // "Heading1", "Heading10", "Caption", ...
    QString name;                // "Heading 1", "Caption", ...
    QString basedOn;             // "Normal", "Heading4", "" (none)
    QString type = QStringLiteral("paragraph"); // "paragraph" | "inline"
    QString font;                // "Inter", "Source Serif 4", ...
    int     size = 12;           // pt
    bool    bold      = false;
    bool    italic    = false;
    bool    underline = false;
    QString color;               // "#RRGGBB"
    QString alignment = QStringLiteral("left");
    int     spacingBefore = 0;   // pt
    int     spacingAfter  = 6;   // pt
    int     indentLeft   = 0;    // pt
    QString background;          // "#RRGGBB" (cell shading)
    QString textTransform;        // "uppercase" | "" (none)
    QString letterSpacing;        // CSS-style, e.g. "0.04em"
    int     outlineLevel = -1;   // -1 = not a heading; 0-9 for headings
    bool    builtin = false;
};

class StyleSheet
{
public:
    StyleSheet();

    /// Seed built-in styles. Idempotent.
    void seedBuiltinStyles();

    /// Register or update a style.
    void registerStyle(const StyleDef& def);

    /// Remove a custom (non-builtin) style.
    bool unregisterStyle(const QString& id);

    /// Get a style by id. Returns nullptr if not found.
    StyleDef get(const QString& id) const;

    /// List all styles (built-in first, then custom alphabetical).
    QList<StyleDef> list() const;

    /// Serialize all styles to DOCX `<w:style>` fragments (XML strings).
    QStringList toDocxFragments() const;

    /// Parse a DOCX `<w:style>` XML string into a StyleDef and add it.
    /// Best-effort parser — handles the most common attributes.
    void fromDocxFragment(const QString& xml);

    /// Persist custom styles to a JSON file in `dir`.
    bool saveCustomStyles(const QString& dirPath) const;

    /// Load custom styles from `dirPath/custom-styles.json` (if present).
    void loadCustomStyles(const QString& dirPath);

private:
    QHash<QString, StyleDef> m_styles;
    QStringList m_builtinOrder; // canonical order of built-in style ids

    QString styleToXml(const StyleDef& s) const;
    QString escapeXml(const QString& s) const;
};

} // namespace vela