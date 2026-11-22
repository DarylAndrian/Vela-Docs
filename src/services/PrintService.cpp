#include "services/PrintService.h"
#include "util/Logger.h"

namespace vela {

PrintService::PrintService(QObject* parent) : QObject(parent) {}

void PrintService::showPreview(QTextDocument* doc, QWidget* parent)
{
    Q_UNUSED(doc);
    Q_UNUSED(parent);
    VE_LOG_DEBUG("PrintService::showPreview — Phase 3");
}

void PrintService::print(QTextDocument* doc, QWidget* parent)
{
    Q_UNUSED(doc);
    Q_UNUSED(parent);
    VE_LOG_DEBUG("PrintService::print — Phase 3");
}

} // namespace vela
