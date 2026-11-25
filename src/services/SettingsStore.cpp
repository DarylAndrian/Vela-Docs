// ════════════════════════════════════════════════════════════════════════════
//  SettingsStore.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "services/SettingsStore.h"

#include <QSettings>

namespace vela {

SettingsStore::SettingsStore(QObject* parent)
    : QObject(parent)
    , m_settings(new QSettings(this))
{
}

SettingsStore::~SettingsStore() = default;

SettingsStore::Theme SettingsStore::theme() const
{
    const int v = m_settings->value(QStringLiteral("ui/theme"), static_cast<int>(Theme::System)).toInt();
    return static_cast<Theme>(v);
}

void SettingsStore::setTheme(Theme t)
{
    if (theme() == t) return;
    m_settings->setValue(QStringLiteral("ui/theme"), static_cast<int>(t));
    emit themeChanged(t);
    emit settingsChanged();
}

SettingsStore::SpellLanguage SettingsStore::spellLanguage() const
{
    const int v = m_settings->value(QStringLiteral("editor/spellLanguage"),
        static_cast<int>(SpellLanguage::Auto)).toInt();
    return static_cast<SpellLanguage>(v);
}

void SettingsStore::setSpellLanguage(SpellLanguage l)
{
    if (spellLanguage() == l) return;
    m_settings->setValue(QStringLiteral("editor/spellLanguage"), static_cast<int>(l));
    emit settingsChanged();
}

bool SettingsStore::spellCheckEnabled() const
{
    return m_settings->value(QStringLiteral("editor/spellCheck"), true).toBool();
}

void SettingsStore::setSpellCheckEnabled(bool enabled)
{
    m_settings->setValue(QStringLiteral("editor/spellCheck"), enabled);
    emit settingsChanged();
}

QByteArray SettingsStore::windowGeometry() const
{
    return m_settings->value(QStringLiteral("window/geometry")).toByteArray();
}

void SettingsStore::setWindowGeometry(const QByteArray& geom)
{
    m_settings->setValue(QStringLiteral("window/geometry"), geom);
}

bool SettingsStore::autoSaveEnabled() const
{
    return m_settings->value(QStringLiteral("editor/autoSave"), true).toBool();
}

void SettingsStore::setAutoSaveEnabled(bool on)
{
    m_settings->setValue(QStringLiteral("editor/autoSave"), on);
    emit settingsChanged();
}

} // namespace vela
