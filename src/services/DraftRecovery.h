#pragma once

#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <memory>
#include <vector>

class QLockFile;

namespace vela {

class Document;

class DraftRecovery
{
public:
    struct Snapshot
    {
        QString path;
        QString originalPath;
        QDateTime savedAt;
        QJsonObject body;
    };

    explicit DraftRecovery(const QString& root);
    ~DraftRecovery();

    bool store(const Document& document);
    void clear(const QString& documentId);
    std::vector<Snapshot> findOrphaned();
    static void removeSnapshot(const Snapshot& snapshot);

private:
    QString m_root;
    QString m_session;
    std::unique_ptr<QLockFile> m_sessionLock;
    std::vector<std::unique_ptr<QLockFile>> m_orphanLocks;
    QHash<QString, QJsonObject> m_saved;
};

} // namespace vela
