// ════════════════════════════════════════════════════════════════════════════
//  SettingsStore.h — Typed wrapper around QSettings
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>

class QSettings;

namespace vela {

/**
 * @brief Typed, validated wrapper around QSettings for application-wide prefs.
 *
 * Phase 0: Theme + editor preferences. More keys added through subsequent phases.
 */
class SettingsStore : public QObject
{
    Q_OBJECT
public:
    enum class Theme { System, Light, Dark };
    Q_ENUM(Theme)

    enum class SpellLanguage { Auto, English, Spanish, French, German, Chinese };
    Q_ENUM(SpellLanguage)

    explicit SettingsStore(QObject* parent = nullptr);
    ~SettingsStore() override;

    // ── Theme ──
    Theme theme() const;
    void setTheme(Theme t);

    // ── Spell check ──
    SpellLanguage spellLanguage() const;
    void setSpellLanguage(SpellLanguage l);
    bool spellCheckEnabled() const;
    void setSpellCheckEnabled(bool enabled);

    // ── Window geometry ──
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geom);

    // ── Auto-save ──
    bool autoSaveEnabled() const;
    void setAutoSaveEnabled(bool on);

signals:
    void themeChanged(Theme t);
    void settingsChanged();

private:
    QSettings* m_settings = nullptr;
};

} // namespace vela
