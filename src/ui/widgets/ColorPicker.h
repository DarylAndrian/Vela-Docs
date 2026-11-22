// ════════════════════════════════════════════════════════════════════════════
//  ColorPicker.h — Phase 2
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QToolButton>

namespace vela {

class ColorPicker : public QToolButton
{
    Q_OBJECT
public:
    explicit ColorPicker(QWidget* parent = nullptr);
};

} // namespace vela
