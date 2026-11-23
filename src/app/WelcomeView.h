#pragma once

#include <QStringList>
#include <QWidget>

class QVBoxLayout;

namespace vela {

class WelcomeView : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomeView(QWidget* parent = nullptr);
    void setRecentFiles(const QStringList& paths);

signals:
    void newRequested();
    void openRequested();
    void recentRequested(const QString& path);

private:
    QVBoxLayout* m_recentFilesLayout = nullptr;
};

} // namespace vela
