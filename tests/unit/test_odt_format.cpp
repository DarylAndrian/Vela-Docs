// test_odt_format.cpp — v0.3.1 (CRIT-04 round 1)
//
// Placeholder tests for ODT import/export. The full v0.3.3 fidelity work
// upgrades the importer to handle styles, images, and tables; v0.3.1 only
// tests the smoke-path.

#include <catch2/catch_test_macros.hpp>
#include "document/Document.h"
#include "formats/OdtImporter.h"
#include "formats/OdtExporter.h"
#include <QCoreApplication>
#include <QTemporaryFile>

TEST_CASE("OdtImporter — missing file fails cleanly", "[odt]") {
    OdtImporter imp;
    Document d;
    CHECK_FALSE(imp.import("/nonexistent/path.odt", &d));
}

TEST_CASE("OdtImporter — empty file", "[odt]") {
    OdtImporter imp; Document d;
    QTemporaryFile f(QCoreApplication::instance() ? QCoreApplication::instance()
                       : (QCoreApplication*)nullptr);
    f.setAutoRemove(false); f.open(); f.close();
    // Empty content.xml or missing parts should not crash.
    imp.import(f.fileName(), &d);
    SUCCEED("did not crash");
}

TEST_CASE("OdtExporter — empty document writes valid (empty) opendocument", "[odt]") {
    OdtExporter exp; Document d;
    QTemporaryFile f;
    f.setAutoRemove(false); f.open(); f.close();
    REQUIRE(exp.exportTo(&d, f.fileName()));
}
