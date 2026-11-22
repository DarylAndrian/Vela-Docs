// test_pdf_export.cpp — v0.3.1 (CRIT-04 round 1)
//
// Smoke tests for PdfExporter. Bookmarks/hyperlinks/PDF-A land in v0.3.5.

#include <catch2/catch_test_macros.hpp>
#include "document/Document.h"
#include "formats/PdfExporter.h"
#include <QTemporaryFile>

TEST_CASE("PdfExporter — writes a non-empty PDF", "[pdf]") {
    PdfExporter exp; Document d;
    d.fromPlainText("Hello PDF");
    QTemporaryFile f;
    f.setAutoRemove(false); f.open(); f.close();
    REQUIRE(exp.exportTo(&d, f.fileName()));

    QFileInfo fi(f.fileName());
    REQUIRE(fi.size() > 0);
}

TEST_CASE("PdfExporter — empty document", "[pdf]") {
    PdfExporter exp; Document d;
    QTemporaryFile f; f.setAutoRemove(false); f.open(); f.close();
    REQUIRE(exp.exportTo(&d, f.fileName()));
}

TEST_CASE("PdfExporter — bad path fails gracefully", "[pdf]") {
    PdfExporter exp; Document d;
    d.fromPlainText("X");
    CHECK_FALSE(exp.exportTo(&d, "/nonexistent/dir/out.pdf"));
}
