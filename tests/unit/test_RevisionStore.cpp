// test_RevisionStore.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "services/RevisionStore.h"
#include <QDateTime>

TEST_CASE("RevisionStore — adds then lists revisions", "[revisions]") {
    RevisionStore rs;
    const QString id = QStringLiteral("doc-test-1");
    rs.addRevision(id, "first",  "rev1", QDateTime::currentDateTime());
    rs.addRevision(id, "second", "rev2", QDateTime::currentDateTime());
    const auto list = rs.list(id);
    CHECK(list.size() == 2);
}
