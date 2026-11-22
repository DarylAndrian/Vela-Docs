// ════════════════════════════════════════════════════════════════════════════
//  ShortcutManager.h — Global keyboard shortcut registry (v0.3.2 — MAJOR-09)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QHash>
#include <QKeySequence>
#include <QObject>
#include <QString>
#include <QStringList>

namespace vela {

/**
 * @brief Central registry of keyboard shortcuts.
 *
 * Loads defaults from `resources/shortcuts.json` and any user overrides
 * from `%APPDATA%/Vela Docs/shortcuts.json`. Provides:
 *   - `registerAction(QAction*)` — bind a key sequence to a Qt action
 *   - `setBinding(QAction*, QKeySequence)` — change a binding
 *   - `loadOverrides(path)` / `saveOverrides(path)` — user customisation
 */
class ShortcutManager : public QObject
{
    Q_OBJECT
public:
    explicit ShortcutManager(QObject* parent = nullptr);

    /// Load default bindings (resources/shortcuts.json).
    void loadDefaults();
    /// Load user overrides (`shortcuts.json` in the app data dir).
    bool loadOverrides(const QString& path);
    /// Persist current bindings + overrides to disk.
    bool saveOverrides(const QString& path) const;

    /// Apply the registered binding to a Qt action.
    void registerAction(const QString& command, QAction* action);

    /// All known command names (for the Preferences UI).
    QStringList commands() const;
    /// Read a single binding.
    QKeySequence binding(const QString& command) const;
    /// Set a single binding (in memory).
    void setBinding(const QString& command, const QKeySequence& seq);

signals:
    void bindingChanged(const QString& command, const QKeySequence& seq);

private:
    QHash<QString, QKeySequence> m_defaults;
    QHash<QString, QKeySequence> m_overrides;
    QHash<QString, QAction*>     m_actions;
};

} // namespace vela
