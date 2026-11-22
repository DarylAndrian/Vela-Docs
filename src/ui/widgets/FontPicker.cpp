#include "ui/widgets/FontPicker.h"

namespace vela {

FontPicker::FontPicker(QWidget* parent)
    : QComboBox(parent)
{
    setEditable(true);
    setEnabled(false); // Phase 2
}

} // namespace vela
