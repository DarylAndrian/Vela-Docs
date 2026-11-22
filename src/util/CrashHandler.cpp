// CrashHandler.cpp
#include "util/CrashHandler.h"
#include "util/PathUtils.h"
#include "util/Logger.h"

#include <QDateTime>
#include <QFile>
#include <QStandardPaths>

#include <atomic>

#ifdef _WIN32
#  include <Windows.h>
#  include <DbgHelp.h>
#  pragma comment(lib, "dbghelp.lib")
#endif

namespace vela {

namespace {
std::atomic<bool> g_installed{false};
QString g_uploadEndpoint;
QString g_lastDump;
}

void CrashHandler::install()
{
    if (g_installed.exchange(true)) return;
#ifdef _WIN32
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ep) -> LONG {
        const QString dir  = PathUtils::crashDumpsDir();
        const QString ts   = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"));
        const QString path = QStringLiteral("%1/vela-%2.dmp").arg(dir, ts);
        HANDLE h = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), GENERIC_WRITE, 0, nullptr,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION info;
            info.ThreadId          = GetCurrentThreadId();
            info.ExceptionPointers = ep;
            info.ClientPointers    = FALSE;
            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), h, MiniDumpNormal, &info, nullptr, nullptr);
            CloseHandle(h);
            g_lastDump = path;
            Logger::error(QStringLiteral("CrashHandler: minidump written to %1").arg(path));
        }
        return EXCEPTION_EXECUTE_HANDLER;
    });
#endif
    Logger::info(QStringLiteral("CrashHandler: installed (uploads to %1)").arg(g_uploadEndpoint));
}

bool CrashHandler::isInstalled() { return g_installed; }

void CrashHandler::setUploadEndpoint(const QString& url) { g_uploadEndpoint = url; }

QString CrashHandler::lastDumpPath() { return g_lastDump; }

} // namespace vela
