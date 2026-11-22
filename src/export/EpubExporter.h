// EpubExporter.h — EPUB export (v0.3.5 — POLISH-03)
#pragma once
#include <QString>

namespace vela {

class Document;

/**
 * @brief Doc → EPUB 3.0 export.
 *
 * Wraps `libepubgen` (added via `FetchContent` in v0.3.5). Produces a
 * reflowable EPUB with:
 *   - one `<spine>` item per H1 chapter
 *   - a `nav.xhtml` auto-TOC
 *   - `dc:title / dc:creator / dc:language` metadata
 *   - optional cover image
 */
class EpubExporter
{
public:
    struct Options
    {
        QString title;
        QString author;
        QString language = QStringLiteral("en");
        QString coverPath;
        bool splitOnH1 = true;
    };

    static bool exportTo(const Document* doc, const QString& epubPath, const Options& opts = {});
};

} // namespace vela
