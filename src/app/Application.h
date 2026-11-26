// ════════════════════════════════════════════════════════════════════════════
//  Application.h — QApplication subclass for Vela Docs
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QApplication>
#include <QFont>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>

// QPointer<MainWindow> needs MainWindow to be complete.
#include "app/MainWindow.h"

namespace vela {

class SettingsStore;
class RevisionStore;

/**
 * @brief The Application class owns the global application state and the main
 *        window. It centralizes initialization so individual components don't
 *        need to know about each other.
 */
class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int& argc, char** argv);
    ~Application() override;

    void showMainWindow(const QString& initialFile = {});
    void enqueueDocumentLaunch(const QStringList& files);

    MainWindow* mainWindow() const { return m_mainWindow; }
    SettingsStore* settings() const { return m_settings; }
    RevisionStore* revisions() const { return m_revisions; }

    static QString dataDirectory();
    static QString logDirectory();

private:
    void registerQtResources();
    void loadApplicationStyle();
    void processDocumentLaunch();
    QFont pickFont(const QString& preferred, const QStringList& fallbacks) const;

    QPointer<MainWindow>    m_mainWindow;
    SettingsStore*          m_settings  = nullptr;
    RevisionStore*          m_revisions = nullptr;
    QStringList             m_pendingLaunches;
    QTimer                  m_launchTimer;
    bool                    m_handlingLaunch = false;
};

} // namespace vela
