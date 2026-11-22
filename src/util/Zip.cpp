// ════════════════════════════════════════════════════════════════════════════
//  Zip.cpp — Minimal zip read/write using Qt's QZip
// ════════════════════════════════════════════════════════════════════════════
#include "util/Zip.h"
#include "util/Logger.h"

#include <QFile>
#include <QFileInfo>
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
    // NOT QSaveFile: QZipWriter::close() dies with STATUS_STACK_BUFFER_OVERRUN
    // (0xC0000409) when the device is a QSaveFile — reproduced in isolation
    // against Qt 6.10.3. A plain QFile works, so write to a scratch file and
    // rename it into place; that keeps the atomic replace QSaveFile promised.
    const QString tmpPath = path + QStringLiteral(".velatmp");
    QFile f(tmpPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        VE_LOG_ERROR("Zip::write — open failed: " << f.errorString());
        return false;
    }

    QZipWriter zip(&f);
    zip.setCompressionPolicy(QZipWriter::AlwaysCompress);
    for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
        zip.addFile(it.key(), it.value());
    }
    zip.close();
    f.close();

    QFile::remove(path);
    if (!QFile::rename(tmpPath, path)) {
        VE_LOG_ERROR("Zip::write — could not move " << tmpPath << " to " << path);
        QFile::remove(tmpPath);
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
