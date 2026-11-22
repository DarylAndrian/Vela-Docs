// ════════════════════════════════════════════════════════════════════════════
//  HunspellBridge.h — Hunspell-backed spell/grammar checker (v0.3.2 — MAJOR-07)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

namespace vela {

/**
 * @brief Wraps the libhunspell 1.7.2 dictionary API for Vela Docs.
 *
 * Backed by `FetchContent` copy at `third_party/hunspell`. Dictionaries
 * (.aff / .dic) live under `resources/dictionaries/<locale>/`.
 *
 * Thread safety: one bridge per editor surface. The underlying Hunspell
 * API is not thread-safe; we serialise access through a QMutex (private).
 *
 * The bridge is language-aware (`setLocale("en-US")` etc.). A grammar
 * pass is exposed too; the v0.3.4 release enables it for English and the
 * languages for which a `*_grammar.aff` ship.
 */
class HunspellBridge : public QObject
{
    Q_OBJECT
public:
    explicit HunspellBridge(QObject* parent = nullptr);
    ~HunspellBridge() override;

    /// Switch to a dictionary set; returns false on missing file.
    bool setLocale(const QString& bcp47);

    /// True iff the bridge has a loaded dictionary.
    bool isReady() const { return m_ready; }

    /// Spell-check a single word; case-insensitive.
    bool spell(const QString& word) const;

    /// Suggest corrections for `word`; up to 12 candidates.
    QStringList suggestions(const QString& word) const;

    /// Add `word` to the user dictionary (persisted).
    void addToUserDictionary(const QString& word);

    /// Batch spell-check (used by the JS spell plugin).
    QStringList misspelledIn(const QStringList& words) const;

signals:
    void dictionaryChanged(const QString& bcp47);
    void errorReported(const QString& message);

private:
    void loadDictionary(const QString& affPath, const QString& dicPath, const QString& userDicPath);

    void*          m_hunspell = nullptr;     /// opaque handle (avoids header dependency)
    QString        m_locale;
    bool           m_ready   = false;
    QSet<QString>  m_userWords;             /// accumulated additions this session
};

} // namespace vela
