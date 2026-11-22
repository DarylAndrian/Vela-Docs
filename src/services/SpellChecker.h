// ════════════════════════════════════════════════════════════════════════════
//  SpellChecker.h — Phase 2 (Hunspell wrapper)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace vela {

class SpellChecker : public QObject
{
    Q_OBJECT
public:
    explicit SpellChecker(QObject* parent = nullptr);

    bool loadDictionary(const QString& language);

    /// Returns true if the word is spelled correctly.
    bool check(const QString& word) const;

    /// Return a list of suggestions for a misspelled word.
    QStringList suggestions(const QString& word) const;

    /// Add a word to the user's running dictionary (in-memory).
    void addToSession(const QString& word);

    bool isLoaded() const { return m_loaded; }
    QString language() const { return m_language; }

private:
    QString     m_language;
    bool        m_loaded = false;
    QStringList m_sessionWords;
};

} // namespace vela
