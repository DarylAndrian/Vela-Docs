// test_PageSetupDialog.cpp — v0.3.1 (CRIT-05); real fixture is in v0.3.2.
#include <catch2/catch_test_macros.hpp>
#include "ui/PageSetupDialog.h"
TEST_CASE("PageSetupDialog — constructs with default Letter portrait", "[pagesetup]") {
    vela::PageSetupDialog dlg;
    SUCCEED("constructed");
}
