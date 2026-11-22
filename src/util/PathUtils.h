// ════════════════════════════════════════════════════════════════════════════
//  PathUtils.h — Path helpers
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QString>

namespace vela {

/**
 * @brief Cross-platform path helpers.
 */
namespace PathUtils {

    /// The local app data directory (already-created, always writable).
    QString dataDir();

    /// Path for cached dictionaries.
    QString dictionariesDir();

    /// Path for autosave files (v0.3.1 — CRIT-03).
    QString autosaveDir();

    /// Path for crash dumps (v0.3.4 — PROD-04).
    QString crashDumpsDir();

    /// Path for the LibreOffice helper (when bundled).
    QString libreOfficeHelperPath();

    /// Sanitize a string for safe use as a filename.
    QString sanitizeFilename(const QString& name);

    /// Returns true if the file extension matches (case-insensitive).
    bool hasExtension(const QString& path, const QString& ext);

} // namespace PathUtils

} // namespace vela
