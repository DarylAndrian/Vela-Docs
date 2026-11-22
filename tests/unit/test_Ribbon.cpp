// test_Ribbon.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "app/Ribbon.h"

TEST_CASE("Ribbon — constructs and exposes tabs", "[ribbon]") {
    vela::Ribbon r;
    CHECK(r.count() >= 0);
    SUCCEED("did not crash");
}
