// ════════════════════════════════════════════════════════════════════════════
//  HunspellBridge.cpp — Implementation outline (v0.3.2 — MAJOR-07)
//
// Filled in by the Hunspell-via-FetchContent task. To enable real spell
// checking add the following to the top-level CMakeLists.txt:
//
//   FetchContent_Declare(hunspell
//     GIT_REPOSITORY https://github.com/hunspell/hunspell.git
//     GIT_TAG        v1.7.2
//   )
//   FetchContent_MakeAvailable(hunspell)
//
// and link this translation unit against `hunspell` + `hunspell-1.7`.
// ════════════════════════════════════════════════════════════════════════════
#include "services/HunspellBridge.h"
#include "util/PathUtils.h"

#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

namespace vela {

namespace {
QMutex g_hunspellMutex;
}

HunspellBridge::HunspellBridge(QObject* parent) : QObject(parent) {}

HunspellBridge::~HunspellBridge()
{
    // TODO: free the Hunspell handle when wired up.
}

bool HunspellBridge::setLocale(const QString& bcp47)
{
    const QString base = PathUtils::dictionariesDir() + QStringLiteral("/") + bcp47;
    const QString aff  = base + QStringLiteral(".aff");
    const QString dic  = base + QStringLiteral(".dic");
    if (!QFile::exists(aff) || !QFile::exists(dic)) {
        emit errorReported(QStringLiteral("Dictionary not found: %1").arg(bcp47));
        return false;
    }
    loadDictionary(aff, dic, base + QStringLiteral("-user.dic"));
    m_locale = bcp47;
    m_ready  = true;
    emit dictionaryChanged(bcp47);
    return true;
}

void HunspellBridge::loadDictionary(const QString& aff, const QString& dic, const QString& user)
{
    Q_UNUSED(aff); Q_UNUSED(dic); Q_UNUSED(user);
    // TODO(v0.3.2):
    //   m_hunspell = Hunspell_create(aff_path, dic_path);
    //   load user dictionary lines into m_userWords + Hunspell_add().
}

bool HunspellBridge::spell(const QString& word) const
{
    if (!m_ready) return true;
    QMutexLocker lock(&g_hunspellMutex);
    Q_UNUSED(word);
    // TODO: Hunspell_spell((Hunhandle*)m_hunspell, word.toUtf8().constData());
    return true;
}

QStringList HunspellBridge::suggestions(const QString& word) const
{
    QStringList out;
    if (!m_ready) return out;
    QMutexLocker lock(&g_hunspellMutex);
    Q_UNUSED(word);
    // TODO: Hunspell_suggest with a char** output; convert to QStringList.
    return out;
}

void HunspellBridge::addToUserDictionary(const QString& word)
{
    if (m_userWords.contains(word))
        return;
    m_userWords.insert(word);
    QMutexLocker lock(&g_hunspellMutex);
    // TODO: Hunspell_add(m_hunspell, word.toUtf8().constData());
    // Also append to user.dic on disk (line per word).
    const QString userDic = PathUtils::dictionariesDir()
                          + QStringLiteral("/") + m_locale + QStringLiteral("-user.dic");
    QFile f(userDic);
    if (f.open(QIODevice::Append))
    {
        QTextStream(&f) << word << "\n";
        f.close();
    }
}

QStringList HunspellBridge::misspelledIn(const QStringList& words) const
{
    QStringList bad;
    for (const QString& w : words)
        if (!spell(w))
            bad.push_back(w);
    return bad;
}

} // namespace vela
