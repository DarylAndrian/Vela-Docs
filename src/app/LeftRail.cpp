// ════════════════════════════════════════════════════════════════════════════
//  LeftRail.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "app/LeftRail.h"

#include <QAction>
#include <QApplication>
#include <QStyle>

namespace vela {

LeftRail::LeftRail(QWidget* parent)
    : QToolBar(tr("Left Rail"), parent)
{
    setObjectName(QStringLiteral("VelaLeftRail"));
    setOrientation(Qt::Vertical);
    setMovable(false);
    setFloatable(false);
    setIconSize(QSize(20, 20));
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setFixedWidth(48);

    // ── Search ──
    {
        QAction* a = new QAction(
            QApplication::style()->standardIcon(QStyle::SP_FileDialogContentsView),
            tr("Search"), this);
        a->setCheckable(true);
        a->setToolTip(tr("Search (Ctrl+F)"));
        m_actions.insert(View::Search, a);
        addAction(a);
    }
    addSeparator();

    // ── Outline ──
    {
        QAction* a = new QAction(
            QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView),
            tr("Outline"), this);
        a->setCheckable(true);
        a->setChecked(true); // default view
        a->setToolTip(tr("Document Outline"));
        m_actions.insert(View::Outline, a);
        addAction(a);
    }

    // ── Dictionary (spell-check) ──
    {
        QAction* a = new QAction(
            QApplication::style()->standardIcon(QStyle::SP_DialogApplyButton),
            tr("Dictionary"), this);
        a->setCheckable(true);
        a->setToolTip(tr("Dictionary & spell check (Phase 2)"));
        a->setEnabled(false);
        m_actions.insert(View::Dictionary, a);
        addAction(a);
    }

    addSeparator();

    // ── Comments ──
    {
        QAction* a = new QAction(
            QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation),
            tr("Comments"), this);
        a->setCheckable(true);
        a->setToolTip(tr("Comments (Phase 5)"));
        a->setEnabled(false);
        m_actions.insert(View::Comments, a);
        addAction(a);
    }

    // ── Explore ──
    {
        QAction* a = new QAction(
            QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion),
            tr("Explore"), this);
        a->setCheckable(true);
        a->setToolTip(tr("Explore (Phase 5)"));
        a->setEnabled(false);
        m_actions.insert(View::Explore, a);
        addAction(a);
    }

    // ── History ──
    {
        QAction* a = new QAction(
            QApplication::style()->standardIcon(QStyle::SP_BrowserReload),
            tr("History"), this);
        a->setCheckable(true);
        a->setToolTip(tr("Revision history (Phase 5)"));
        a->setEnabled(false);
        m_actions.insert(View::History, a);
        addAction(a);
    }

    // Wire up: toggle behavior
    for (auto it = m_actions.constBegin(); it != m_actions.constEnd(); ++it) {
        const View v = it.key();
        QAction* a = it.value();
        connect(a, &QAction::toggled, this, [this, v](bool checked) {
            if (checked) {
                if (m_active != v) {
                    if (m_actions.contains(m_active)) {
                        m_actions.value(m_active)->blockSignals(true);
                        m_actions.value(m_active)->setChecked(false);
                        m_actions.value(m_active)->blockSignals(false);
                    }
                    m_active = v;
                }
                emit viewActivated(v);
            } else {
                if (m_active == v) {
                    emit viewDeactivated(v);
                }
            }
        });
    }
}

void LeftRail::activateView(View v)
{
    if (m_actions.contains(v)) {
        m_actions.value(v)->setChecked(true);
    }
}

QAction* LeftRail::actionFor(View v) const
{
    return m_actions.value(v, nullptr);
}

} // namespace vela