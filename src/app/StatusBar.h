// ════════════════════════════════════════════════════════════════════════════
//  StatusBar.h — Bottom status bar with page / word / zoom info
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QStatusBar>

class QLabel;
class QProgressBar;
class QSlider;

namespace vela {

/**
 * @brief Custom status bar with permanent widgets (word count, zoom, etc.).
 */
class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget* parent = nullptr);

public slots:
    void setReadyMessage(const QString& text);
    void setPageInfo(int current, int total);
    void setWordCount(int words, int characters);
    void setZoomPercent(int pct);
    void setLanguage(const QString& lang);

private slots:
    void onZoomChanged(int value);

signals:
    void zoomChanged(int percent);

private:
    QLabel*      m_message     = nullptr;
    QLabel*      m_pageLabel   = nullptr;
    QLabel*      m_wordLabel   = nullptr;
    QLabel*      m_langLabel   = nullptr;
    QLabel*      m_zoomLabel   = nullptr;
    QSlider*     m_zoomSlider  = nullptr;
    QProgressBar* m_progress   = nullptr;
    int          m_currentZoom = 100;
};

} // namespace vela
