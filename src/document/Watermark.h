// Watermark.h — v0.3.3 (IMP-08)
#pragma once
#include <QColor>
#include <QString>

namespace vela {

struct Watermark
{
    QString text;
    QString imagePath;
    QString layout    = QStringLiteral("diagonal");   // diagonal/horizontal/custom
    QColor  color     = QColor(0x80, 0x80, 0x80, 0x40);
    int     sizePct   = 200;                          // % of page width
    int     rotation  = -45;                          // °
    bool    isText    = true;
};

class WatermarkFactory
{
public:
    /// Build a `<w:pict>` overlay for the document header.
    static QString serializeForDocx(const Watermark& w);
};

} // namespace vela
