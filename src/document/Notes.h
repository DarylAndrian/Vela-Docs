// Notes.h — Footnotes & endnotes (v0.3.3 — IMP-05)
#pragma once
#include <QString>
#include <QVector>

namespace vela {

class Document;

enum class NoteKind { Footnote, Endnote };
enum class NumberFormat { Decimal, UpperRoman, LowerRoman, UpperLetter, LowerLetter };

struct Note
{
    int     number   = 0;        // 1-based; 0 = unassigned
    NoteKind kind     = NoteKind::Footnote;
    QString text;
    int     refFrom  = 0;        // character offset of the in-doc reference
};

class Notes
{
public:
    /// Add a new note at the given offset. Renumbers automatically.
    static int  add(Document* doc, NoteKind kind, const QString& text, int refFrom);

    /// Renumber every note in `doc` according to its kind and NumberFormat.
    /// Returns the rendered footnote area text per page (or empty for endnotes).
    static void renumber(QVector<Note>& notes, NumberFormat fmt);

    /// Format a 1-based number into the requested style.
    static QString format(int n, NumberFormat fmt);

private:
    static QString toRoman(int n, bool upper);
};

} // namespace vela
