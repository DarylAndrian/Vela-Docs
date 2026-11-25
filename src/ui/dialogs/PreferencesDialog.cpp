#include "ui/dialogs/PreferencesDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace vela {

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    auto* layout = new QVBoxLayout(this);
    m_darkMode = new QCheckBox(tr("Use dark appearance"), this);
    m_autoSave = new QCheckBox(tr("Keep local recovery copies while I write"), this);
    m_spellCheck = new QCheckBox(tr("Check spelling in English (US)"), this);
    layout->addWidget(m_darkMode);
    layout->addWidget(m_autoSave);
    auto* description = new QLabel(tr("Recovery copies help restore drafts after an unexpected exit."), this);
    description->setWordWrap(true);
    layout->addWidget(description);
    layout->addWidget(m_spellCheck);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(bb);
}

void PreferencesDialog::setOptions(bool darkMode, bool autoSave, bool spellCheck)
{
    m_darkMode->setChecked(darkMode);
    m_autoSave->setChecked(autoSave);
    m_spellCheck->setChecked(spellCheck);
}

bool PreferencesDialog::darkMode() const { return m_darkMode->isChecked(); }
bool PreferencesDialog::autoSaveEnabled() const { return m_autoSave->isChecked(); }
bool PreferencesDialog::spellCheckEnabled() const { return m_spellCheck->isChecked(); }

} // namespace vela
