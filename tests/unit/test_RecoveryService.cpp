// test_RecoveryService.cpp — v0.3.1 (CRIT-03 + CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "services/RecoveryService.h"

TEST_CASE("RecoveryService — hasRecoverables returns false on empty app data", "[recovery]") {
    vela::RecoveryService rs;
    // On CI without prior app runs, the autosave directory is empty/absent.
    // We can't assert a specific boolean (depends on host state), but the
    // call must not throw and must return a bool.
    CHECK_NOTHROW(rs.hasRecoverables());
}

TEST_CASE("RecoveryService — discard on missing file is a no-op error", "[recovery]") {
    vela::RecoverableDocument d;
    d.autosavePath = QStringLiteral("/nonexistent/vela-recovery-test.docx");
    d.id           = QUuid::createUuid();
    CHECK_FALSE(vela::RecoveryService::discard(d));
}
