// docx_hf.h — v0.3.2 (MAJOR-01)
#pragma once

#include <QByteArray>

#include "document/HeaderFooter.h"

namespace vela {

/**
 * @brief Round-trip helpers for DOCX header/footer + sectPr parts.
 *
 * Pure functions: `serialize…` builds the XML; `parse…` reads it back.
 */
class DocxHF
{
public:
    /// Build the `<w:sectPr>` element from a `SectionOptions`.
    static QByteArray   serializeSectionProps(const SectionOptions& opts);
    /// Read `<w:sectPr>` into a `SectionOptions`. Defaults if absent.
    static SectionOptions parseSectionProps(const QByteArray& xml);
};

} // namespace vela
