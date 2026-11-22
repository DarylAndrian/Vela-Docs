// ════════════════════════════════════════════════════════════════════════════
//  TablePropertiesDialog.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "ui/dialogs/TablePropertiesDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace vela {

TablePropertiesDialog::TablePropertiesDialog(const TableOptions& initial, QWidget* parent)
    : QDialog(parent)
    , m_value(initial)
{
    setWindowTitle(tr("Table Properties"));
    setObjectName(QStringLiteral("VelaTablePropertiesDialog"));

    auto* layout = new QVBoxLayout(this);
    auto* form   = new QFormLayout;

    auto* width    = new QSpinBox(this); width->setRange(10, 200); width->setValue(initial.widthPct); width->setSuffix(QStringLiteral("%"));
    auto* align    = new QComboBox(this); align->addItems({tr("Left"), tr("Center"), tr("Right")});
    align->setCurrentIndex(align->findText(initial.align, Qt::MatchFixedString));
    auto* pad      = new QSpinBox(this); pad->setRange(0, 60); pad->setValue(initial.cellPadding); pad->setSuffix(QStringLiteral(" px"));
    auto* spacing  = new QSpinBox(this); spacing->setRange(0, 30); spacing->setValue(initial.cellSpacing); spacing->setSuffix(QStringLiteral(" px"));
    auto* bStyle   = new QComboBox(this); bStyle->addItems({QStringLiteral("single"), QStringLiteral("double"), QStringLiteral("dotted"), QStringLiteral("dashed"), QStringLiteral("none")});
    bStyle->setCurrentText(initial.borderStyle);
    auto* bColor   = new QLineEdit(this); bColor->setText(initial.borderColor);
    auto* bWidth   = new QSpinBox(this); bWidth->setRange(0, 8); bWidth->setValue(initial.borderWidth); bWidth->setSuffix(QStringLiteral(" px"));
    auto* shading  = new QLineEdit(this); shading->setPlaceholderText(tr("Optional")); shading->setText(initial.shadingColor);
    auto* autofit  = new QComboBox(this); autofit->addItems({QStringLiteral("window"), QStringLiteral("contents"), QStringLiteral("fixed")});
    autofit->setCurrentText(initial.autofit);

    form->addRow(tr("Width"),              width);
    form->addRow(tr("Alignment"),          align);
    form->addRow(tr("Cell padding"),       pad);
    form->addRow(tr("Cell spacing"),       spacing);
    form->addRow(tr("Border style"),       bStyle);
    form->addRow(tr("Border color"),       bColor);
    form->addRow(tr("Border width"),       bWidth);
    form->addRow(tr("Cell shading color"), shading);
    form->addRow(tr("Auto-fit"),           autofit);

    layout->addLayout(form);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this, width, align, pad, spacing, bStyle, bColor, bWidth, shading, autofit]{
        m_value.widthPct     = width->value();
        m_value.align        = align->currentText().toLower();
        m_value.cellPadding  = pad->value();
        m_value.cellSpacing  = spacing->value();
        m_value.borderStyle  = bStyle->currentText();
        m_value.borderColor  = bColor->text();
        m_value.borderWidth  = bWidth->value();
        m_value.shadingColor = shading->text();
        m_value.autofit      = autofit->currentText();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

} // namespace vela
