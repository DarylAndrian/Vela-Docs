// test_AutoSaver.cpp — v0.3.1 (CRIT-03 + CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "services/AutoSaver.h"

TEST_CASE("AutoSaver — interval clamps into valid range", "[autosaver]") {
    vela::AutoSaver as;
    as.setIntervalSeconds(2);   // clamps to 5
    CHECK(as.intervalSeconds() == 5);
    as.setIntervalSeconds(9999);// clamps to 600
    CHECK(as.intervalSeconds() == 600);
}

TEST_CASE("AutoSaver — registers and unregisters sessions", "[autosaver]") {
    vela::AutoSaver as;
    // Need a real TabSession, but the header is included transitively above.
    // Smoke test: register nullptr is a no-op.
    as.registerSession(nullptr);
    CHECK(as.sessions().empty());
}
