// ════════════════════════════════════════════════════════════════════════════
//  Backstage.cpp — Microsoft Word-style backstage view (v0.4.0)
//
//  See Backstage.h for design notes. The class is split across Backstage.h
//  (the public interface) and BackstagePanel.h (the base class for each
//  right-side panel).
// ════════════════════════════════════════════════════════════════════════════
#include "app/Backstage.h"
#include "app/BackstagePanel.h"

#include <QAbstractButton>
#include <QApplication>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace vela {

// ═══════════════════════════════════════════════════════════════════════════
//  BackstageRail
// ═══════════════════════════════════════════════════════════════════════════

BackstageRail::BackstageRail(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("VelaBackstageRail"));
    setFixedWidth(220);
    setStyleSheet(QStringLiteral(
        "QFrame#VelaBackstageRail { background: #2A2A3A; border-right: 1px solid #1A1A28; }"));

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(8, 16, 8, 16);
    m_layout->setSpacing(2);

    // Top stretch
    m_layout->addStretch(1);
}

void BackstageRail::addItem(const QString& id, const QString& label, const QString& iconText)
{
    auto* btn = new QToolButton(this);
    btn->setObjectName(QStringLiteral("VelaBackstageItem"));
    btn->setText(QStringLiteral("  %1   %2").arg(iconText, label));
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    btn->setMinimumHeight(36);
    btn->setCheckable(true);
    btn->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    btn->setStyleSheet(QStringLiteral(
        "QToolButton#VelaBackstageItem {"
        "  color: #C8C8D8;"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 4px;"
        "  text-align: left;"
        "  padding: 6px 10px;"
        "}"
        "QToolButton#VelaBackstageItem:hover { background: #3A3A4D; color: #FFFFFF; }"
        "QToolButton#VelaBackstageItem:checked { background: #3525CD; color: #FFFFFF; }"));

    connect(btn, &QToolButton::clicked, this, [this, id]() {
        emit itemActivated(id);
    });

    m_buttons.insert(id, btn);
    m_layout->insertWidget(m_layout->count() - 1, btn); // before the trailing stretch
}

void BackstageRail::setActive(const QString& id)
{
    for (auto it = m_buttons.constBegin(); it != m_buttons.constEnd(); ++it) {
        it.value()->setChecked(it.key() == id);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Backstage
// ═══════════════════════════════════════════════════════════════════════════

Backstage::Backstage(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("VelaBackstage"));
    setStyleSheet(QStringLiteral(
        "QFrame#VelaBackstage { background: #F7F7FA; border-top: 1px solid #E1E8FD; }"));

    setupUi();
    setupAnimation();
    hide();
}

Backstage::~Backstage()
{
    qDeleteAll(m_panels);
    m_panels.clear();
}

void Backstage::setupUi()
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_rail = new BackstageRail(this);
    root->addWidget(m_rail);

    m_content = new QStackedWidget(this);
    m_content->setObjectName(QStringLiteral("VelaBackstageContent"));
    root->addWidget(m_content, 1);

    // Close button floating in the top-right of the content area
    m_closeBtn = new QPushButton(QStringLiteral("✕"), this);
    m_closeBtn->setObjectName(QStringLiteral("VelaBackstageClose"));
    m_closeBtn->setFixedSize(32, 32);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton#VelaBackstageClose {"
        "  background: transparent;"
        "  color: #464555;"
        "  border: none;"
        "  border-radius: 16px;"
        "  font-size: 16px;"
        "}"
        "QPushButton#VelaBackstageClose:hover { background: #E1E8FD; }"));
    m_closeBtn->setToolTip(tr("Close (Esc)"));

    connect(m_rail, &BackstageRail::itemActivated,
            this, &Backstage::onItemActivated);
    connect(m_closeBtn, &QPushButton::clicked,
            this, &Backstage::onCloseClicked);
}

void Backstage::setupAnimation()
{
    m_anim = new QPropertyAnimation(this, "geometry", this);
    m_anim->setDuration(180);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
}

void Backstage::show(QWidget* /*parent*/, const QString& panelId)
{
    if (m_panels.isEmpty()) {
        qWarning("Backstage::show: no panels registered");
        return;
    }
    if (panelId.isEmpty()) {
        selectPanel(m_panels.firstKey());
    } else {
        selectPanel(panelId);
    }

    QWidget* top = parentWidget();
    if (top) {
        setGeometry(0, 0, top->width(), top->height());
    }
    QFrame::show();
    raise();
    emit visibilityChanged(true);
}

void Backstage::hide()
{
    if (isHidden()) return;
    QFrame::hide();
    emit visibilityChanged(false);
}

void Backstage::toggle(QWidget* parent, const QString& panelId)
{
    if (isVisible()) hide();
    else show(parent, panelId);
}

void Backstage::selectPanel(const QString& panelId)
{
    if (!m_panels.contains(panelId)) {
        qWarning("Backstage::selectPanel: unknown panel '%s'", qUtf8Printable(panelId));
        return;
    }
    m_currentPanelId = panelId;
    m_rail->setActive(panelId);
    m_content->setCurrentWidget(m_panels.value(panelId));
    emit panelSelected(panelId);
}

void Backstage::registerPanel(BackstagePanel* panel)
{
    if (!panel) return;
    m_panels.insert(panel->panelId(), panel);
    m_content->addWidget(panel);
}

void Backstage::onItemActivated(const QString& id)
{
    selectPanel(id);
}

void Backstage::onCloseClicked()
{
    hide();
}

} // namespace vela