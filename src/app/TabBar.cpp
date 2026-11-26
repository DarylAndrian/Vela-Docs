// ════════════════════════════════════════════════════════════════════════════
//  TabBar.cpp — Multi-document tabs (v0.3.1 — CRIT-02)
// ════════════════════════════════════════════════════════════════════════════
#include "app/TabBar.h"

#include "document/TabSession.h"
#include "ui/dialogs/SaveChangesDialog.h"

#include <QAction>
#include <QApplication>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QWidget>

namespace vela {

TabBar::TabBar(QWidget* parent)
    : QTabWidget(parent)
{
    setObjectName(QStringLiteral("VelaTabBar"));
    setAttribute(Qt::WA_StyledBackground);
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setIconSize(QSize(16, 16));

    // "+" button in the top-right corner of the strip.
    m_addAction = new QAction(QApplication::style()->standardIcon(QStyle::SP_FileDialogNewFolder),
                              tr("New Tab"), this);
    m_addAction->setToolTip(tr("New document  (Ctrl+N)"));
    connect(m_addAction, &QAction::triggered, this, &TabBar::newTabRequested);

    auto* corner = new QToolButton(this);
    corner->setDefaultAction(m_addAction);
    corner->setAutoRaise(true);
    corner->setIconSize(QSize(14, 14));
    setCornerWidget(corner, Qt::TopRightCorner);

    connect(this, &QTabWidget::currentChanged, this, &TabBar::onCurrentChanged);
    connect(this, &QTabWidget::tabCloseRequested, this, &TabBar::onTabCloseRequested);
}

int TabBar::addSession(TabSession* session)
{
    if (!session) return -1;

    // QTabWidget insists on a page widget per tab. The editor surface is shared
    // and owned by MainWindow, so the page is an inert placeholder.
    auto* page = new QWidget(this);
    const bool first = count() == 0;
    int index;
    {
        const QSignalBlocker blocker(this);
        index = addTab(page, session->title());
    }
    m_sessions.insert(page, session);

    setTabToolTip(index, session->filePath().isEmpty() ? tr("Unsaved") : session->filePath());
    setTabIcon(index, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
    refreshTab(index);

    connect(session, &TabSession::titleChanged,     this, &TabBar::onSessionTitleChanged);
    connect(session, &TabSession::dirtyChanged,     this, &TabBar::onSessionDirtyChanged);
    connect(session, &TabSession::saveStateChanged, this, [this, session] {
        refreshTab(indexOfSession(session));
    });

    setCurrentIndex(index);
    if (first) emit currentSessionChanged(session);
    return index;
}

void TabBar::removeSession(int index)
{
    if (index < 0 || index >= count())
        return;
    QWidget* page = widget(index);
    m_sessions.remove(page);
    removeTab(index);
    page->deleteLater();
}

TabSession* TabBar::sessionAt(int index) const
{
    if (index < 0 || index >= count())
        return nullptr;
    return m_sessions.value(widget(index), nullptr);
}

TabSession* TabBar::currentSession() const
{
    return sessionAt(currentIndex());
}

int TabBar::indexOfSession(TabSession* session) const
{
    if (!session) return -1;
    for (int i = 0; i < count(); ++i)
        if (m_sessions.value(widget(i), nullptr) == session)
            return i;
    return -1;
}

void TabBar::onCurrentChanged(int index)
{
    Q_UNUSED(index);
    emit currentSessionChanged(currentSession());
}

void TabBar::onTabCloseRequested(int index)
{
    TabSession* session = sessionAt(index);
    if (!session)
        return;

    if (m_managedClose) {
        emit closeRequested(session);
        return;
    }

    if (session->isDirty()) {
        SaveChangesDialog dialog(this, tr("Save changes before closing?"), session->title());
        const int result = dialog.exec();
        if (result != SaveChangesDialog::Discard
            && (result != SaveChangesDialog::Save || !session->save()))
            return;
    }
    emit closeRequested(session);
    removeSession(index);
}

void TabBar::onSessionTitleChanged(const QString& title)
{
    Q_UNUSED(title);
    refreshTab(indexOfSession(qobject_cast<TabSession*>(sender())));
}

void TabBar::onSessionDirtyChanged(bool dirty)
{
    Q_UNUSED(dirty);
    refreshTab(indexOfSession(qobject_cast<TabSession*>(sender())));
}

void TabBar::refreshTab(int index)
{
    TabSession* session = sessionAt(index);
    if (!session) return;
    const QString title = session->title()
        + (session->isDirty() ? QStringLiteral(" \u2022") : QString());
    setTabText(index, title);
    setTabToolTip(index, session->filePath().isEmpty() ? tr("Unsaved") : session->filePath());
}

} // namespace vela
