// ════════════════════════════════════════════════════════════════════════════
//  Logger.h — Lightweight logger with file output
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QDebug>
#include <QString>
#include <QFile>
#include <QMutex>
#include <QObject>

namespace vela {

/**
 * @brief Singleton logger that writes to a rotating file in the per-user
 *        log directory. Thread-safe.
 */
class Logger : public QObject
{
    Q_OBJECT
public:
    static Logger& instance();

    /// Install as Qt's message handler so qDebug/qInfo/warning/critical
    /// all flow through us.
    void installMessageHandler();

    /// Toggle verbose (debug) messages on/off.
    void setVerbose(bool on);
    bool isVerbose() const { return m_verbose; }

    /// Write a structured log line (timestamped). Internal API.
    void write(QtMsgType level, const QString& message);

private:
    explicit Logger(QObject* parent = nullptr);
    ~Logger() override;

    QFile   m_file;
    QMutex  m_mutex;
    bool    m_verbose = false;
};

// ─── Convenience macros ───────────────────────────────────────────────────
#define VE_LOG_DEBUG(msg)  do { if (::vela::Logger::instance().isVerbose()) { \
        QString _s; QDebug _dbg(&_s); _dbg << msg; \
        ::vela::Logger::instance().write(QtDebugMsg, _s); } } while (0)
#define VE_LOG_INFO(msg)   do { QString _s; QDebug _dbg(&_s); _dbg << msg; \
        ::vela::Logger::instance().write(QtInfoMsg, _s); } while (0)
#define VE_LOG_WARNING(msg)  do { QString _s; QDebug _dbg(&_s); _dbg << msg; \
        ::vela::Logger::instance().write(QtWarningMsg, _s); } while (0)
#define VE_LOG_ERROR(msg)  do { QString _s; QDebug _dbg(&_s); _dbg << msg; \
        ::vela::Logger::instance().write(QtCriticalMsg, _s); } while (0)

} // namespace vela
