// TableOfContents.h — v0.3.3 (IMP-04)
#pragma once
#include <QString>
#include <QVector>

namespace vela {

class Document;

struct TocEntry
{
    int     level   = 1;
    QString text;
    QString anchor;       ///< doc-relative slug for navigation
    int     charPos = 0;
};

struct TocOptions
{
    int  maxLevel   = 3;
    bool leaderDots = true;
    QString style   = QStringLiteral("Modern");
};

class TableOfContents
{
public:
    /// Rebuild from the document's heading marks.
    static QVector<TocEntry> build(const QString& markdown,
                                   const TocOptions& opts = TocOptions{});

    /// Render as a Markdown block, ready to be inserted as a `velaToc` node.
    static QString renderMarkdown(const QVector<TocEntry>& entries, const TocOptions& opts);
};

} // namespace vela
