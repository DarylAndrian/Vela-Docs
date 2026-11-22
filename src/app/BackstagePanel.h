// ════════════════════════════════════════════════════════════════════════════
//  BackstagePanel.h — base class for the right-side panels of the Backstage
//
//  A BackstagePanel is a regular QWidget that fills the content area of the
//  Backstage overlay. Subclasses override `panelId()` (must be unique) and
//  build their UI in the constructor.
//
//  Optional lifecycle:
//      onShown()    — called every time the panel becomes active (after
//                     a panel switch from the rail)
//      onHidden()   — called every time the panel is no longer active
//
//  Panels are owned by the Backstage (it deletes them on destruction).
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QString>
#include <QWidget>

namespace vela {

class BackstagePanel : public QWidget
{
    Q_OBJECT
public:
    explicit BackstagePanel(QWidget* parent = nullptr) : QWidget(parent) {}
    ~BackstagePanel() override = default;

    /// Unique identifier used by Backstage::selectPanel().
    virtual QString panelId() const = 0;

    /// Human-readable title shown in the rail tooltip.
    virtual QString panelTitle() const { return panelId(); }

    /// Called by Backstage when this panel becomes the active one.
    virtual void onShown() {}

    /// Called by Backstage when this panel is no longer active.
    virtual void onHidden() {}
};

} // namespace vela