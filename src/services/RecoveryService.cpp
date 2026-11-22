// ════════════════════════════════════════════════════════════════════════════
//  RecoveryService.cpp
// ════════════════════════════════════════════════════════════════════════════
#include "services/RecoveryService.h"

#include "util/Logger.h"
#include "util/PathUtils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>

namespace vela {

RecoveryService::RecoveryService(QObject* parent)
    : QObject(parent) {}

QString RecoveryService::autosaveDir() const
{
    return PathUtils::autosaveDir();
}

std::vector<RecoverableDocument> RecoveryService::scan()
{
    std::vector<RecoverableDocument> out;
    const QString dir = autosaveDir();
    QDir d(dir);
    if (!d.exists()) {
        emit recoverablesFound(out);
        return out;
    }

    // Sidecar: a JSON sibling at "<uuid>.json" maps the UUID to originalPath
    // and timestamp (the autosave itself is a real .docx).
    const QStringList entries = d.entryList(QStringList() << QStringLiteral("*.docx"),
                                            QDir::Files, QDir::Time);
    for (const QString& file : entries) {
        const QString full = d.absoluteFilePath(file);
        QFileInfo fi(full);
        if (!fi.exists() || fi.size() == 0)
            continue;

        RecoverableDocument r;
        r.id           = QUuid(file.left(file.size() - 5)); // strip ".docx"
        r.autosavePath = full;
        r.saved        = fi.lastModified();
        r.sizeBytes    = fi.size();

        // Read sidecar if present.
        const QString meta = full.left(full.size() - 5) + QStringLiteral(".json");
        QFile m(meta);
        if (m.open(QIODevice::ReadOnly)) {
            const QByteArray data = m.readAll();
            // Minimal JSON parse — we control the writer.
            const QByteArrayList lines = data.split('\n');
            for (const QByteArray& ln : lines) {
                if (ln.startsWith("originalPath="))
                    r.originalPath = QString::fromUtf8(ln.mid(13).trimmed());
            }
        }

        out.push_back(r);
    }

    emit recoverablesFound(out);
    return out;
}

bool RecoveryService::hasRecoverables()
{
    return !scan().empty();
}

QString RecoveryService::recover(const RecoverableDocument& doc)
{
    // Copy the autosave to a recovery-named file in the user's temp dir.
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString target = QStringLiteral("%1/Recovered_%2.docx")
                              .arg(tempDir, doc.id.toString(QUuid::WithoutBraces));
    QFile::remove(target);
    if (!QFile::copy(doc.autosavePath, target)) {
        Logger::warning(QStringLiteral("RecoveryService: failed to copy %1").arg(doc.autosavePath));
        return {};
    }
    Logger::info(QStringLiteral("RecoveryService: recovered %1 → %2")
                     .arg(doc.autosavePath, target));
    return target;
}

bool RecoveryService::discard(const RecoverableDocument& doc)
{
    bool a = QFile::remove(doc.autosavePath);
    const QString sidecar = doc.autosavePath.left(doc.autosavePath.size() - 5) + QStringLiteral(".json");
    bool b = QFile::remove(sidecar);
    return a && b;
}

} // namespace vela
