// ════════════════════════════════════════════════════════════════════════════
//  FontPicker.h — Phase 2
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QComboBox>

namespace vela {

class FontPicker : public QComboBox
{
    Q_OBJECT
public:
    explicit FontPicker(QWidget* parent = nullptr);
};

} // namespace vela
