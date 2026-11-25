#include "services/DraftRecovery.h"

#include "document/Document.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLockFile>
#include <QSaveFile>
#include <QUuid>
#include <algorithm>

namespace vela {

DraftRecovery::DraftRecovery(const QString& root) : m_root(root) {}

DraftRecovery::~DraftRecovery()
{
    m_sessionLock.reset();
    if (!m_session.isEmpty()) QDir().rmdir(m_session);
}

bool DraftRecovery::store(const Document& document)
{
    const QJsonObject snapshot{{QStringLiteral("originalPath"), document.filePath()},
                               {QStringLiteral("body"), document.body()}};
    if (m_saved.value(document.id()) == snapshot) return true;

    if (!m_sessionLock) {
        const QString session = QDir(m_root).filePath(QUuid::createUuid().toString(QUuid::WithoutBraces));
        if (!QDir().mkpath(session)) return false;
        auto lock = std::make_unique<QLockFile>(QDir(session).filePath(QStringLiteral("active.lock")));
        lock->setStaleLockTime(0);
        if (!lock->tryLock(0)) return false;
        m_session = session;
        m_sessionLock = std::move(lock);
    }

    QSaveFile file(QDir(m_session).filePath(document.id() + QStringLiteral(".json")));
    const QByteArray data = QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        return false;
    m_saved.insert(document.id(), snapshot);
    return true;
}

void DraftRecovery::clear(const QString& documentId)
{
    if (m_saved.remove(documentId) > 0)
        QFile::remove(QDir(m_session).filePath(documentId + QStringLiteral(".json")));
}

std::vector<DraftRecovery::Snapshot> DraftRecovery::findOrphaned()
{
    const QDir root(m_root);
    std::vector<Snapshot> results;
    for (const auto& entry : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks)) {
        if (QUuid(entry.fileName()).isNull() || entry.absoluteFilePath() == m_session) continue;
        const QDir session(entry.absoluteFilePath());
        auto lock = std::make_unique<QLockFile>(session.filePath(QStringLiteral("active.lock")));
        lock->setStaleLockTime(0);
        if (!lock->tryLock(0)) continue;
        for (const auto& file : session.entryInfoList({QStringLiteral("*.json")}, QDir::Files | QDir::NoSymLinks)) {
            QFile source(file.absoluteFilePath());
            if (!source.open(QIODevice::ReadOnly)) continue;
            const auto snapshot = QJsonDocument::fromJson(source.readAll()).object();
            const auto body = snapshot.value(QStringLiteral("body")).toObject();
            if (body.value(QStringLiteral("type")).toString() != QLatin1String("doc")) continue;
            results.push_back({file.absoluteFilePath(), snapshot.value(QStringLiteral("originalPath")).toString(),
                               file.lastModified(), body});
        }
        m_orphanLocks.push_back(std::move(lock));
    }
    std::sort(results.begin(), results.end(), [](const auto& left, const auto& right) {
        return left.savedAt > right.savedAt;
    });
    return results;
}

void DraftRecovery::removeSnapshot(const Snapshot& snapshot)
{
    QFile::remove(snapshot.path);
}

} // namespace vela
