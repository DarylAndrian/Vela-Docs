// ════════════════════════════════════════════════════════════════════════════
//  PreferencesDialog.h — Phase 5
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDialog>

class QCheckBox;

namespace vela {

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    void setOptions(bool darkMode, bool autoSave, bool spellCheck);
    bool darkMode() const;
    bool autoSaveEnabled() const;
    bool spellCheckEnabled() const;

private:
    QCheckBox* m_darkMode;
    QCheckBox* m_autoSave;
    QCheckBox* m_spellCheck;
};

} // namespace vela
