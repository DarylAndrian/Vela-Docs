#include "services/UpdateService.h"
#include "util/Logger.h"

#include <QDesktopServices>

namespace vela {

UpdateService::UpdateService(QObject* parent) : QObject(parent) {}

void UpdateService::checkForUpdates()
{
    VE_LOG_DEBUG("UpdateService::checkForUpdates — Phase 6 will hit GitHub Releases API");
    emit upToDate(); // no-op for now
}

void UpdateService::openReleasePage(const QUrl& url)
{
    QDesktopServices::openUrl(url);
}

} // namespace vela
