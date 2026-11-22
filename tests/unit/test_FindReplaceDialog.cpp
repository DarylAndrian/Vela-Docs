// test_FindReplaceDialog.cpp — v0.3.1 stub; v0.3.2 wires to the editor.
#include <catch2/catch_test_macros.hpp>
#include "ui/FindReplaceDialog.h"
TEST_CASE("FindReplaceDialog — constructs empty", "[findreplace]") {
    vela::FindReplaceDialog dlg;
    SUCCEED("did not crash");
}
