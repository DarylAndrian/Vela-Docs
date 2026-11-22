// ════════════════════════════════════════════════════════════════════════════
//  TabBar.h — Multi-document tab strip (v0.3.1 — CRIT-02)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QHash>
#include <QTabWidget>
#include <memory>

class QAction;

namespace vela {

class TabSession;

/**
 * @brief Multi-document tab strip below the ribbon.
 *
 * Replaces the v0.2.0 stub (single Welcome tab) with a real tab widget
 * underneath the Stitch "Vela Precision" chrome:
 *   - Drag to reorder tabs
 *   - Close button per tab; "Save before closing?" prompt when dirty
 *   - Quick "+" to create a new tab
 *   - Per-tab dirty indicator dot
 *
 * Each tab is backed by a `TabSession` (lifecycle managed by MainWindow). A
 * TabSession is a QObject, not a widget, so every tab gets an empty placeholder
 * page and the session is tracked in m_sessions keyed by that page — the page
 * travels with its tab when the user reorders, so the pairing stays valid.
 */
class TabBar : public QTabWidget
{
    Q_OBJECT
public:
    explicit TabBar(QWidget* parent = nullptr);

    /// Add a new tab for an existing session. Ownership of `session` stays with caller.
    int  addSession(TabSession* session);
    /// Remove a tab by index; signals before the tab is destroyed.
    void removeSession(int index);

    /// Number of open tabs.
    int  count() const { return QTabWidget::count(); }

    /// The session in the current tab (or nullptr).
    TabSession* currentSession() const;
    /// The session behind tab `index` (or nullptr).
    TabSession* sessionAt(int index) const;
    /// The tab index showing `session`, or -1.
    int indexOfSession(TabSession* session) const;

signals:
    void newTabRequested();
    void currentSessionChanged(TabSession* session);
    void closeRequested(TabSession* session);

private slots:
    void onCurrentChanged(int index);
    void onTabCloseRequested(int index);
    void onSessionTitleChanged(const QString& title);
    void onSessionDirtyChanged(bool dirty);

private:
    void refreshTab(int index);

    QAction* m_addAction = nullptr;
    QHash<QWidget*, TabSession*> m_sessions;
};

} // namespace vela
