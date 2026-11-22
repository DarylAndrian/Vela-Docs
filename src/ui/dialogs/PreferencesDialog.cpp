#include "ui/dialogs/PreferencesDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace vela {

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    setMinimumSize(640, 480);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr(
        "General · Editor · Appearance · Spell check · Advanced · About\n"
        "(Sidebar nav with form panels — Phase 5)"), this));
    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(bb);
}

} // namespace vela
