// ════════════════════════════════════════════════════════════════════════════
//  CrashHandler.h — Minidumps + opt-in report (v0.3.4 — PROD-04)
// ════════════════════════════════════════════════════════════════════════════
#pragma once

#include <QString>

namespace vela {

/**
 * @brief Install platform-native unhandled-exception handlers.
 *
 * On Windows: registers `SetUnhandledExceptionFilter` and writes a
 * minidump to `crashDumpsDir()/vela-<timestamp>.dmp` plus a sibling
 * `vela-<timestamp>.crashlog`.
 *
 * On macOS/Linux: installs signal handlers (SIGSEGV, SIGABRT, SIGFPE).
 *
 * Calling `install()` more than once is safe.
 */
class CrashHandler
{
public:
    static void install();
    static bool isInstalled();

    /// Set the URL the next crash report upload will POST to.
    /// Pass empty to disable uploads entirely (local-only dumps).
    static void setUploadEndpoint(const QString& url);

    /// Last crash minidump path, or empty if none.
    static QString lastDumpPath();
};

} // namespace vela
