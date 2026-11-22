// ════════════════════════════════════════════════════════════════════════════
//  Outline.h — Document heading outline (v0.3.3 — IMP-01)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QVector>

namespace vela {

class Document;

/// One entry in the navigation pane.
struct OutlineEntry
{
    QString title;
    int     level   = 1;        // 1..6
    int     charPos = 0;        // < 0 means pending
};

class Outline
{
public:
    /// Compute an outline from any plain text containing `#` markdown
    /// headings (used as the editor calls back into this fallback path
    /// until v0.3.3's TipTap outline node lands).
    static QVector<OutlineEntry> fromMarkdown(const QString& md);

    /// Live update: subscribed to the editor's onUpdate event.
    /// The first version uses the static method above until a richer
    /// ProseMirror-backed walker ships in v0.3.3.
};

} // namespace vela
