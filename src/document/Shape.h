// Shape.h — Drawing shapes (v0.3.3 — IMP-10)
#pragma once
#include <QColor>
#include <QString>

namespace vela {

enum class ShapeKind { Rectangle, RoundedRectangle, Oval, Arrow, Line, Callout };

struct Shape
{
    ShapeKind kind       = ShapeKind::Rectangle;
    QColor    fill       = QColor(0x35, 0x25, 0xcd, 0x40);
    QColor    outline    = QColor(0x35, 0x25, 0xcd);
    int       outlinePx  = 1;
    int       x = 0, y = 0;   ///< position from page origin
    int       width = 100, height = 80;
    QString   text;          ///< text-in-shape
};

} // namespace vela
