// ════════════════════════════════════════════════════════════════════════════
//  LegacyDocBridge.h — Bridge to LibreOffice for legacy .doc/.rtf/.odt
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>

namespace vela {

/**
 * @brief Converts .doc/.rtf/.odt ↔ .docx by invoking LibreOffice headless.
 *
 * Phase 4 will implement this with bundled LibreOffice (or slim build).
 */
class LegacyDocBridge : public QObject
{
    Q_OBJECT
public:
    explicit LegacyDocBridge(QObject* parent = nullptr);

    /// Convert a legacy file to .docx for editing. Returns true on success.
    bool convertToDocx(const QString& sourcePath, const QString& destDocxPath, QString& outError);

    /// Convert a .docx back to legacy format. Returns true on success.
    bool convertFromDocx(const QString& sourceDocxPath, const QString& destPath, QString& outError);

signals:
    void progressChanged(qreal fraction);
};

} // namespace vela
