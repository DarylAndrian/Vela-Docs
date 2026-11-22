// ════════════════════════════════════════════════════════════════════════════
//  PageSetupDialog.cpp — v0.5.0
// ════════════════════════════════════════════════════════════════════════════
#include "ui/dialogs/PageSetupDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#ifdef VE_HAS_WEBENGINE
#  include <QWebEngineView>
#  include <QWebEnginePage>
#endif

namespace vela {

PageSetupDialog::PageSetupDialog(QWebEngineView* view, QWidget* parent)
    : QDialog(parent), m_view(view)
{
    setWindowTitle(tr("Page Setup"));
    setMinimumWidth(460);
    buildUi();

    // Defaults: A4 portrait, 1" margins (matches page-settings.js DEFAULT_SETTINGS).
    QJsonObject defaults;
    defaults.insert(QStringLiteral("size"),        QStringLiteral("A4"));
    defaults.insert(QStringLiteral("orientation"), QStringLiteral("portrait"));
    defaults.insert(QStringLiteral("marginTop"),    1.0);
    defaults.insert(QStringLiteral("marginBottom"), 1.0);
    defaults.insert(QStringLiteral("marginLeft"),   1.0);
    defaults.insert(QStringLiteral("marginRight"),  1.0);
    setSettings(defaults);
}

void PageSetupDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);

    // ── Paper size + Orientation ────────────────────────────────
    auto* topRow = new QHBoxLayout;

    auto* sizeBox = new QGroupBox(tr("Paper size"), this);
    auto* sizeLay = new QVBoxLayout(sizeBox);
    m_paperSize = new QComboBox(this);
    m_paperSize->addItem(tr("A4 (210 × 297 mm)"),   QStringLiteral("A4"));
    m_paperSize->addItem(tr("Letter (8.5 × 11 in)"), QStringLiteral("Letter"));
    m_paperSize->addItem(tr("Legal (8.5 × 14 in)"),  QStringLiteral("Legal"));
    m_paperSize->addItem(tr("Tabloid (11 × 17 in)"), QStringLiteral("Tabloid"));
    sizeLay->addWidget(m_paperSize);
    topRow->addWidget(sizeBox);

    auto* orientBox = new QGroupBox(tr("Orientation"), this);
    auto* orientLay = new QVBoxLayout(orientBox);
    m_portrait  = new QRadioButton(tr("&Portrait"), this);
    m_landscape = new QRadioButton(tr("&Landscape"), this);
    m_portrait->setChecked(true);
    orientLay->addWidget(m_portrait);
    orientLay->addWidget(m_landscape);
    topRow->addWidget(orientBox);

    root->addLayout(topRow);

    // ── Margins ─────────────────────────────────────────────────
    auto* marginBox = new QGroupBox(tr("Margins (inches)"), this);
    auto* marginLay = new QFormLayout(marginBox);
    auto makeSpin = [this]() {
        auto* s = new QDoubleSpinBox(this);
        s->setRange(0.0, 5.0);
        s->setSingleStep(0.25);
        s->setValue(1.0);
        return s;
    };
    m_marginTop    = makeSpin();
    m_marginBottom = makeSpin();
    m_marginLeft   = makeSpin();
    m_marginRight  = makeSpin();
    marginLay->addRow(tr("&Top:"),    m_marginTop);
    marginLay->addRow(tr("&Bottom:"), m_marginBottom);
    marginLay->addRow(tr("&Left:"),   m_marginLeft);
    marginLay->addRow(tr("&Right:"),  m_marginRight);
    root->addWidget(marginBox);

    root->addStretch();

    // ── Buttons ─────────────────────────────────────────────────
    auto* bb = new QDialogButtonBox(
        QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);
    connect(bb, &QDialogButtonBox::accepted, this, &PageSetupDialog::applyAndClose);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(bb->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &PageSetupDialog::applyOnly);
    root->addWidget(bb);
}

QJsonObject PageSetupDialog::settingsJson() const
{
    QJsonObject s;
    s.insert(QStringLiteral("size"),
             m_paperSize->currentData().toString());
    s.insert(QStringLiteral("orientation"),
             m_landscape->isChecked() ? QStringLiteral("landscape") : QStringLiteral("portrait"));
    s.insert(QStringLiteral("marginTop"),    m_marginTop->value());
    s.insert(QStringLiteral("marginBottom"), m_marginBottom->value());
    s.insert(QStringLiteral("marginLeft"),   m_marginLeft->value());
    s.insert(QStringLiteral("marginRight"),  m_marginRight->value());
    // mode is owned by the web editor's toggle — leave it alone here.
    return s;
}

void PageSetupDialog::setSettings(const QJsonObject& s)
{
    const QString size = s.value(QStringLiteral("size")).toString(QStringLiteral("A4"));
    for (int i = 0; i < m_paperSize->count(); ++i) {
        if (m_paperSize->itemData(i).toString() == size) {
            m_paperSize->setCurrentIndex(i);
            break;
        }
    }
    const QString orient = s.value(QStringLiteral("orientation")).toString(QStringLiteral("portrait"));
    m_portrait->setChecked(orient != QLatin1String("landscape"));
    m_landscape->setChecked(orient == QLatin1String("landscape"));
    if (s.contains(QStringLiteral("marginTop")))    m_marginTop->setValue(s.value(QStringLiteral("marginTop")).toDouble(1.0));
    if (s.contains(QStringLiteral("marginBottom"))) m_marginBottom->setValue(s.value(QStringLiteral("marginBottom")).toDouble(1.0));
    if (s.contains(QStringLiteral("marginLeft")))   m_marginLeft->setValue(s.value(QStringLiteral("marginLeft")).toDouble(1.0));
    if (s.contains(QStringLiteral("marginRight")))  m_marginRight->setValue(s.value(QStringLiteral("marginRight")).toDouble(1.0));
}

void PageSetupDialog::pushToEditor()
{
#ifdef VE_HAS_WEBENGINE
    if (!m_view) return;
    const QJsonDocument doc(settingsJson());
    // page-settings.js registers window.__velaSetPageSettings(jsonStr).
    const QString js = QStringLiteral("window.__velaSetPageSettings && window.__velaSetPageSettings(%1);")
                           .arg(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    m_view->page()->runJavaScript(js, [](const QVariant&) { /* fire-and-forget */ });
#endif
}

void PageSetupDialog::applyAndClose()
{
    pushToEditor();
    accept();
}

void PageSetupDialog::applyOnly()
{
    pushToEditor();
}

} // namespace vela
