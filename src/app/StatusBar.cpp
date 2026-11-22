// ════════════════════════════════════════════════════════════════════════════
//  StatusBar.cpp — v0.4.0 cleanup
//
//  Removed the "Phase 0" placeholder badge.
//  Wired the zoom slider to QWebEngineView (no longer permanently disabled).
//  Wired the language label to SettingsStore::spellLanguage().
// ════════════════════════════════════════════════════════════════════════════
#include "app/StatusBar.h"

#include "app/Application.h"
#include "services/SettingsStore.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QSlider>
#include <QWidget>

namespace vela {

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent)
{
    setObjectName(QStringLiteral("VelaStatusBar"));
    setSizeGripEnabled(true);

    // ── Temporary message area (left) ──
    m_message = new QLabel(tr("Ready"), this);
    addWidget(m_message);

    // ── Permanent widgets (right side, in order) ──
    m_pageLabel = new QLabel(tr("Page 1 / 1"), this);
    m_pageLabel->setToolTip(tr("Current page / total pages"));
    addPermanentWidget(m_pageLabel);

    m_wordLabel = new QLabel(tr("Words: 0   Characters: 0"), this);
    m_wordLabel->setToolTip(tr("Word and character count"));
    addPermanentWidget(m_wordLabel);

    // Wired to SettingsStore::spellLanguageChanged in MainWindow
    m_langLabel = new QLabel(this);
    m_langLabel->setToolTip(tr("Spell-check language — change in Preferences"));
    addPermanentWidget(m_langLabel);

    addPermanentWidget(new QLabel(QStringLiteral(" | "), this));

    // ── Zoom slider (wired to QWebEngineView::setZoomFactor in MainWindow) ──
    QWidget* zoomContainer = new QWidget(this);
    QHBoxLayout* zoomLayout = new QHBoxLayout(zoomContainer);
    zoomLayout->setContentsMargins(0, 0, 0, 0);
    zoomLayout->setSpacing(6);

    QLabel* minus = new QLabel(QStringLiteral("−"), this);
    zoomLayout->addWidget(minus);

    m_zoomSlider = new QSlider(Qt::Horizontal, this);
    m_zoomSlider->setRange(50, 300);
    m_zoomSlider->setValue(100);
    m_zoomSlider->setMaximumWidth(120);
    m_zoomSlider->setToolTip(tr("Editor zoom (50%–300%)"));
    // v0.4.0: enabled — no longer the v0.3.x permanently-disabled state
    m_zoomSlider->setEnabled(true);
    connect(m_zoomSlider, &QSlider::valueChanged,
            this, &StatusBar::onZoomChanged);
    zoomLayout->addWidget(m_zoomSlider);

    QLabel* plus = new QLabel(QStringLiteral("+"), this);
    zoomLayout->addWidget(plus);

    m_zoomLabel = new QLabel(QStringLiteral("100%"), this);
    m_zoomLabel->setMinimumWidth(40);
    zoomLayout->addWidget(m_zoomLabel);

    addPermanentWidget(zoomContainer);

    // Progress bar (hidden by default — used for long-running operations)
    m_progress = new QProgressBar(this);
    m_progress->setMaximumWidth(120);
    m_progress->setVisible(false);
    addPermanentWidget(m_progress);

    // Initial language label. There is no SettingsStore singleton — Application
    // owns the one instance — so read it from there and map the enum to text.
    QString lang = QStringLiteral("English (US)");
    if (auto* app = qobject_cast<Application*>(QCoreApplication::instance())) {
        if (SettingsStore* s = app->settings()) {
            switch (s->spellLanguage()) {
            case SettingsStore::SpellLanguage::Auto:    lang = QStringLiteral("Auto"); break;
            case SettingsStore::SpellLanguage::Spanish: lang = QStringLiteral("Spanish"); break;
            case SettingsStore::SpellLanguage::French:  lang = QStringLiteral("French"); break;
            case SettingsStore::SpellLanguage::German:  lang = QStringLiteral("German"); break;
            case SettingsStore::SpellLanguage::Chinese: lang = QStringLiteral("Chinese"); break;
            case SettingsStore::SpellLanguage::English: break;
            }
        }
    }
    setLanguage(lang);
}

void StatusBar::setReadyMessage(const QString& text)
{
    if (m_message) m_message->setText(text);
}

void StatusBar::setPageInfo(int current, int total)
{
    if (m_pageLabel) m_pageLabel->setText(tr("Page %1 / %2").arg(current).arg(total));
}

void StatusBar::setWordCount(int words, int characters)
{
    if (m_wordLabel) m_wordLabel->setText(tr("Words: %1   Characters: %2").arg(words).arg(characters));
}

void StatusBar::setZoomPercent(int pct)
{
    m_currentZoom = pct;
    if (m_zoomSlider) {
        // Block signals to avoid recursion while syncing
        const QSignalBlocker block(m_zoomSlider);
        m_zoomSlider->setValue(pct);
    }
    if (m_zoomLabel)  m_zoomLabel->setText(QStringLiteral("%1%").arg(pct));
}

void StatusBar::setLanguage(const QString& lang)
{
    if (m_langLabel) m_langLabel->setText(lang);
}

void StatusBar::onZoomChanged(int value)
{
    m_currentZoom = value;
    if (m_zoomLabel) m_zoomLabel->setText(QStringLiteral("%1%").arg(value));
    emit zoomChanged(value);
}

} // namespace vela