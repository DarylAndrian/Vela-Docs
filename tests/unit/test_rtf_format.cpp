// test_rtf_format.cpp — v0.3.1 (CRIT-04 round 1)
//
// Smoke tests for RtfImporter. Full character formatting / tables / images
// land in v0.3.3.

#include <catch2/catch_test_macros.hpp>
#include "document/Document.h"
#include "formats/RtfImporter.h"
#include <QString>
#include <QTemporaryFile>

TEST_CASE("RtfImporter — minimal valid RTF", "[rtf]") {
    RtfImporter imp; Document d;
    QTemporaryFile f;
    f.setAutoRemove(false);
    f.open();
    f.write(R"({\rtf1\ansi Hello \b world\b0})");
    f.close();
    // Should at least not crash; text content may be partial in v0.3.1.
    imp.import(f.fileName(), &d);
    SUCCEED("did not crash");
}

TEST_CASE("RtfImporter — malformed input", "[rtf]") {
    RtfImporter imp; Document d;
    QTemporaryFile f;
    f.setAutoRemove(false);
    f.open();
    f.write("not really rtf");
    f.close();
    imp.import(f.fileName(), &d);
    SUCCEED("did not crash on malformed input");
}
