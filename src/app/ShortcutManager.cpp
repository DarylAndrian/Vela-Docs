// ════════════════════════════════════════════════════════════════════════════
//  ShortcutManager.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "app/ShortcutManager.h"

#include <QAction>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace vela {

namespace {
// Defaults match v0.3.2 CHANGELOG (§MAJOR-09).
const struct Default { const char* cmd; const char* keys; } DEFAULTS[] = {
    // inline marks
    {"bold",          "Ctrl+B"},
    {"italic",        "Ctrl+I"},
    {"underline",     "Ctrl+U"},
    {"strikethrough", "Ctrl+Shift+S"},
    {"clearFormat",   "Ctrl+\\"},
    // lists / blocks
    {"bulletList",    "Ctrl+Shift+L"},
    {"orderedList",   "Ctrl+Shift+O"},
    {"taskList",      "Ctrl+Shift+7"},
    {"indent",        "Ctrl+]"},
    {"outdent",       "Ctrl+["},
    // headings
    {"h1",            "Ctrl+1"},
    {"h2",            "Ctrl+2"},
    {"h3",            "Ctrl+3"},
    {"h4",            "Ctrl+4"},
    {"paragraph",     "Ctrl+0"},
    // history
    {"undo",          "Ctrl+Z"},
    {"redo",          "Ctrl+Y"},
    {"redoAlt",       "Ctrl+Shift+Z"},
    // file
    {"new",           "Ctrl+N"},
    {"open",          "Ctrl+O"},
    {"save",          "Ctrl+S"},
    {"saveAs",        "Ctrl+Shift+S"},
    {"print",         "Ctrl+P"},
    // selection / navigation
    {"selectAll",     "Ctrl+A"},
    {"docStart",      "Ctrl+Home"},
    {"docEnd",        "Ctrl+End"},
    // tools
    {"find",          "Ctrl+F"},
    {"replace",       "Ctrl+H"},
    {"findNext",      "F3"},
    {"findPrev",      "Shift+F3"},
    {"insertLink",    "Ctrl+K"},
    {"insertImage",   "Ctrl+Alt+I"},
    {"focusMode",     "Ctrl+Shift+F11"},
    {"spellCheck",    "F7"},
    // ribbon
    {"switchToHome",  "Ctrl+Alt+H"},
    {"switchToInsert","Ctrl+Alt+I2"}, // disambiguated alias
    {"switchToTable", "Ctrl+Alt+T"},
};
} // anon

ShortcutManager::ShortcutManager(QObject* parent)
    : QObject(parent)
{
    loadDefaults();
}

void ShortcutManager::loadDefaults()
{
    for (const auto& d : DEFAULTS) {
        m_defaults.insert(QString::fromLatin1(d.cmd), QKeySequence(QString::fromLatin1(d.keys)));
        if (!m_overrides.contains(QString::fromLatin1(d.cmd)))
            m_overrides.insert(QString::fromLatin1(d.cmd), QKeySequence(QString::fromLatin1(d.keys)));
    }
}

bool ShortcutManager::loadOverrides(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const auto bytes = f.readAll();
    f.close();

    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject())
        return false;
    const QJsonObject obj = doc.object();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        m_overrides.insert(it.key(), QKeySequence(it.value().toString()));
        if (auto* act = m_actions.value(it.key(), nullptr))
            act->setShortcut(m_overrides.value(it.key()));
    }
    emit bindingChanged({}, {});
    return true;
}

bool ShortcutManager::saveOverrides(const QString& path) const
{
    QJsonObject obj;
    for (auto it = m_overrides.constBegin(); it != m_overrides.constEnd(); ++it)
        obj.insert(it.key(), it.value().toString());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

void ShortcutManager::registerAction(const QString& command, QAction* action)
{
    if (!action) return;
    m_actions.insert(command, action);
    action->setShortcut(m_overrides.value(command, m_defaults.value(command)));
}

QStringList ShortcutManager::commands() const
{
    QStringList out;
    out.reserve(m_defaults.size());
    for (auto it = m_defaults.constBegin(); it != m_defaults.constEnd(); ++it)
        out.append(it.key());
    return out;
}

QKeySequence ShortcutManager::binding(const QString& command) const
{
    return m_overrides.value(command, m_defaults.value(command));
}

void ShortcutManager::setBinding(const QString& command, const QKeySequence& seq)
{
    if (!m_defaults.contains(command))
        m_defaults.insert(command, seq);
    m_overrides.insert(command, seq);
    if (auto* act = m_actions.value(command, nullptr))
        act->setShortcut(seq);
    emit bindingChanged(command, seq);
}

} // namespace vela
