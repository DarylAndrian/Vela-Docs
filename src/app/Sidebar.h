// ════════════════════════════════════════════════════════════════════════════
//  Sidebar.h — Right-side dock widget (Stitch "Vela Precision")
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDockWidget>

class QListWidget;
class QTabWidget;

namespace vela {

/**
 * @brief Right-side dock widget showing the active sidebar tab
 *        (Outline / Comments / Explore / History) selected by the LeftRail.
 */
class Sidebar : public QDockWidget
{
    Q_OBJECT
public:
    explicit Sidebar(QWidget* parent = nullptr);

public slots:
    /// Show only the tab matching the given left-rail view, hide others.
    void showView(int index);
    /// Show only the Outline tab (default).
    void showOutline();

signals:
    /// Emitted when user clicks an outline entry — MainWindow can jump to it.
    void outlineItemActivated(const QString& sectionTitle);

private:
    void buildContents();

    QTabWidget*   m_tabs     = nullptr;
    QListWidget*  m_outline  = nullptr;
};

} // namespace vela