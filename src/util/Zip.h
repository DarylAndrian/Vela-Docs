// ════════════════════════════════════════════════════════════════════════════
//  Zip.h — Thin wrapper around QuaZip-style operations on .docx/.odt files
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QByteArray>
#include <QString>
#include <QHash>

class QIODevice;

namespace vela {

/**
 * @brief Lightweight ZIP reader/writer.
 *
 * Phase 0: full implementation backed by QZip. Used in Phase 1+ to
 *          read/write .docx (which is just a zip of XML parts).
 *
 * API is intentionally simple: extract a named entry, write a named entry,
 * and iterate entries. No streaming, no advanced features.
 */
class Zip
{
public:
    /// Open an existing zip from disk. Returns true on success.
    static bool openForReading(const QString& path,
                               QHash<QString, QByteArray>& entries);

    /// Write a new zip to disk. Returns true on success.
    static bool write(const QString& path,
                      const QHash<QString, QByteArray>& entries);

    /// Read a single entry from an open zip device.
    static QByteArray readEntry(QIODevice* device, const QString& entryName);
};

} // namespace vela
