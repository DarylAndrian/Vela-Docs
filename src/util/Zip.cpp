// ════════════════════════════════════════════════════════════════════════════
//  Zip.cpp — Minimal zip read/write using Qt's QZip
// ════════════════════════════════════════════════════════════════════════════
#include "util/Zip.h"
#include "util/Logger.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryFile>
// QZipReader / QZipWriter are QtCore *private* headers (QtCore/6.x/QtCore/private),
// reached through the Qt6::GuiPrivate -> Qt6::CorePrivate include chain.
#include <private/qzipreader_p.h>
#include <private/qzipwriter_p.h>

namespace vela {

bool Zip::openForReading(const QString& path, QHash<QString, QByteArray>& entries)
{
    QFileInfo info(path);
    if (!info.exists()) {
        VE_LOG_WARNING("Zip::openForReading — file missing: " << path);
        return false;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        VE_LOG_WARNING("Zip::openForReading — open failed: " << f.errorString());
        return false;
    }

    QZipReader zip(&f);
    const QList<QZipReader::FileInfo> infos = zip.fileInfoList();
    for (const auto& info : infos) {
        if (info.isDir) continue;
        entries.insert(info.filePath, zip.fileData(info.filePath));
    }
    zip.close();
    VE_LOG_DEBUG("Zip::openForReading loaded " << entries.size() << " entries from " << path);
    return true;
}

bool Zip::write(const QString& path, const QHash<QString, QByteArray>& entries)
{
    // QZipWriter closes its device, so stage the archive before committing it
    // with QSaveFile, which must keep its own device open until commit().
    QSaveFile destination(path);
    if (!destination.open(QIODevice::WriteOnly)) {
        VE_LOG_ERROR("Zip::write — open failed: " << destination.errorString());
        return false;
    }

    QTemporaryFile staging(QFileInfo(path).absolutePath() + QStringLiteral("/.vela-XXXXXX"));
    if (!staging.open()) {
        VE_LOG_ERROR("Zip::write — could not stage archive: " << staging.errorString());
        return false;
    }
    {
        QZipWriter zip(&staging);
        zip.setCompressionPolicy(QZipWriter::AlwaysCompress);
        for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
            zip.addFile(it.key(), it.value());
        }
        if (!staging.flush() || zip.status() != QZipWriter::NoError) return false;
        zip.close();
        if (zip.status() != QZipWriter::NoError || staging.error() != QFileDevice::NoError) {
            VE_LOG_ERROR("Zip::write — could not finish archive: " << staging.errorString());
            return false;
        }
    }

    QFile source(staging.fileName());
    if (!source.open(QIODevice::ReadOnly)) return false;
    QByteArray buffer(64 * 1024, Qt::Uninitialized);
    qint64 count;
    while ((count = source.read(buffer.data(), buffer.size())) > 0) {
        if (destination.write(buffer.constData(), count) != count) return false;
    }
    if (count < 0 || !destination.commit()) {
        VE_LOG_ERROR("Zip::write — could not commit " << path << ": " << destination.errorString());
        return false;
    }
    VE_LOG_DEBUG("Zip::write wrote " << entries.size() << " entries to " << path);
    return true;
}

QByteArray Zip::readEntry(QIODevice* device, const QString& entryName)
{
    QZipReader zip(device);
    const QByteArray data = zip.fileData(entryName);
    zip.close();
    return data;
}

} // namespace vela
