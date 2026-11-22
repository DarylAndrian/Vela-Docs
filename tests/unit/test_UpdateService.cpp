// test_UpdateService.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "services/UpdateService.h"

TEST_CASE("UpdateService — currentVersion returns non-empty", "[updates]") {
    UpdateService u;
    CHECK_FALSE(u.currentVersion().isEmpty());
}

TEST_CASE("UpdateService — checkForUpdates does not throw", "[updates]") {
    UpdateService u;
    u.checkForUpdates();
    SUCCEED("did not throw");
}
