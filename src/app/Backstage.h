// ════════════════════════════════════════════════════════════════════════════
//  Backstage.h — Microsoft Word-style backstage view (v0.4.0)
//
//  A full-width overlay that slides over the editor when the user clicks
//  the colored "Files" tab in the ribbon or presses Alt+F. Hosts a left
//  rail of actions (Save / Save As / Open / Print / Print Preview / Options
//  / Help / Recent) and a right-side content stack that swaps panels.
//
//  Owns:
//      m_rail       — the vertical action list
//      m_content    — QStackedWidget that holds the active BackstagePanel
//      m_closeBtn   — the "X" close button (top-right)
//
//  Each panel is a BackstagePanel subclass. New panels are added by
//  (1) subclassing BackstagePanel and (2) calling Backstage::registerPanel().
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QFrame>
#include <QHash>
#include <QPointer>
#include <QStackedWidget>
#include <QString>

class QAbstractButton;
class QVBoxLayout;
class QToolButton;
class QPropertyAnimation;

namespace vela {

class BackstagePanel;

class BackstageRail : public QFrame
{
    Q_OBJECT
public:
    explicit BackstageRail(QWidget* parent = nullptr);

    void addItem(const QString& id, const QString& label, const QString& iconText);
    void setActive(const QString& id);

signals:
    void itemActivated(const QString& id);

private:
    QVBoxLayout* m_layout = nullptr;
    QHash<QString, QToolButton*> m_buttons;
};

class Backstage : public QFrame
{
    Q_OBJECT
public:
    explicit Backstage(QWidget* parent = nullptr);
    ~Backstage() override;

    /// Show the backstage overlay on top of `parent` and select `panelId`.
    /// If `panelId` is empty, the first registered panel is used.
    void show(QWidget* parent, const QString& panelId = QString());

    /// Hide the overlay.
    void hide();

    /// Toggle the overlay.
    void toggle(QWidget* parent, const QString& panelId = QString());

    /// Switch the active panel by id.
    void selectPanel(const QString& panelId);

    /// Register a panel. The Backstage takes ownership of `panel`.
    void registerPanel(BackstagePanel* panel);

    /// True if the overlay is currently visible.
    bool isVisible() const { return !isHidden(); }

signals:
    void visibilityChanged(bool visible);
    void panelSelected(const QString& panelId);

private slots:
    void onItemActivated(const QString& id);
    void onCloseClicked();

private:
    void setupUi();
    void setupAnimation();

    BackstageRail*       m_rail    = nullptr;
    QStackedWidget*      m_content = nullptr;
    QAbstractButton*     m_closeBtn = nullptr;
    QPropertyAnimation*  m_anim    = nullptr;

    QHash<QString, BackstagePanel*> m_panels;
    QString m_currentPanelId;
};

} // namespace vela