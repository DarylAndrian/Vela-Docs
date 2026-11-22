#include "ui/widgets/TableEditor.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace vela {

TableEditor::TableEditor(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Insert Table"));
    setMinimumWidth(360);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Rows × Columns picker — Phase 2."), this));
    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(bb);
}

} // namespace vela
