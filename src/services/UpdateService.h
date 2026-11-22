// ════════════════════════════════════════════════════════════════════════════
//  UpdateService.h — Phase 6 (check GitHub Releases for new versions)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

namespace vela {

class UpdateService : public QObject
{
    Q_OBJECT
public:
    explicit UpdateService(QObject* parent = nullptr);

    /// Async check for updates. Result emitted via updateAvailable().
    void checkForUpdates();

    /// Open the release URL in the default browser.
    void openReleasePage(const QUrl& url);

signals:
    void updateAvailable(const QString& version, const QUrl& releaseUrl, const QString& notes);
    void upToDate();
    void checkFailed(const QString& error);
};

} // namespace vela
