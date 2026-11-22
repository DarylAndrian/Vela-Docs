#include "services/SpellChecker.h"
#include "util/Logger.h"

namespace vela {

SpellChecker::SpellChecker(QObject* parent) : QObject(parent) {}

bool SpellChecker::loadDictionary(const QString& language)
{
    Q_UNUSED(language);
    VE_LOG_DEBUG("SpellChecker::loadDictionary — Phase 2 will use Hunspell");
    return false;
}

bool SpellChecker::check(const QString& word) const
{
    Q_UNUSED(word);
    return true; // permissive default until Phase 2
}

QStringList SpellChecker::suggestions(const QString& word) const
{
    Q_UNUSED(word);
    return {};
}

void SpellChecker::addToSession(const QString& word)
{
    if (!m_sessionWords.contains(word)) m_sessionWords.append(word);
}

} // namespace vela
