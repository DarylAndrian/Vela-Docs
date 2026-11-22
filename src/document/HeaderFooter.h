// ════════════════════════════════════════════════════════════════════════════
//  HeaderFooter.h — Headers, footers, page numbers (v0.3.2 — MAJOR-01)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QString>

namespace vela {

/**
 * @brief Per-document header/footer/page-number configuration.
 *
 * Stored on the Document model (and serialized as `<w:hdr>` / `<w:ftr>` /
 * `<w:sectPr>` parts of the DOCX).
 */
struct SectionOptions
{
    bool   differentFirstPage = false;
    bool   oddEven            = false;
    QString headerText;       ///< free text; supports {PAGE}/{NUMPAGES}/Roman tokens
    QString footerText;
    QString pageNumberFormat; ///< "decimal" | "lowerRoman" | "upperRoman" | "lowerLetter" | "upperLetter"
};

class Document;

class HeaderFooter
{
public:
    /// Read section settings from the document; defaults if absent.
    static SectionOptions sectionFor(const Document* doc);

    /// Persist section settings into the document.
    static void writeSection(Document* doc, const SectionOptions& opts);

    /// Render a token-bearing string ("Page {PAGE} of {NUMPAGES}") for status
    /// bar / footer display. Provided because the runtime needs to compute
    /// the visible string independently from the writer.
    static QString renderTokens(const QString& template_, int page, int totalPages);
};

} // namespace vela
