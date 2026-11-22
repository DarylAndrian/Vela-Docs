// ════════════════════════════════════════════════════════════════════════════
//  PrintService.h — Phase 3
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QObject>
#include <QString>
#include <QPrinter>

class QTextDocument;

namespace vela {

class PrintService : public QObject
{
    Q_OBJECT
public:
    explicit PrintService(QObject* parent = nullptr);

    /// Show a print preview dialog for the active document.
    void showPreview(QTextDocument* doc, QWidget* parent = nullptr);

    /// Send to printer (system dialog).
    void print(QTextDocument* doc, QWidget* parent = nullptr);
};

} // namespace vela
