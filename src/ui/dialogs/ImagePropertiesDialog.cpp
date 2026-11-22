// ════════════════════════════════════════════════════════════════════════════
//  ImagePropertiesDialog.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "ui/dialogs/ImagePropertiesDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace vela {

ImagePropertiesDialog::ImagePropertiesDialog(const ImageOptions& initial, QWidget* parent)
    : QDialog(parent)
    , m_value(initial)
{
    setWindowTitle(tr("Image Properties"));
    setObjectName(QStringLiteral("VelaImagePropertiesDialog"));

    auto* layout = new QVBoxLayout(this);
    auto* form   = new QFormLayout;

    auto* w      = new QSpinBox(this); w->setRange(0, 4096); w->setValue(initial.width);  w->setSuffix(QStringLiteral(" px"));
    auto* h      = new QSpinBox(this); h->setRange(0, 4096); h->setValue(initial.height); h->setSuffix(QStringLiteral(" px"));
    auto* aspect = new QCheckBox(this); aspect->setChecked(initial.keepAspect);
    auto* wrap   = new QComboBox(this); wrap->addItems({QStringLiteral("inline"), QStringLiteral("square"), QStringLiteral("tight"), QStringLiteral("behind"), QStringLiteral("front")});
    wrap->setCurrentText(initial.wrap);
    auto* alt    = new QLineEdit(this); alt->setText(initial.altText);
    auto* cap    = new QLineEdit(this); cap->setText(initial.caption);
    auto* rot    = new QSpinBox(this); rot->setRange(-180, 180); rot->setValue(initial.rotationDeg); rot->setSuffix(QStringLiteral(" °"));

    form->addRow(tr("Width"),          w);
    form->addRow(tr("Height"),         h);
    form->addRow(tr("Keep aspect"),    aspect);
    form->addRow(tr("Text wrapping"),  wrap);
    form->addRow(tr("Alt text"),       alt);
    form->addRow(tr("Caption"),        cap);
    form->addRow(tr("Rotation"),       rot);

    layout->addLayout(form);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this, w, h, aspect, wrap, alt, cap, rot]{
        m_value.width      = w->value();
        m_value.height     = h->value();
        m_value.keepAspect = aspect->isChecked();
        m_value.wrap       = wrap->currentText();
        m_value.altText    = alt->text();
        m_value.caption    = cap->text();
        m_value.rotationDeg= rot->value();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

} // namespace vela
