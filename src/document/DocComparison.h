// DocComparison.h — Side-by-side doc compare (v0.3.5 — POLISH-05)
#pragma once
#include <QString>
#include <QVector>

namespace vela {

struct DiffHunk
{
    int    leftStart  = 0;
    int    leftLen    = 0;
    int    rightStart = 0;
    int    rightLen   = 0;
    enum Kind { Equal, Insertion, Deletion, Modification } kind = Equal;
};

class DocComparison
{
public:
    /// Word-level diff of two plain-text documents.
    static QVector<DiffHunk> wordDiff(const QString& left, const QString& right);
    /// Render the diff as a sequence of `{ kind, text }` for the UI.
    static QString           renderSideBySide(const QVector<DiffHunk>& hunks);
};

} // namespace vela
