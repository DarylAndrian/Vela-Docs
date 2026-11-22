// test_PrintService.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "services/PrintService.h"
#include "document/Document.h"

TEST_CASE("PrintService — renders to a QPrinter", "[print]") {
    Document d; d.fromPlainText("Print this");
    PrintService ps;
    // Smoke — the test is that no exceptions escape.
    ps.renderToPdf(&d, QStringLiteral("/tmp/vela-test-print.pdf"));
    SUCCEED("did not crash");
}
