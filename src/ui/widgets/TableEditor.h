// ════════════════════════════════════════════════════════════════════════════
//  TableEditor.h — Phase 2
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDialog>

namespace vela {

class TableEditor : public QDialog
{
    Q_OBJECT
public:
    explicit TableEditor(QWidget* parent = nullptr);
};

} // namespace vela
