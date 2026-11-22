// ════════════════════════════════════════════════════════════════════════════
//  LibreOfficeHelper.h — Phase 4 (manage bundled LibreOffice)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

namespace vela {

class LibreOfficeHelper : public QObject
{
    Q_OBJECT
public:
    explicit LibreOfficeHelper(QObject* parent = nullptr);

    bool isInstalled() const;
    QString installPath() const;

    /// Spawn a headless conversion. Async; result via signals.
    void convert(const QString& from, const QString& to, const QString& targetFormat);

signals:
    void conversionFinished(bool ok, const QString& message);

private:
    QProcess* m_proc = nullptr;
};

} // namespace vela
