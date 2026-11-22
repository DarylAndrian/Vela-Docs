// ════════════════════════════════════════════════════════════════════════════
//  TabStop.h — v0.4.0
//
//  Per-paragraph tab stop data structure. Mirrors the JS shape used by
//  editor-playground/src/ruler.js so the two sides stay in sync.
//
//  A TabStop is serialized into DOCX `<w:tabs>` inside `<w:pPr>`:
//
//      <w:tabs>
//          <w:tab w:val="center" w:pos="2160" w:leader="dot"/>
//          <w:tab w:val="right"  w:pos="4320"/>
//      </w:tabs>
//
//  `pos` is in twentieths of a point (1/1440 inch).
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QList>
#include <QString>

namespace vela {

class TabStop
{
public:
    enum Type {
        Left,
        Center,
        Right,
        Decimal,
        Bar,
    };

    enum Leader {
        None,
        Dot,
        Hyphen,
        Underscore,
    };

    TabStop() = default;
    TabStop(double posInches, Type type = Left, Leader leader = None)
        : m_pos(posInches), m_type(type), m_leader(leader) {}

    double pos() const { return m_pos; }       // inches
    Type   type() const { return m_type; }
    Leader leader() const { return m_leader; }

    void setPos(double p) { m_pos = p; }
    void setType(Type t) { m_type = t; }
    void setLeader(Leader l) { m_leader = l; }

    /// DOCX `<w:tab w:val="…" w:pos="…" w:leader="…"/>` XML element (without
    /// the outer tags). `pos` is converted from inches to twentieths of a point.
    QString toDocxXml() const;

    static Type   typeFromString(const QString& s);
    static Leader leaderFromString(const QString& s);
    static QString typeToString(Type t);
    static QString leaderToString(Leader l);

    bool operator==(const TabStop& other) const {
        return m_pos == other.m_pos && m_type == other.m_type && m_leader == other.m_leader;
    }

private:
    double m_pos   = 0.0;
    Type   m_type  = Left;
    Leader m_leader = None;
};

/// Convenience type for a list of tab stops on a single paragraph.
using TabStopList = QList<TabStop>;

} // namespace vela