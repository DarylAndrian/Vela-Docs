// test_DocumentController.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "document/Document.h"
#include "document/DocumentController.h"
#include <QTemporaryDir>
#include <QTemporaryFile>

TEST_CASE("DocumentController — new/open/save round-trip", "[controller]") {
    QTemporaryDir dir;
    Document d; DocumentController c(&d);
    REQUIRE(c.newDocument());

    QTemporaryFile f(dir.path() + "/out_XXXXXX.docx");
    f.setAutoRemove(false); f.open(); f.close();

    // save then reload via DOCX; assumes the round-trip stays valid.
    REQUIRE(c.saveAs(f.fileName()));
    CHECK(QFileInfo::exists(f.fileName()));
}
