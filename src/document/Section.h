// Section.h — Sections, columns, per-section orientation (v0.3.3 — IMP-09)
#pragma once
#include <QString>

namespace vela {

enum class SectionBreak { NextPage, Continuous, EvenPage, OddPage };
enum class Orientation { Portrait, Landscape };

struct Section
{
    SectionBreak breakKind    = SectionBreak::NextPage;
    Orientation  orientation = Orientation::Portrait;
    int          marginTop   = 1440;
    int          marginBottom= 1440;
    int          marginLeft  = 1440;
    int          marginRight = 1440;
    int          columns     = 1;
    QString      headerText;     ///< shared with HeaderFooter in v0.3.2
    QString      footerText;
};

class SectionStack
{
public:
    /// Number of sections defined in the document.
    int                  count() const { return m_sections.size(); }
    /// Read a section.
    const Section&       at(int i) const { return m_sections[i]; }
    /// Append / replace.
    void                 append(const Section& s);
    void                 replace(int i, const Section& s);

    /// Serialize the full chain of `<w:sectPr>` to DOCX (one per section).
    QString              serializeForDocx() const;

private:
    QVector<Section> m_sections;
};

} // namespace vela
