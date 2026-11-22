// ════════════════════════════════════════════════════════════════════════════
//  PreferencesDialog.h — Phase 5
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDialog>

namespace vela {

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
};

} // namespace vela
