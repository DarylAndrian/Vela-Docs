// test_PreferencesDialog.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "ui/PreferencesDialog.h"
TEST_CASE("PreferencesDialog — constructs", "[preferences]") {
    vela::PreferencesDialog dlg;
    SUCCEED("did not crash");
}
