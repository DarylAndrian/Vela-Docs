// ════════════════════════════════════════════════════════════════════════════
//  LeftRail.h — Left icon rail (Google Docs / Linear style)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QToolBar>
#include <QHash>

class QAction;

namespace vela {

/**
 * @brief A narrow icon-rail toolbar on the LEFT side of the editor.
 *
 * From the Stitch "Vela Precision" design:
 * - 48px wide, full height
 * - Icon buttons stacked vertically
 * - Round "pill" hover/active states
 * - Holds: Search, Outline, Dictionary, (future) Comments, Explore, History
 *
 * Clicking a button toggles the corresponding right-side sidebar tab.
 */
class LeftRail : public QToolBar
{
    Q_OBJECT
public:
    explicit LeftRail(QWidget* parent = nullptr);

    enum class View {
        Search,
        Outline,
        Dictionary,
        Comments,
        Explore,
        History,
    };
    Q_ENUM(View)

    /// Programmatically activate a view (emits toggled signal).
    void activateView(View v);
    View activeView() const { return m_active; }

signals:
    /// Emitted when the user clicks an icon (or activateView is called).
    void viewActivated(View v);

    /// Emitted when the same active icon is clicked again — for closing.
    void viewDeactivated(View v);

private:
    QAction* actionFor(View v) const;

    QHash<View, QAction*> m_actions;
    View m_active = View::Outline;
};

} // namespace vela