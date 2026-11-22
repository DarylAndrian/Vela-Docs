// test_PathUtils.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "util/PathUtils.h"

TEST_CASE("PathUtils — sanitizeFilename strips bad chars", "[pathutils]") {
    using namespace vela;
    const QString in  = QStringLiteral("a/b\\c:d*e?f\"g<h>i|\n");
    const QString out = PathUtils::sanitizeFilename(in);
    CHECK_FALSE(out.contains('/'));
    CHECK_FALSE(out.contains('\\'));
    CHECK_FALSE(out.contains(':'));
}

TEST_CASE("PathUtils — autosave + crashDumps dirs are absolute", "[pathutils]") {
    using namespace vela;
    QVERIFY2(QFileInfo(PathUtils::autosaveDir()).isAbsolute(), "autosave");
    QVERIFY2(QFileInfo(PathUtils::crashDumpsDir()).isAbsolute(), "crashDumps");
}
